/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "platforms/PlatformCapabilities.h"

namespace pixelroot32::audio {

    class AudioEngine;

/**
      * @class AudioBackend
      * @brief Abstract interface for platform-specific audio drivers.
      * 
      * This class abstracts the underlying audio hardware or API (e.g., SDL2 on native,
      * I2S on ESP32). It is responsible for requesting audio samples from the AudioEngine
      * and pushing them to the output device. Platforms implement this interface to
      * bridge between the engine's sample generation and the actual hardware.
      * 
      * Typical lifecycle:
      * 1. init() is called with an AudioEngine pointer.
      * 2. The backend sets up the audio output (I2S, SDL audio, etc.).
      * 3. In its output callback, the backend calls engine->generateSamples().
      * 4. On destruction, resources are cleaned up.
      */
    class AudioBackend {
    public:
        /** @brief Virtual destructor for proper cleanup in derived classes. */
        virtual ~AudioBackend() = default;

        /**
         * @brief Initializes the audio backend.
         * @param engine Pointer to the AudioEngine instance to request samples from.
         *        Must not be nullptr.
         * @param caps Platform capabilities to guide backend initialization
         *        (e.g., core pinning on ESP32, thread priorities).
         */
        virtual void init(AudioEngine* engine, const pixelroot32::platforms::PlatformCapabilities& caps = pixelroot32::platforms::PlatformCapabilities()) = 0;

        /**
         * @brief Returns the configured sample rate of the backend.
         * @return Sample rate in Hz (e.g., 22050, 44100, 48000).
         */
        virtual int getSampleRate() const = 0;
    };

}
