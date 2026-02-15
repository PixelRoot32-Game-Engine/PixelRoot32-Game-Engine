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
        AudioEngine(const AudioConfig& config, const pixelroot32::core::PlatformCapabilities& caps = pixelroot32::core::PlatformCapabilities());
        
        void init();

        void generateSamples(int16_t* stream, int length);

        void playEvent(const AudioEvent& event);

        void setMasterVolume(float volume);
        float getMasterVolume() const;

        void submitCommand(const AudioCommand& cmd);

        /**
         * @brief Sets a custom scheduler.
         * @param scheduler The scheduler to use.
         */
        void setScheduler(std::unique_ptr<AudioScheduler> scheduler);

    private:
        AudioConfig config;
        pixelroot32::core::PlatformCapabilities capabilities;
        std::unique_ptr<AudioScheduler> scheduler;
        
        float masterVolume = 1.0f; // Cached for getMasterVolume
    };

}
