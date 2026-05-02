/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "audio/AudioBackend.h"
#include "audio/AudioEngine.h"
#include <cstdint>
#include <cstring>

namespace pixelroot32::audio {

    /**
     * @class MockAudioBackend
     * @brief Mock implementation of AudioBackend for unit testing.
     *
     * Inherits from AudioBackend.
     *
     * This mock captures initialization state and provides test hooks
     * for verifying AudioScheduler and AudioEngine interactions.
     */
    class MockAudioBackend : public AudioBackend {
    public:
        MockAudioBackend(int sampleRate = 22050) : sampleRate_(sampleRate) {}
        
        virtual ~MockAudioBackend() = default;

        void init(AudioEngine* engine, const platforms::PlatformCapabilities& caps) override {
            engineInstance = engine;
            capsInstance = caps;
            initCalled = true;
        }

        int getSampleRate() const override {
            return sampleRate_;
        }

        // Test inspection methods
        bool wasInitCalled() const { return initCalled; }
        AudioEngine* getEngine() const { return engineInstance; }
        const platforms::PlatformCapabilities& getCaps() const { return capsInstance; }
        
        // Test control methods
        void reset() {
            initCalled = false;
            engineInstance = nullptr;
        }

    private:
        int sampleRate_;
        bool initCalled = false;
        AudioEngine* engineInstance = nullptr;
        platforms::PlatformCapabilities capsInstance;
    };

} // namespace pixelroot32::audio
