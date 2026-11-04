/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#ifdef PLATFORM_NATIVE

#include "audio/AudioConfig.h"
#include "audio/AudioScheduler.h"
#include "audio/ApuCore.h"

#include <atomic>
#include <thread>
#include <vector>

namespace pixelroot32::audio {

    /**
     * @class NativeAudioScheduler
     * @brief Audio scheduler for native builds.
     *
     * Inherits from AudioScheduler.
     *
     * Runs ApuCore in its own std::thread and double-buffers samples through
     * a lock-free ring, mirroring the dual-core ESP32 behaviour. All
     * synthesis / sequencer logic lives in ApuCore; this class owns only
     * threading and the ring buffer.
     *
     * The render block is configuration (S12): it is read once in init() from
     * the caller and rendered in a single `generateSamples` call into a member
     * buffer of `PR32_AUDIO_MAX_BLOCK_SAMPLES`, so the output is exactly what
     * one block-sized call per block produces.
     */
    class NativeAudioScheduler : public AudioScheduler {
    public:
        explicit NativeAudioScheduler(size_t ringBufferSize = 4096);
        ~NativeAudioScheduler() override;

        void init(AudioBackend* backend, int sampleRate,
                  const pixelroot32::platforms::PlatformCapabilities& caps, int blockSize = 512) override;
        void submitCommand(const AudioCommand& cmd) override;
        void start() override;
        void stop() override;
        bool isIndependent() const override { return true; }
        void generateSamples(int16_t* stream, int length) override;
        bool isMusicPlaying() const override { return apu.isMusicPlaying(); }
        bool isMusicPaused()  const override { return apu.isMusicPaused(); }
        ApuCore& getApuCore() override { return apu; }

        const ApuCore& core() const { return apu; }
        ApuCore& core() { return apu; }

    private:
        ApuCore apu;

        std::thread audioThread;
        std::atomic<bool> running{false};

        std::vector<int16_t> ringBuffer;
        std::atomic<size_t> rbReadPos{0};
        std::atomic<size_t> rbWritePos{0};
        size_t rbCapacity;
        /** Render block in samples, set once by init(); 0 means init() rejected it. */
        int blockSize = 512;
        /**
         * Render buffer (S12): a member, not the audio thread's stack. Sized for
         * the shared maximum block, which init() enforces.
         */
        int16_t renderBuffer_[PR32_AUDIO_MAX_BLOCK_SAMPLES];

        void threadLoop();

        size_t rbAvailableToRead() const;
        size_t rbAvailableToWrite() const;
        void rbWrite(const int16_t* data, size_t count);
        void rbRead(int16_t* data, size_t count);
    };

} // namespace pixelroot32::audio

#endif // PLATFORM_NATIVE
