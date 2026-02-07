/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/PlatformCapabilities.h"

namespace pixelroot32::audio {

    class AudioEngine;

    /**
     * @class AudioBackend
     * @brief Abstract interface for platform-specific audio drivers.
     * 
     * This class abstracts the underlying audio hardware or API (e.g., SDL2, I2S).
     * It is responsible for requesting audio samples from the AudioEngine and
     * pushing them to the output device.
     */
    class AudioBackend {
    public:
        virtual ~AudioBackend() = default;

        /**
         * @brief Initializes the audio backend.
         * @param engine Pointer to the AudioEngine instance to request samples from.
         * @param caps Platform capabilities to guide backend initialization (e.g., core pinning).
         */
        virtual void init(AudioEngine* engine, const pixelroot32::core::PlatformCapabilities& caps = pixelroot32::core::PlatformCapabilities()) = 0;

        /**
         * @brief Returns the configured sample rate of the backend.
         * @return Sample rate in Hz (e.g., 22050, 44100).
         */
        virtual int getSampleRate() const = 0;
    };

}
