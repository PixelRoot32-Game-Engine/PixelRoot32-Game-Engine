/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioConfig.h"
#include "AudioTypes.h"
#include "AudioScheduler.h"
#include "DefaultAudioScheduler.h"
#include "platforms/PlatformCapabilities.h"
#include <cstdint>
#include <memory>

namespace pixelroot32::audio {

    /**
     * @class AudioEngine
     * @brief Facade class for the NES-style audio subsystem.
     * 
     * Provides a high-level API for playing sound events, controlling volume,
     * and managing music playback. Internally delegates to an AudioScheduler
     * (typically DefaultAudioScheduler) which owns the ApuCore for synthesis.
     * 
     * Usage:
     * 1. Construct with an AudioConfig and PlatformCapabilities.
     * 2. Call init() to set up the scheduler and backend.
     * 3. Call playEvent() to trigger one-shot sounds.
     * 4. Use MusicPlayer for music track sequencing.
     */
    class AudioEngine {
    public:
        /**
         * @brief Constructs the AudioEngine.
         * @param config Audio configuration including backend pointer and sample rate.
         * @param caps Platform capabilities (e.g., FPU presence, core count).
         */
        AudioEngine(const AudioConfig& config, const pixelroot32::platforms::PlatformCapabilities& caps = pixelroot32::platforms::PlatformCapabilities());
        
        /** @brief Initializes the engine and its internal scheduler. */
        void init();

        /**
         * @brief Generates audio samples into the output buffer.
         * @param stream Output buffer (mono, int16 samples).
         * @param length Number of samples to generate.
         */
        void generateSamples(int16_t* stream, int length);

        /**
         * @brief Triggers a one-shot sound event.
         * @param event The event to play (type, frequency, duration, volume).
         */
        void playEvent(const AudioEvent& event);

        /**
         * @brief Sets the master volume for all audio output.
         * @param volume Volume level [0.0 = silent, 1.0 = full].
         */
        void setMasterVolume(float volume);
        /** @brief Gets the current master volume. @return Volume [0.0 - 1.0]. */
        float getMasterVolume() const;

        /**
         * @brief Sets the master bitcrusher effect on the final output.
         * @param bits Bit depth reduction [0 = off, 1-15 = re-quantize to N bits].
         */
        void setMasterBitcrush(uint8_t bits);
        /** @brief Gets the current master bitcrush setting. @return Bit depth (0 = off). */
        uint8_t getMasterBitcrush() const;

        /**
         * @brief Submits a raw audio command to the scheduler.
         * @param cmd The command to execute.
         */
        void submitCommand(const AudioCommand& cmd);

        /**
         * @brief Reports the real-time music transport state from the
         *        underlying scheduler/ApuCore (not a cached flag).
         *
         * This lets MusicPlayer and game code observe natural end-of-track
         * for non-looping music without polling private scheduler state.
         */
        bool isMusicPlaying() const;
        bool isMusicPaused() const;

        /**
         * @brief Replaces the scheduler with a custom implementation.
         * @param scheduler Unique pointer to the new scheduler. Takes ownership.
         */
        void setScheduler(std::unique_ptr<AudioScheduler> scheduler);

        /**
         * @brief Gets the current scheduler for diagnostics or profiling.
         * @return Pointer to the current AudioScheduler, or nullptr if not set.
         */
        AudioScheduler* getScheduler() const { return scheduler.get(); }

    private:
        AudioConfig config;
        pixelroot32::platforms::PlatformCapabilities capabilities;
        std::unique_ptr<AudioScheduler> scheduler;
        
        float masterVolume = 1.0f; // Cached for getMasterVolume
        uint8_t masterBitcrushBits = 0;
    };

}
