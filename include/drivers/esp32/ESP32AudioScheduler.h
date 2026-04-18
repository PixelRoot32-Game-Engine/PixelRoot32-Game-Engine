/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#ifdef ESP32

#include "audio/AudioScheduler.h"
#include "audio/AudioCommandQueue.h"
#include "audio/AudioMusicTypes.h"
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace pixelroot32::audio {

    /**
     * @class ESP32AudioScheduler
     * @brief Audio mixer and sequencer for ESP32; runs in the backend audio task.
     *
     * The I2S/DAC backend creates the FreeRTOS task and calls generateSamples(); this
     * class does not spawn its own task. Constructor arguments are reserved for API
     * stability (see PlatformCapabilities used by the backend).
     */
    class ESP32AudioScheduler : public AudioScheduler {
    public:
        ESP32AudioScheduler(int reservedCoreId = 0, int reservedTaskPriority = configMAX_PRIORITIES - 1);
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
        int32_t masterVolumeScale = 65536; // Pre-computed for LUT path (Q16 fixed-point)
        uint64_t audioTimeSamples = 0;
        
        TaskHandle_t taskHandle = nullptr;
        volatile bool running = false;
        std::atomic<uint32_t> droppedCommands{0};

        // Music Sequencer State - NES-style tick synchronization
        static constexpr size_t MAX_MUSIC_TRACKS = 4;
        static constexpr float DEFAULT_BPM = 150.0f;  // Typical NES tempo
        static constexpr int TICKS_PER_BEAT = 4;        // 4 ticks per beat (quarter notes)
        
        const MusicTrack* tracks[MAX_MUSIC_TRACKS] = {nullptr, nullptr, nullptr, nullptr};
        size_t currentNoteIndices[MAX_MUSIC_TRACKS] = {0, 0, 0, 0};
        uint64_t nextNoteSamples[MAX_MUSIC_TRACKS] = {0, 0, 0, 0};
        
        // NES-style tick-based synchronization
        uint64_t globalTickCounter = 0;         // Global tick counter (synchronizes all tracks)
        uint64_t tickDurationSamples = 0;       // Samples per tick
        float tempoBPM = DEFAULT_BPM;            // Beats per minute
        size_t activeTrackCount = 0;
        float tempoFactor = 1.0f;                // Speed multiplier
        bool musicPlaying = false;
        bool musicPaused = false;

        // Sequencer limits to prevent CPU spikes
        static constexpr int MAX_NOTES_PER_FRAME = 8;
        uint32_t notesSkipped = 0;
        uint32_t maxNotesProcessed = 0;

        // Performance metrics (when profiling enabled)
        uint64_t totalGenerateTimeUs = 0;
        uint32_t generateSampleCount = 0;
        uint32_t maxGenerateTimeUs = 0;

        void processCommands();
        void executePlayEvent(const AudioEvent& event);
        void updateMusicSequencer(int length);
        void playCurrentNote();
        
        AudioChannel* findFreeChannel(WaveType type);
        float generateSampleForChannel(AudioChannel& ch);
    };

} // namespace pixelroot32::audio

#endif // ESP32
