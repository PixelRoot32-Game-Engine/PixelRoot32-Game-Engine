/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once

#include "AudioBackend.h"

namespace pixelroot32::audio {

    /**
     * @struct AudioConfig
     * @brief Configuration for the Audio subsystem.
     */
    struct AudioConfig {
        AudioBackend* backend = nullptr; ///< Pointer to the platform-specific audio backend.
        int sampleRate = 22050;          ///< Desired sample rate in Hz.

        /**
         * @brief Default constructor.
         * @param backend Pointer to the audio backend implementation.
         * @param sampleRate Desired sample rate (default 22050Hz for retro feel).
         */
        AudioConfig(AudioBackend* backend = nullptr, int sampleRate = 22050)
            : backend(backend), sampleRate(sampleRate) {}
    };

}
