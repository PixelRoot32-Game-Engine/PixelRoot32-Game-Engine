/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "audio/AudioScheduler.h"
#include <vector>

namespace pixelroot32::audio {

/**
 * @class MockAudioScheduler
 * @brief A mock implementation of AudioScheduler for testing.
 */
class MockAudioScheduler : public AudioScheduler {
public:
    std::vector<AudioCommand> submittedCommands;
    bool started = false;
    bool stopped = false;
    bool initialized = false;
    AudioBackend* backend = nullptr;
    int sampleRate = 0;

    void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps) override {
        this->backend = backend;
        this->sampleRate = sampleRate;
        this->initialized = true;
    }

    void submitCommand(const AudioCommand& cmd) override {
        submittedCommands.push_back(cmd);
    }

    void start() override {
        started = true;
    }

    void stop() override {
        stopped = true;
    }

    bool isIndependent() const override {
        return false;
    }

    void generateSamples(int16_t* stream, int length) override {
        // Fill with silence by default
        for (int i = 0; i < length; ++i) {
            stream[i] = 0;
        }
    }
    
    // Helper to find a command by type
    bool hasCommand(AudioCommandType type) const {
        for (const auto& cmd : submittedCommands) {
            if (cmd.type == type) return true;
        }
        return false;
    }
};

} // namespace pixelroot32::audio
