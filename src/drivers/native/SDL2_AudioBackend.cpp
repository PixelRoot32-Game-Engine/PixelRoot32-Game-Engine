/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef PLATFORM_NATIVE

#include "drivers/native/SDL2_AudioBackend.h"
#include "audio/AudioEngine.h"
#include <iostream>

namespace pixelroot32::drivers::native {

    // Global wrapper for C-style callback
    static void SDLAudioCallbackWrapper(void* userdata, uint8_t* stream, int len) {
        auto* backend = static_cast<SDL2_AudioBackend*>(userdata);
        if (backend) {
            backend->audioCallback(stream, len);
        }
    }

    SDL2_AudioBackend::SDL2_AudioBackend(int sampleRate, int bufferSize)
        : sampleRate(sampleRate), bufferSize(bufferSize), deviceId(0) {}

    SDL2_AudioBackend::~SDL2_AudioBackend() {
        if (deviceId != 0) {
            SDL_CloseAudioDevice(deviceId);
        }
    }

    void SDL2_AudioBackend::init(pixelroot32::audio::AudioEngine* engine, const pixelroot32::core::PlatformCapabilities& /*caps*/) {
        this->engineInstance = engine;

        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::cerr << "SDL Audio initialization failed: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = sampleRate;
        want.format = AUDIO_S16SYS; // Signed 16-bit native endian
        want.channels = 1;          // Mono
        want.samples = bufferSize;  // Buffer size in samples
        want.callback = SDLAudioCallbackWrapper;
        want.userdata = this;

        deviceId = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0); // 0 = no changes allowed

        if (deviceId == 0) {
            std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        } else {
            std::cout << "Audio device opened: " << have.freq << "Hz, " << (int)have.channels << "ch" << std::endl;
            // Start playback
            SDL_PauseAudioDevice(deviceId, 0);
        }
    }

    void SDL2_AudioBackend::audioCallback(uint8_t* stream, int len) {
        if (!engineInstance) {
            SDL_memset(stream, 0, len);
            return;
        }

        // len is in bytes, but we generate 16-bit samples
        int samplesRequested = len / sizeof(int16_t);
        
        // Let the engine fill the buffer directly
        engineInstance->generateSamples(reinterpret_cast<int16_t*>(stream), samplesRequested);
    }

}

#endif // PLATFORM_NATIVE
