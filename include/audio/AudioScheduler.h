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
         * @param backend The audio backend to use for output.
         * @param sampleRate The output sample rate in Hz.
         * @param caps Platform capabilities to guide core pinning or threading decisions.
         * @param blockSize Audio block size in samples for I2S DMA and ring buffer operations.
         *        Must be a multiple of 128.
         */
        virtual void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps = pixelroot32::platforms::PlatformCapabilities(), int blockSize = 256) = 0;

        /**
         * @brief Submits a command to the scheduler for execution.
         * @param cmd The command to enqueue (PLAY_EVENT, STOP_CHANNEL, etc.).
         */
        virtual void submitCommand(const AudioCommand& cmd) = 0;

        /** @brief Starts the scheduler execution. Enables audio generation. */
        virtual void start() = 0;

        /** @brief Stops the scheduler execution. Silences all voices. */
        virtual void stop() = 0;

        /**
         * @brief Checks if the scheduler runs in an independent thread.
         * @return true if the scheduler owns a dedicated audio thread, false if it
         *        runs synchronously with the backend's callback.
         */
        virtual bool isIndependent() const = 0;

        /**
         * @brief Generates samples into the provided buffer.
         * @param stream Pointer to the output buffer (mono, int16 samples).
         * @param length Number of samples to generate (must match blockSize).
         * 
         * Called by the backend (or scheduler thread) to fill the audio buffer.
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
