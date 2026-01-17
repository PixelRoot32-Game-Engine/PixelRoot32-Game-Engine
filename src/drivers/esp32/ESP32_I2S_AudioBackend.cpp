#ifdef ARDUINO_ARCH_ESP32

#include "drivers/esp32/ESP32_I2S_AudioBackend.h"
#include "audio/AudioEngine.h"
#include <Arduino.h>

namespace pixelroot32::drivers::esp32 {

    // FreeRTOS task wrapper
    static void audioTaskTrampoline(void* arg) {
        auto* backend = static_cast<ESP32_I2S_AudioBackend*>(arg);
        if (backend) {
            backend->audioTaskLoop();
        }
        vTaskDelete(NULL);
    }

    ESP32_I2S_AudioBackend::ESP32_I2S_AudioBackend(int bclkPin, int wclkPin, int doutPin, int sampleRate)
        : bclkPin(bclkPin), wclkPin(wclkPin), doutPin(doutPin), sampleRate(sampleRate) {}

    ESP32_I2S_AudioBackend::~ESP32_I2S_AudioBackend() {
        if (audioTaskHandle) {
            vTaskDelete(audioTaskHandle);
        }
        i2s_driver_uninstall(I2S_NUM_0);
    }

    void ESP32_I2S_AudioBackend::init(pixelroot32::audio::AudioEngine* engine) {
        this->engineInstance = engine;

        // I2S Configuration
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = (uint32_t)sampleRate,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // Mono usually mixed to one channel or duplicated
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 8,
            .dma_buf_len = 64, // Small buffers for low latency
            .use_apll = false,
            .tx_desc_auto_clear = true
        };

        // Pin Configuration
        i2s_pin_config_t pin_config = {
            .bck_io_num = bclkPin,
            .ws_io_num = wclkPin,
            .data_out_num = doutPin,
            .data_in_num = I2S_PIN_NO_CHANGE
        };

        // Install and start I2S driver
        esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
        if (err != ESP_OK) {
            Serial.printf("Failed to install I2S driver: %d\n", err);
            return;
        }

        err = i2s_set_pin(I2S_NUM_0, &pin_config);
        if (err != ESP_OK) {
            Serial.printf("Failed to set I2S pins: %d\n", err);
            return;
        }

        // Create audio task pinned to Core 0 (App usually runs on Core 1)
        xTaskCreatePinnedToCore(
            audioTaskTrampoline,
            "AudioTask",
            4096,
            this,
            configMAX_PRIORITIES - 1, // High priority
            &audioTaskHandle,
            0
        );
    }

    void ESP32_I2S_AudioBackend::audioTaskLoop() {
        const int BUFFER_SAMPLES = 256;
        int16_t sampleBuffer[BUFFER_SAMPLES];
        size_t bytesWritten;

        while (true) {
            if (engineInstance) {
                // Generate samples directly into our local buffer
                engineInstance->generateSamples(sampleBuffer, BUFFER_SAMPLES);
                
                // Write to I2S (blocking if DMA is full)
                // Note: sampleBuffer is int16_t, I2S expects bytes
                i2s_write(I2S_NUM_0, sampleBuffer, BUFFER_SAMPLES * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
            } else {
                // Should not happen, but wait a bit to avoid watchdog
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }

}

#endif // ARDUINO_ARCH_ESP32
