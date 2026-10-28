/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioBackend.h"
#include "platforms/EngineConfig.h"
#include <cstdint>

namespace pixelroot32::audio {

    /**
     * Maximum render block in samples (S12). Render buffers are fixed-size
     * members, so a `blockSize` above this — or not a multiple of 128 — is
     * rejected by the backend/scheduler `init()` instead of overflowing them.
     */
    inline constexpr int PR32_AUDIO_MAX_BLOCK_SAMPLES = 1024;

    /** Optional RT-safe post-mix processing on the final mono int16 buffer (after bitcrush). */
    using PostMixMonoFn = void (*)(int16_t* mono, int length, void* user);

    /**
     * @struct AudioConfig
     * @brief Configuration for the Audio subsystem.
     */
    struct AudioConfig {
        AudioBackend* backend = nullptr; ///< Pointer to the platform-specific audio backend.
        /**
         * Desired sample rate in Hz.
         *
         * 22 050 is the only rate that is supported on every target. 32 000 is
         * available where p99 stays under half the render block, and the APU's
         * own measurements decide that per board: on `esp32-c3-devkitm-1` the
         * `Nes` and `Hybrid` profiles measure 50.1 % and 50.9 % of the block at
         * eight voices and 32 000 Hz, so that rate is unsupported for those
         * profiles on that target, while `Legacy` keeps it at 48.7 %. The APU
         * renders at whatever rate it is given and has no per-target policy:
         * this is the envelope, and the consumer enforces it.
         */
        int sampleRate = 22050;
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
