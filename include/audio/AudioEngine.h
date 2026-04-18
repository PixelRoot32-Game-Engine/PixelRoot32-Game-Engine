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
     * @brief Core class for the NES-like audio subsystem.
     * 
     * In Phase 2, this class becomes a facade for the AudioScheduler.
     */
    class AudioEngine {
    public:
        AudioEngine(const AudioConfig& config, const pixelroot32::platforms::PlatformCapabilities& caps = pixelroot32::platforms::PlatformCapabilities());
        
        void init();

        void generateSamples(int16_t* stream, int length);

        void playEvent(const AudioEvent& event);

        void setMasterVolume(float volume);
        float getMasterVolume() const;

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
         * @brief Sets a custom scheduler.
         * @param scheduler The scheduler to use.
         */
        void setScheduler(std::unique_ptr<AudioScheduler> scheduler);

    private:
        AudioConfig config;
        pixelroot32::platforms::PlatformCapabilities capabilities;
        std::unique_ptr<AudioScheduler> scheduler;
        
        float masterVolume = 1.0f; // Cached for getMasterVolume
    };

}
