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
#include <algorithm>
#include <esp_timer.h>

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
        if (dacPin != 25 && dacPin != 26) {
            log(LogLevel::Error, "[DAC] Internal DAC requires GPIO 25 or 26 (got %d)", dacPin);
            return;
        }

        // S12: same rule as the I2S backend. The render block is configuration,
        // read once here; a block the DMA derivation cannot express is rejected
        // before the driver is installed.
        if (!engine) {
            log(LogLevel::Error, "[DAC] init() requires an AudioEngine to read blockSize");
            return;
        }
        const int requestedBlockSize = engine->getBlockSize();
        if (requestedBlockSize <= 0
            || requestedBlockSize > pixelroot32::audio::PR32_AUDIO_MAX_BLOCK_SAMPLES
            || (requestedBlockSize % kDmaBufLenSamples) != 0) {
            log(LogLevel::Error,
                "[DAC] invalid blockSize %d: must be a multiple of %d in (0, %d]",
                requestedBlockSize, kDmaBufLenSamples,
                pixelroot32::audio::PR32_AUDIO_MAX_BLOCK_SAMPLES);
            return;
        }

        this->engineInstance = engine;
        this->isSingleCore = !caps.hasDualCore;
        this->blockSize = requestedBlockSize;

        // S12: DMA depth derived from the render block, exactly as in the I2S
        // backend:
        //   dma_buf_len   = 128 samples, fixed (the granularity blockSize is a
        //                   multiple of)
        //   dma_buf_count = clamp(blockSize / 128, 2, 8)
        // 256 -> 2, 512 -> 4, 1024 -> 8: one render block fits in the DMA ring
        // and `i2s_write(portMAX_DELAY)` paces the task (S13).
        const int dmaBufCount = std::clamp(blockSize / kDmaBufLenSamples, 2, 8);

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
        cfg.dma_buf_count = dmaBufCount;
        cfg.dma_buf_len   = kDmaBufLenSamples;
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
        // S13: the real-time budget of one block is `blockSize / sampleRate`
        // seconds; in integer microseconds it is blockSize * 1e6 / sampleRate.
        const int64_t blockBudgetUs = (sampleRate > 0)
            ? ((int64_t)blockSize * 1000000LL / (int64_t)sampleRate)
            : 0;
        const size_t blockBytes = (size_t)blockSize * sizeof(int16_t);
        size_t bytesWritten = 0;
        uint32_t iteration = 0;

        while (true) {
            if (!engineInstance) {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
            }

            const int64_t iterationStartUs = esp_timer_get_time();

            // S12: render exactly one block, straight into the member buffer.
            engineInstance->generateSamples(renderBuffer_, blockSize);

            // ESP32 built-in DAC via I2S expects **unsigned 16-bit** samples
            // where only the top 8 bits are used by the DAC. Convert signed
            // int16 -> offset-binary uint16 (apply the old 0.7× PAM8302A
            // attenuation along the way). The conversion is done in place: each
            // sample is read before it is overwritten, so one member buffer
            // covers render and DAC tap.
            for (int i = 0; i < blockSize; ++i) {
                int32_t s = (int32_t)(renderBuffer_[i] * 0.7f);
                if (s > 32767)  s = 32767;
                if (s < -32768) s = -32768;
                renderBuffer_[i] = (int16_t)((uint16_t)(s + 32768));
            }

            // Blocks until DMA has room; no busy-wait, no per-sample
            // dacWrite() overhead. portMAX_DELAY is deliberate and is the
            // pacing mechanism (S13): a timeout would turn a late producer into
            // dropped samples, creating the defect the counter measures.
            bytesWritten = 0;
            const int64_t writeStartUs = esp_timer_get_time();
            const esp_err_t err = i2s_write(DAC_I2S_PORT,
                                            renderBuffer_,
                                            blockBytes,
                                            &bytesWritten,
                                            portMAX_DELAY);
            const int64_t writeUs = esp_timer_get_time() - writeStartUs;

            // S13: with a one-block DMA ring a healthy write blocks for most of
            // a block period, so the underrun signal is the opposite of "took
            // long": a write that barely waited means the ring was already
            // empty and the DMA is repeating the last buffer.
            const int64_t iterationUs = esp_timer_get_time() - iterationStartUs;
            const bool countUnderruns = (iteration >= kUnderrunWarmupIterations);
            accountBlockTiming(iterationUs, writeUs, blockBudgetUs,
                               err == ESP_OK && bytesWritten == blockBytes,
                               countUnderruns);
            ++iteration;
        }
    }

    void ESP32_DAC_AudioBackend::accountBlockTiming(int64_t iterationUs, int64_t writeUs,
                                                    int64_t budgetUs, bool writeComplete,
                                                    bool countUnderruns) {
        const uint32_t blockUs = (iterationUs > 0) ? (uint32_t)iterationUs : 0u;
        const uint32_t writeBlockUs = (writeUs > 0) ? (uint32_t)writeUs : 0u;

        // Worst cases over every block, underrun or not: they say how close the
        // producer ran to its deadline and how much slack the ring had.
        uint32_t previousWorst = worstBlockUs_.load(std::memory_order_relaxed);
        while (blockUs > previousWorst
               && !worstBlockUs_.compare_exchange_weak(previousWorst, blockUs,
                                                       std::memory_order_relaxed)) {
        }
        previousWorst = worstWriteUs_.load(std::memory_order_relaxed);
        while (writeBlockUs > previousWorst
               && !worstWriteUs_.compare_exchange_weak(previousWorst, writeBlockUs,
                                                       std::memory_order_relaxed)) {
        }

        if (!countUnderruns) {
            return;
        }

        // S13: starved when the write had less than a tenth of the budget to
        // wait for, or when it failed / came back short. Measured on
        // `i2s_write` alone, not on the whole iteration.
        const int64_t starvedWriteUs = budgetUs / 10;
        if (writeComplete && writeUs >= starvedWriteUs) {
            return;
        }

        const uint32_t underruns = underrunCount_.fetch_add(1, std::memory_order_relaxed) + 1u;
        if ((underruns % 100u) == 0u) {
            log(LogLevel::Warning,
                "[DAC] %u underruns: write blocked %lld us of a %lld us block "
                "(starved below %lld us), worst block %u us, worst write %u us (%d samples @ %d Hz)",
                (unsigned)underruns, (long long)writeUs, (long long)budgetUs,
                (long long)starvedWriteUs,
                (unsigned)worstBlockUs_.load(std::memory_order_relaxed),
                (unsigned)worstWriteUs_.load(std::memory_order_relaxed),
                blockSize, sampleRate);
        }
    }

}

#endif // PIXELROOT32_USE_DAC_AUDIO
