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
        int blockSize = platforms::config::HasFPU ? 256 : 128; ///< Audio block size (samples). Must be multiple of 128.
        PostMixMonoFn postMixMono = nullptr;
        void* postMixUser = nullptr;

        /**
         * @brief Constructs an AudioConfig with platform-adaptive block size.
         * @param backend Pointer to the audio backend implementation. May be nullptr for headless configs.
         * @param sampleRate Desired sample rate in Hz (default 22050 for retro feel).
         * @param blockSize Audio block size in samples. Defaults to 256 on FPU platforms, 128 on no-FPU platforms.
         *        Must be a multiple of 128 for I2S DMA alignment.
         */
        AudioConfig(AudioBackend* backend = nullptr, int sampleRate = 22050,
                  int blockSize = platforms::config::HasFPU ? 256 : 128)
            : backend(backend), sampleRate(sampleRate), blockSize(blockSize) {}
    };

    // Validate blockSize at compile time using platform-specific constexpr
    // Both 128 (no-FPU) and 256 (FPU) are multiples of 128, so this is always true.
    // The assertion exists to catch future misconfigurations.
    static_assert((platforms::config::HasFPU ? 256 : 128) % 128 == 0,
                 "AudioConfig::blockSize must be multiple of 128 for I2S alignment");

} // namespace pixelroot32::audio
