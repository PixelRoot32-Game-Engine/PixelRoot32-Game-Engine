/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioTypes.h"
#include "AudioBackend.h"
#include "platforms/PlatformCapabilities.h"

namespace pixelroot32::audio {

    /**
     * @class AudioScheduler
     * @brief Abstract interface for the audio execution context.
     * 
     * The scheduler is responsible for owning the audio state, processing commands,
     * and generating samples. It can run in the same thread as the game loop
     * or in a dedicated audio thread.
     */
    class AudioScheduler {
    public:
        virtual ~AudioScheduler() = default;

        /**
         * @brief Initializes the scheduler.
         * @param backend The audio backend to use.
         * @param sampleRate The output sample rate.
         * @param caps Platform capabilities to guide core pinning or threading.
         */
        virtual void init(AudioBackend* backend, int sampleRate, const pixelroot32::core::PlatformCapabilities& caps = pixelroot32::core::PlatformCapabilities()) = 0;

        /**
         * @brief Submits a command to the scheduler.
         * @param cmd The command to execute.
         */
        virtual void submitCommand(const AudioCommand& cmd) = 0;

        /**
         * @brief Starts the scheduler execution.
         */
        virtual void start() = 0;

        /**
         * @brief Stops the scheduler execution.
         */
        virtual void stop() = 0;

        /**
         * @brief Checks if the scheduler runs in an independent thread.
         */
        virtual bool isIndependent() const = 0;

        /**
         * @brief Generates samples. Should be called by the backend or scheduler thread.
         */
        virtual void generateSamples(int16_t* stream, int length) = 0;
    };

} // namespace pixelroot32::audio
