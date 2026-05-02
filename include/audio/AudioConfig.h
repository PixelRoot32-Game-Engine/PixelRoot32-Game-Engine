/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioBackend.h"
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
        PostMixMonoFn postMixMono = nullptr;
        void* postMixUser = nullptr;

        /**
         * @brief Default constructor.
         * @param backend Pointer to the audio backend implementation.
         * @param sampleRate Desired sample rate (default 22050Hz for retro feel).
         */
        AudioConfig(AudioBackend* backend = nullptr, int sampleRate = 22050)
            : backend(backend), sampleRate(sampleRate) {}
    };

}
