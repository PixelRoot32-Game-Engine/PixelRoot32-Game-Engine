/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_USE_I2S_AUDIO)

#include "audio/AudioBackend.h"
#include "audio/AudioConfig.h"
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include <cstdint>

namespace pixelroot32::drivers::esp32 {

    /**
     * @class ESP32_I2S_AudioBackend
     * @brief Audio backend implementation for ESP32 using I2S.
     *
     * Inherits from AudioBackend.
     *
     * Uses a FreeRTOS task to continuously feed the I2S DMA buffer
     * to ensure smooth playback independent of the game loop frame rate.
     *
     * The render block is configuration, not a backend constant (S12): it is
     * read once in init() from AudioEngine::getBlockSize() and drives the DMA
     * depth (`dma_buf_len` / `dma_buf_count`), the per-iteration render length
     * and the real-time budget of the underrun counter (S13).
     */
    class ESP32_I2S_AudioBackend : public pixelroot32::audio::AudioBackend {
    public:
        /**
         * @brief Construct a new ESP32_I2S_AudioBackend
         * 
         * @param bclkPin Bit Clock pin (BCLK)
         * @param wclkPin Word Clock / LR Clock pin (LRCK/WS)
         * @param doutPin Data Out pin (DIN/DOUT)
         * @param sampleRate Audio sample rate (default 22050)
         */
        ESP32_I2S_AudioBackend(int bclkPin, int wclkPin, int doutPin, int sampleRate = 22050);
        virtual ~ESP32_I2S_AudioBackend();

        void init(pixelroot32::audio::AudioEngine* engine, const pixelroot32::platforms::PlatformCapabilities& caps) override;
        int getSampleRate() const override { return sampleRate; }

        // Internal task function
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
         * @brief Worst total iteration (render + write) duration, in µs.
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

        int bclkPin;
        int wclkPin;
        int doutPin;
        int sampleRate;
        pixelroot32::audio::AudioEngine* engineInstance = nullptr;
        TaskHandle_t audioTaskHandle = nullptr;
        bool isSingleCore = false;
        /** Render block in samples, read once in init() from the engine (S12). */
        int blockSize = 512;
        /**
         * Render buffer (S12): a member, never the audio task stack. The task
         * stack is 4096 B; a 1 KB block would take a quarter of it.
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

#endif // PIXELROOT32_USE_I2S_AUDIO
