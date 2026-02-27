/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#ifdef ESP32

#include "audio/AudioScheduler.h"
#include "audio/AudioCommandQueue.h"
#include "audio/AudioMusicTypes.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace pixelroot32::audio {

    /**
     * @class ESP32AudioScheduler
     * @brief Audio scheduler pinned to a specific core on ESP32.
     * 
     * Uses a FreeRTOS task to process commands and generate samples.
     */
    class ESP32AudioScheduler : public AudioScheduler {
    public:
        ESP32AudioScheduler(int coreId = 0, int priority = configMAX_PRIORITIES - 1);
        ~ESP32AudioScheduler();

        void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps) override;
        void submitCommand(const AudioCommand& cmd) override;
        void start() override;
        void stop() override;
        bool isIndependent() const override { return true; }
        void generateSamples(int16_t* stream, int length) override;

    private:
        static constexpr int NUM_CHANNELS = 4;
        AudioChannel channels[NUM_CHANNELS];
        AudioCommandQueue commandQueue;
        
        AudioBackend* backend = nullptr;
        int sampleRate = 44100;
        float masterVolume = 1.0f;
        uint64_t audioTimeSamples = 0;
        
        int coreId;
        int priority;
        TaskHandle_t taskHandle = nullptr;
        volatile bool running = false;

        // Music Sequencer State
        const MusicTrack* currentTrack = nullptr;
        size_t currentNoteIndex = 0;
        uint64_t nextNoteSample = 0;
        float tempoFactor = 1.0f;
        bool musicPlaying = false;
        bool musicPaused = false;

        static void taskWrapper(void* arg);
        void run();

        void processCommands();
        void executePlayEvent(const AudioEvent& event);
        void updateMusicSequencer(int length);
        void playCurrentNote();
        
        AudioChannel* findFreeChannel(WaveType type);
        float generateSampleForChannel(AudioChannel& ch);
    };

} // namespace pixelroot32::audio

#endif // ESP32
