/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(PIXELROOT32_USE_DAC_AUDIO)

#include "audio/AudioBackend.h"
#include "audio/AudioConfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/dac.h>
#include <driver/i2s.h>
#include <atomic>
#include <cstdint>

namespace pixelroot32::drivers::esp32 {

    /**
     * @class ESP32_DAC_AudioBackend
     * @brief Audio backend for ESP32 classic / S2 internal 8-bit DAC.
     *
     * Inherits from AudioBackend.
     *
     * Uses **I2S in DAC-built-in mode** so samples are pushed to the DAC via
     * DMA instead of the previous per-sample `dacWrite()` spin loop. This
     * frees ~15-25% of CPU on Core 0 at 22050 Hz and removes the per-sample
     * pinmux/mutex overhead that `dacWrite()` incurs on every call.
     *
     * Target: ESP32 (original), ESP32-S2. The DAC does not exist on S3/C3.
     *
     * Like the I2S backend, the render block is configuration (S12): it is read
     * once in init() from AudioEngine::getBlockSize() and drives the DMA depth,
     * the per-iteration render length and the underrun budget (S13).
     */
    class ESP32_DAC_AudioBackend : public pixelroot32::audio::AudioBackend {
    public:
        /**
         * @param dacPin GPIO (must be 25 for DAC1 or 26 for DAC2).
         * @param sampleRate Audio sample rate (default 22050).
         */
        ESP32_DAC_AudioBackend(int dacPin = 25, int sampleRate = 22050);
        virtual ~ESP32_DAC_AudioBackend();

        void init(pixelroot32::audio::AudioEngine* engine,
                  const pixelroot32::platforms::PlatformCapabilities& caps) override;
        int getSampleRate() const override { return sampleRate; }

        void audioTaskLoop();

        /**
         * @brief Blocks where `i2s_write` had almost nothing to wait for
         *        (less than a tenth of the block budget, so the DMA ring had
         *        already drained), or where the write failed or came back short.
         *
         * Read from any thread; the audio task owns the writes.
         */
        uint32_t getUnderrunCount() const { return underrunCount_.load(std::memory_order_relaxed); }

        /**
         * @brief Worst total iteration (render + DAC conversion + write)
         *        duration, in µs.
         *
         * Over every block, underrun or not: it says how close the producer
         * ran to its deadline, which the count alone cannot.
         */
        uint32_t getWorstBlockUs() const { return worstBlockUs_.load(std::memory_order_relaxed); }

        /**
         * @brief Worst `i2s_write` blocking time seen, in µs.
         *
         * A healthy ring makes the write wait most of a block; a value near
         * zero over many blocks is the starvation the counter reports.
         */
        uint32_t getWorstWriteUs() const { return worstWriteUs_.load(std::memory_order_relaxed); }

    private:
        /** S12: fixed `dma_buf_len`; `blockSize` must be a multiple of it. */
        static constexpr int kDmaBufLenSamples = 128;
        /** S13: the first iterations fill the ring, so they are not counted. */
        static constexpr uint32_t kUnderrunWarmupIterations = 10;

        int dacPin;
        int sampleRate;
        dac_channel_t dacChannel;
        i2s_dac_mode_t dacMode = I2S_DAC_CHANNEL_DISABLE;
        pixelroot32::audio::AudioEngine* engineInstance = nullptr;
        TaskHandle_t audioTaskHandle = nullptr;
        bool i2sInstalled = false;
        bool isSingleCore = false;
        /** Render block in samples, read once in init() from the engine (S12). */
        int blockSize = 512;
        /**
         * Render buffer (S12): a member, never the audio task stack. The 8-bit
         * DAC tap converts signed PCM to offset-binary **in place**, so one
         * buffer covers both stages and the object grows by 2 KB, not 4 KB.
         */
        int16_t renderBuffer_[pixelroot32::audio::PR32_AUDIO_MAX_BLOCK_SAMPLES];
        /** Blocks whose write did not have to wait, or wrote short (S13). */
        std::atomic<uint32_t> underrunCount_{0};
        /** Worst total iteration duration in microseconds, over every block. */
        std::atomic<uint32_t> worstBlockUs_{0};
        /** Worst `i2s_write` blocking time in microseconds, over every block. */
        std::atomic<uint32_t> worstWriteUs_{0};

        /**
         * @brief Accounts one iteration: updates both worst cases and, once the
         *        ring is primed, the underrun count.
         *
         * Audio thread only. Logs one line every 100 underruns.
         */
        void accountBlockTiming(int64_t iterationUs, int64_t writeUs, int64_t budgetUs,
                                bool writeComplete, bool countUnderruns);
    };

}

#endif // ARDUINO_ARCH_ESP32 && PIXELROOT32_USE_DAC_AUDIO
