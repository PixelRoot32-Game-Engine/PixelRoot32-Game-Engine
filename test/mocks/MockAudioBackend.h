/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "audio/AudioBackend.h"

namespace pixelroot32::audio {

/**
 * @class MockAudioBackend
 * @brief A mock implementation of AudioBackend for testing.
 */
class MockAudioBackend : public AudioBackend {
public:
    int sampleRate = 22050;
    AudioEngine* engine = nullptr;
    bool initialized = false;

    void init(AudioEngine* engine, const pixelroot32::core::PlatformCapabilities& caps) override {
        this->engine = engine;
        this->initialized = true;
    }

    int getSampleRate() const override {
        return sampleRate;
    }
};

} // namespace pixelroot32::audio
