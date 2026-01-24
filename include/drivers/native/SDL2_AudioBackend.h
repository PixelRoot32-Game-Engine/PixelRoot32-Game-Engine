/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#ifdef PLATFORM_NATIVE

#include "audio/AudioBackend.h"
#include <SDL2/SDL.h>

namespace pixelroot32::drivers::native {

    /**
     * @class SDL2_AudioBackend
     * @brief Audio backend implementation for SDL2 (Windows/Linux/Mac).
     */
    class SDL2_AudioBackend : public pixelroot32::audio::AudioBackend {
    public:
        SDL2_AudioBackend(int sampleRate = 22050, int bufferSize = 1024);
        virtual ~SDL2_AudioBackend();

        void init(pixelroot32::audio::AudioEngine* engine) override;
        int getSampleRate() const override { return sampleRate; }

        // Internal callback for SDL
        void audioCallback(uint8_t* stream, int len);

    private:
        int sampleRate;
        int bufferSize;
        SDL_AudioDeviceID deviceId;
        pixelroot32::audio::AudioEngine* engineInstance = nullptr;
    };

}

#endif // PLATFORM_NATIVE
