/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/AudioEngine.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace pixelroot32::audio {

    AudioEngine::AudioEngine(const AudioConfig& config) 
        : config(config), scheduler(std::make_unique<DefaultAudioScheduler>()) {
    }

    void AudioEngine::init() {
        if (scheduler) {
            scheduler->init(config.backend, config.sampleRate);
            scheduler->start();
        }
        if (config.backend) {
            config.backend->init(this);
        }
    }

    void AudioEngine::update(unsigned long deltaTime) {
        // In Phase 2, this is deprecated. 
        // Timing is handled by the scheduler in generateSamples.
        (void)deltaTime;
    }

    void AudioEngine::generateSamples(int16_t* stream, int length) {
        if (scheduler) {
            scheduler->generateSamples(stream, length);
        }
    }

    void AudioEngine::playEvent(const AudioEvent& event) {
        AudioCommand cmd;
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event = event;
        submitCommand(cmd);
    }

    void AudioEngine::setMasterVolume(float volume) {
        masterVolume = std::clamp(volume, 0.0f, 1.0f);
        AudioCommand cmd;
        cmd.type = AudioCommandType::SET_MASTER_VOLUME;
        cmd.volume = masterVolume;
        submitCommand(cmd);
    }

    float AudioEngine::getMasterVolume() const {
        return masterVolume;
    }

    void AudioEngine::submitCommand(const AudioCommand& cmd) {
        if (scheduler) {
            scheduler->submitCommand(cmd);
        }
    }

    void AudioEngine::setScheduler(std::unique_ptr<AudioScheduler> newScheduler) {
        if (newScheduler) {
            newScheduler->init(config.backend, config.sampleRate);
            newScheduler->start();
            scheduler = std::move(newScheduler);
        }
    }

}
