/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioScheduler.h"
#include "AudioCommandQueue.h"
#include "AudioMusicTypes.h"
#include <atomic>

namespace pixelroot32::audio {

    /**
     * @class DefaultAudioScheduler
     * @brief Standard implementation of AudioScheduler.
     * 
     * This implementation processes commands and generates samples in the same
     * context (either the audio thread or the game loop).
     */
    class DefaultAudioScheduler : public AudioScheduler {
    public:
        DefaultAudioScheduler();

        void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps) override;
        void submitCommand(const AudioCommand& cmd) override;
        void start() override;
        void stop() override;
        bool isIndependent() const override;
        void generateSamples(int16_t* stream, int length) override;

    private:
        static constexpr int NUM_CHANNELS = 4;
        AudioChannel channels[NUM_CHANNELS];
        AudioCommandQueue commandQueue;
        
        int sampleRate = 44100;
        float masterVolume = 1.0f;
        int32_t masterVolumeScale = 65536; // Pre-computed for LUT path (Q16 fixed-point)
        uint64_t audioTimeSamples = 0;
        bool running = false;

        // Music Sequencer State - NES-style tick synchronization
        static constexpr size_t MAX_MUSIC_TRACKS = 4;
        static constexpr float DEFAULT_BPM = 150.0f;  // Typical NES tempo
        static constexpr int TICKS_PER_BEAT = 4;        // 4 ticks per beat (quarter notes)
        
        const MusicTrack* tracks[MAX_MUSIC_TRACKS] = {nullptr, nullptr, nullptr, nullptr};
        size_t currentNoteIndices[MAX_MUSIC_TRACKS] = {0, 0, 0, 0};
        uint64_t nextNoteSamples[MAX_MUSIC_TRACKS] = {0, 0, 0, 0};
        
        // NES-style tick-based synchronization
        uint64_t globalTickCounter = 0;         // Global tick counter (synchronizes all tracks)
        uint64_t tickDurationSamples = 0;       // Samples per tick = sampleRate / (BPM * ticksPerBeat / 60)
        float tempoBPM = DEFAULT_BPM;            // Beats per minute
        size_t activeTrackCount = 0;
        float tempoFactor = 1.0f;                // Speed multiplier (1.0 = normal, 2.0 = double speed)
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
