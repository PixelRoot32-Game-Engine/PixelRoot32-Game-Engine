/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioTypes.h"
#include "AudioBackend.h"
#include "ApuCore.h"
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
         * @param blockSize Audio block size (samples) for I2S DMA and ring buffer operations.
         */
        virtual void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps = pixelroot32::platforms::PlatformCapabilities(), int blockSize = 256) = 0;

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

        /**
         * @brief Reports whether a music track is currently being sequenced.
         *
         * Implementations back this with the underlying ApuCore atomic flag
         * so callers (e.g. MusicPlayer) observe the *actual* audio-thread
         * state, including natural end-of-track for non-looping tracks.
         * Default returns false so legacy/mock schedulers don't need to opt in.
         */
        virtual bool isMusicPlaying() const { return false; }

        /**
         * @brief Reports whether the music sequencer is paused.
         */
        virtual bool isMusicPaused() const { return false; }

        /**
         * @brief Gets reference to the underlying ApuCore for diagnostics/profiling.
         */
        virtual ApuCore& getApuCore() = 0;
    };

} // namespace pixelroot32::audio
