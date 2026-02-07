/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#ifdef PLATFORM_NATIVE

#include "audio/AudioScheduler.h"
#include "audio/AudioCommandQueue.h"
#include "audio/AudioMusicTypes.h"
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace pixelroot32::audio {

    /**
     * @class NativeAudioScheduler
     * @brief Audio scheduler for native platforms using a dedicated thread.
     * 
     * This mimics the multi-core behavior of the ESP32 by running audio generation
     * in a separate std::thread and communicating via a ring buffer.
     */
    class NativeAudioScheduler : public AudioScheduler {
    public:
        NativeAudioScheduler(size_t ringBufferSize = 4096);
        virtual ~NativeAudioScheduler();

        void init(AudioBackend* backend, int sampleRate) override;
        void submitCommand(const AudioCommand& cmd) override;
        void start() override;
        void stop() override;
        bool isIndependent() const override { return true; }
        void generateSamples(int16_t* stream, int length) override;

    private:
        static constexpr int NUM_CHANNELS = 4;
        AudioChannel channels[NUM_CHANNELS];
        AudioCommandQueue commandQueue;
        
        int sampleRate = 44100;
        float masterVolume = 1.0f;
        uint64_t audioTimeSamples = 0;
        
        std::thread audioThread;
        std::atomic<bool> running{false};
        
        // Sample Ring Buffer (Thread-safe SPSC)
        std::vector<int16_t> ringBuffer;
        std::atomic<size_t> rbReadPos{0};
        std::atomic<size_t> rbWritePos{0};
        size_t rbCapacity;

        // Music Sequencer State
        const MusicTrack* currentTrack = nullptr;
        size_t currentNoteIndex = 0;
        uint64_t nextNoteSample = 0;
        float tempoFactor = 1.0f;
        bool musicPlaying = false;
        bool musicPaused = false;

        void threadLoop();
        void processCommands();
        void executePlayEvent(const AudioEvent& event);
        void updateMusicSequencer(int length);
        void playCurrentNote();
        
        AudioChannel* findFreeChannel(WaveType type);
        int16_t generateSampleForChannel(AudioChannel& ch);

        // Ring buffer helpers
        size_t rbAvailableToRead() const;
        size_t rbAvailableToWrite() const;
        void rbWrite(const int16_t* data, size_t count);
        void rbRead(int16_t* data, size_t count);
    };

} // namespace pixelroot32::audio

#endif // PLATFORM_NATIVE
