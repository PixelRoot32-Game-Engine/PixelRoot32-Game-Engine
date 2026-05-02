/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioScheduler.h"
#include "ApuCore.h"

namespace pixelroot32::audio {

    /**
     * @class DefaultAudioScheduler
     * @brief Backend-driven scheduler used on platforms without a dedicated
     *        audio task.
     *
     * Inherits from AudioScheduler.
     *
     * Delegates all synthesis to ApuCore; generateSamples() runs in whichever
     * context the backend invokes it (tests, simulators without a thread,
     * etc.).
     */
    class DefaultAudioScheduler : public AudioScheduler {
    public:
        DefaultAudioScheduler() : apu() {
            // Initialize with default sample rate to ensure ApuCore is properly set up
            // This prevents issues where init() is never called
            apu.init(44100);
        }

        void init(AudioBackend* backend, int sampleRate,
                  const pixelroot32::platforms::PlatformCapabilities& caps) override;
        void submitCommand(const AudioCommand& cmd) override;
        void start() override;
        void stop() override;
        bool isIndependent() const override;
        void generateSamples(int16_t* stream, int length) override;
        bool isMusicPlaying() const override { return apu.isMusicPlaying(); }
        bool isMusicPaused()  const override { return apu.isMusicPaused(); }
        ApuCore& getApuCore() override { return apu; }

        /** Exposes the underlying core for tests or higher-level queries. */
        const ApuCore& core() const { return apu; }
        ApuCore& core() { return apu; }

    private:
        ApuCore apu;
        bool running = false;
    };

} // namespace pixelroot32::audio
