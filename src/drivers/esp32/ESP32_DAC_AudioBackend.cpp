/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "drivers/esp32/ESP32_DAC_AudioBackend.h"
#include "platforms/PlatformDefaults.h"
#include "core/Log.h"

#if defined(PIXELROOT32_USE_DAC_AUDIO)
#include "audio/AudioEngine.h"
#include <Arduino.h>

namespace pixelroot32::drivers::esp32 {

    namespace logging = pixelroot32::core::logging;
    using logging::LogLevel;
    using logging::log;

    // We reuse the legacy I2S driver (same one the I2S backend uses) in
    // `I2S_MODE_DAC_BUILT_IN` mode so the DMA engine, not the CPU, drives
    // the 8-bit DAC.
    static constexpr i2s_port_t DAC_I2S_PORT = I2S_NUM_0;

    static void audioTaskTrampoline(void* arg) {
        auto* backend = static_cast<ESP32_DAC_AudioBackend*>(arg);
        if (backend) backend->audioTaskLoop();
        vTaskDelete(NULL);
    }

    ESP32_DAC_AudioBackend::ESP32_DAC_AudioBackend(int dacPin, int sampleRate)
        : dacPin(dacPin), sampleRate(sampleRate) {}

    ESP32_DAC_AudioBackend::~ESP32_DAC_AudioBackend() {
        if (audioTaskHandle) {
            vTaskDelete(audioTaskHandle);
            audioTaskHandle = nullptr;
        }
        if (i2sInstalled) {
            i2s_driver_uninstall(DAC_I2S_PORT);
            i2sInstalled = false;
        }
    }

    void ESP32_DAC_AudioBackend::init(pixelroot32::audio::AudioEngine* engine,
                                      const pixelroot32::platforms::PlatformCapabilities& caps) {
        this->engineInstance = engine;

        if (dacPin != 25 && dacPin != 26) {
            log(LogLevel::Error, "[DAC] Internal DAC requires GPIO 25 or 26 (got %d)", dacPin);
            return;
        }

        dacChannel = (dacPin == 25) ? DAC_CHANNEL_1 : DAC_CHANNEL_2;
        // Right-channel on I2S maps to DAC1 (GPIO25), Left to DAC2 (GPIO26).
        dacMode = (dacPin == 25) ? I2S_DAC_CHANNEL_RIGHT_EN
                                 : I2S_DAC_CHANNEL_LEFT_EN;

        i2s_config_t cfg = {};
        cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
        cfg.sample_rate = (uint32_t)sampleRate;
        cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        cfg.channel_format  = I2S_CHANNEL_FMT_ONLY_RIGHT;
        cfg.communication_format = I2S_COMM_FORMAT_STAND_MSB;
        cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
        cfg.dma_buf_count = 8;
        cfg.dma_buf_len   = 128;
        cfg.use_apll = false;
        cfg.tx_desc_auto_clear = true;

        esp_err_t err = i2s_driver_install(DAC_I2S_PORT, &cfg, 0, NULL);
        if (err != ESP_OK) {
            log(LogLevel::Error, "[DAC] i2s_driver_install failed: %d", (int)err);
            return;
        }
        i2sInstalled = true;

        err = i2s_set_pin(DAC_I2S_PORT, NULL); // NULL -> route to built-in DAC
        if (err != ESP_OK) {
            log(LogLevel::Error, "[DAC] i2s_set_pin(NULL) failed: %d", (int)err);
            return;
        }

        err = i2s_set_dac_mode(dacMode);
        if (err != ESP_OK) {
            log(LogLevel::Error, "[DAC] i2s_set_dac_mode failed: %d", (int)err);
            return;
        }

        xTaskCreatePinnedToCore(
            audioTaskTrampoline,
            "DACAudioTask",
            4096,
            this,
            caps.audioPriority,
            &audioTaskHandle,
            caps.audioCoreId
        );
        log(LogLevel::Info, "[DAC] I2S DAC-built-in mode active on GPIO %d (core %d)",
            dacPin, caps.audioCoreId);
    }

    void ESP32_DAC_AudioBackend::audioTaskLoop() {
        constexpr int BUFFER_SAMPLES = 256;
        int16_t sampleBuffer[BUFFER_SAMPLES];
        int16_t dmaBuffer[BUFFER_SAMPLES];
        size_t bytesWritten = 0;

        while (true) {
            if (!engineInstance) {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
            }

            engineInstance->generateSamples(sampleBuffer, BUFFER_SAMPLES);

            // ESP32 built-in DAC via I2S expects **unsigned 16-bit** samples
            // where only the top 8 bits are used by the DAC. Convert signed
            // int16 -> offset-binary uint16 (apply the old 0.7× PAM8302A
            // attenuation along the way).
            for (int i = 0; i < BUFFER_SAMPLES; ++i) {
                int32_t s = (int32_t)(sampleBuffer[i] * 0.7f);
                if (s > 32767)  s = 32767;
                if (s < -32768) s = -32768;
                dmaBuffer[i] = (int16_t)((uint16_t)(s + 32768));
            }

            // Blocks until DMA has room; no busy-wait, no per-sample
            // dacWrite() overhead.
            i2s_write(DAC_I2S_PORT,
                      dmaBuffer,
                      BUFFER_SAMPLES * sizeof(int16_t),
                      &bytesWritten,
                      portMAX_DELAY);
        }
    }

}

#endif // PIXELROOT32_USE_DAC_AUDIO
