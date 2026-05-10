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
     * @brief Backend-driven scheduler used on platforms without a dedicated audio task.
     *
     * Delegates all synthesis to ApuCore; generateSamples() runs in whichever
     * context the backend invokes it (tests, simulators without a thread, etc.).
     * Does not own a thread — audio generation is driven by the backend's callback.
     *
     * @note For platforms with a dedicated audio task (e.g., FreeRTOS on ESP32),
     *       use a scheduler that spawns its own thread instead.
     */
    class DefaultAudioScheduler : public AudioScheduler {
    public:
        /**
         * @brief Default constructor. Initializes ApuCore with default sample rate.
         * 
         * Ensures ApuCore is properly set up even if init() is never called explicitly.
         */
        DefaultAudioScheduler() : apu() {
            // Initialize with default sample rate to ensure ApuCore is properly set up
            // This prevents issues where init() is never called
            apu.init(44100);
        }

        void init(AudioBackend* backend, int sampleRate,
                  const pixelroot32::platforms::PlatformCapabilities& caps, int blockSize = 256) override;
        /** @brief Enqueues a command to the ApuCore. @param cmd The command to submit. */
        void submitCommand(const AudioCommand& cmd) override;
        /** @brief Marks scheduler as running. Starts audio generation context. */
        void start() override;
        /** @brief Marks scheduler as stopped. Silences all voices. */
        void stop() override;
        /** @brief Returns false (no dedicated audio thread). @return false. */
        bool isIndependent() const override;
        /** @brief Generates samples via ApuCore. @param stream Output buffer. @param length Sample count. */
        void generateSamples(int16_t* stream, int length) override;
        bool isMusicPlaying() const override { return apu.isMusicPlaying(); }
        bool isMusicPaused()  const override { return apu.isMusicPaused(); }
        /** @brief Returns reference to the ApuCore for diagnostics. @return ApuCore reference. */
        ApuCore& getApuCore() override { return apu; }

        /** @brief Exposes the underlying core for tests or higher-level queries. @return Const ApuCore reference. */
        const ApuCore& core() const { return apu; }
        /** @brief Exposes the underlying core for tests or higher-level queries. @return ApuCore reference. */
        ApuCore& core() { return apu; }

    private:
        ApuCore apu;     ///< The shared APU core handling all synthesis.
        bool running = false; ///< Whether the scheduler is active.
    };

} // namespace pixelroot32::audio
