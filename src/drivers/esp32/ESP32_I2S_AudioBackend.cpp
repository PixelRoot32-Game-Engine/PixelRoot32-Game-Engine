/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "drivers/esp32/ESP32_I2S_AudioBackend.h"
#include "core/Log.h"

#if defined(PIXELROOT32_USE_I2S_AUDIO)

#include "audio/AudioEngine.h"
#include <Arduino.h>
#include <algorithm>
#include <esp_timer.h>

namespace pixelroot32::drivers::esp32 {

    namespace logging = pixelroot32::core::logging;
    using logging::LogLevel;
    using logging::log;

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

    void ESP32_I2S_AudioBackend::init(pixelroot32::audio::AudioEngine* engine, const pixelroot32::platforms::PlatformCapabilities& caps) {
        // S12: the render block is configuration. Read it once here, not per
        // iteration, and reject anything the DMA derivation cannot express
        // before installing the driver — a half-configured backend would only
        // fail later, inside the audio task, where it cannot be logged.
        if (!engine) {
            log(LogLevel::Error, "[I2S] init() requires an AudioEngine to read blockSize");
            return;
        }
        const int requestedBlockSize = engine->getBlockSize();
        if (requestedBlockSize <= 0
            || requestedBlockSize > pixelroot32::audio::PR32_AUDIO_MAX_BLOCK_SAMPLES
            || (requestedBlockSize % kDmaBufLenSamples) != 0) {
            log(LogLevel::Error,
                "[I2S] invalid blockSize %d: must be a multiple of %d in (0, %d]",
                requestedBlockSize, kDmaBufLenSamples,
                pixelroot32::audio::PR32_AUDIO_MAX_BLOCK_SAMPLES);
            return;
        }

        this->engineInstance = engine;
        this->isSingleCore = !caps.hasDualCore;
        this->blockSize = requestedBlockSize;

        // S12: DMA depth derived from the render block, once, here:
        //   dma_buf_len   = 128 samples, fixed (the granularity blockSize is a
        //                   multiple of)
        //   dma_buf_count = clamp(blockSize / 128, 2, 8)
        // So 256 -> 2, 512 -> 4 and 1024 -> 8 buffers of 128 samples: the DMA
        // ring holds exactly one render block, which is what makes
        // `i2s_write(portMAX_DELAY)` the pacing mechanism (S13). Do not
        // re-derive these numbers by hand: they follow from blockSize.
        const int dmaBufCount = std::clamp(blockSize / kDmaBufLenSamples, 2, 8);

        // I2S Configuration
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = (uint32_t)sampleRate,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // Mono usually mixed to one channel or duplicated
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = dmaBufCount,
            .dma_buf_len = kDmaBufLenSamples,
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
            log("Failed to install I2S driver: %d", err);
            return;
        }

        err = i2s_set_pin(I2S_NUM_0, &pin_config);
        if (err != ESP_OK) {
            log("Failed to set I2S pins: %d", err);
            return;
        }

        // Create audio task pinned to core specified by capabilities
        xTaskCreatePinnedToCore(
            audioTaskTrampoline,
            "AudioTask",
            4096,
            this,
            caps.audioPriority,
            &audioTaskHandle,
            caps.audioCoreId
        );
    }

    void ESP32_I2S_AudioBackend::audioTaskLoop() {
        // S13: the real-time budget of one block is `blockSize / sampleRate`
        // seconds; in integer microseconds it is blockSize * 1e6 / sampleRate.
        // Both terms are fixed after init(), so the division stays out of the
        // loop.
        const int64_t blockBudgetUs = (sampleRate > 0)
            ? ((int64_t)blockSize * 1000000LL / (int64_t)sampleRate)
            : 0;
        const size_t blockBytes = (size_t)blockSize * sizeof(int16_t);
        size_t bytesWritten = 0;
        uint32_t iteration = 0;

        while (true) {
            if (!engineInstance) {
                // Should not happen, but wait a bit to avoid watchdog
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
            }

            const int64_t iterationStartUs = esp_timer_get_time();

            // S12: render exactly one block, straight into the member buffer.
            engineInstance->generateSamples(renderBuffer_, blockSize);

            // Write to I2S (blocking if DMA is full).
            // portMAX_DELAY is deliberate and is the pacing mechanism (S13):
            // a timeout here would turn a late producer into dropped samples,
            // creating the very defect the underrun counter measures.
            bytesWritten = 0;
            const int64_t writeStartUs = esp_timer_get_time();
            const esp_err_t err = i2s_write(I2S_NUM_0, renderBuffer_, blockBytes,
                                           &bytesWritten, portMAX_DELAY);
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

            // i2s_write with portMAX_DELAY already yields via FreeRTOS
            // semaphore when DMA buffers are full. No explicit delay needed.
            if (isSingleCore) {
                taskYIELD();
            }
        }
    }

    void ESP32_I2S_AudioBackend::accountBlockTiming(int64_t iterationUs, int64_t writeUs,
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
                "[I2S] %u underruns: write blocked %lld us of a %lld us block "
                "(starved below %lld us), worst block %u us, worst write %u us (%d samples @ %d Hz)",
                (unsigned)underruns, (long long)writeUs, (long long)budgetUs,
                (long long)starvedWriteUs,
                (unsigned)worstBlockUs_.load(std::memory_order_relaxed),
                (unsigned)worstWriteUs_.load(std::memory_order_relaxed),
                blockSize, sampleRate);
        }
    }

}

#endif // PIXELROOT32_USE_I2S_AUDIO
