/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioBackend.h"
#include "platforms/EngineConfig.h"
#include <cstdint>

namespace pixelroot32::audio {

    /** Optional RT-safe post-mix processing on the final mono int16 buffer (after bitcrush). */
    using PostMixMonoFn = void (*)(int16_t* mono, int length, void* user);

    /**
     * @struct AudioConfig
     * @brief Configuration for the Audio subsystem.
     */
    struct AudioConfig {
        AudioBackend* backend = nullptr; ///< Pointer to the platform-specific audio backend.
        int sampleRate = 22050;          ///< Desired sample rate in Hz.
        /// Audio block size (samples): the render unit, multiple of 128. The I2S backend derives its DMA depth from it.
        int blockSize = platforms::config::HasFPU ? 512 : 1024;
        PostMixMonoFn postMixMono = nullptr;
        void* postMixUser = nullptr;

        /**
         * @brief Constructs an AudioConfig with platform-adaptive block size.
         * @param backend Pointer to the audio backend implementation. May be nullptr for headless configs.
         * @param sampleRate Desired sample rate in Hz (default 22050 for retro feel).
         * @param blockSize Audio block size in samples. Defaults to 512 on FPU platforms, 1024 on no-FPU platforms.
         *        Must be a multiple of 128: the block is the render unit, and the I2S backend derives
         *        its DMA depth from it (dma_buf_len = 128, dma_buf_count = clamp(blockSize / 128, 2, 8)).
         */
        AudioConfig(AudioBackend* backend = nullptr, int sampleRate = 22050,
                  int blockSize = platforms::config::HasFPU ? 512 : 1024)
            : backend(backend), sampleRate(sampleRate), blockSize(blockSize) {}
    };

    // Validate the default block sizes at compile time using platform-specific constexpr.
    // Both 1024 (no-FPU) and 512 (FPU) are multiples of 128, so this is always true today.
    // The assertion exists to catch future misconfigurations: the block is the render unit and
    // the I2S backend derives its DMA depth from it, so a non-multiple would break that mapping.
    static_assert((platforms::config::HasFPU ? 512 : 1024) % 128 == 0,
                 "AudioConfig::blockSize render unit must be a multiple of 128: the I2S backend derives its DMA depth from it");

} // namespace pixelroot32::audio
