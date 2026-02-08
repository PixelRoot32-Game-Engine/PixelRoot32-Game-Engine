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

        void init(AudioBackend* backend, int sampleRate, const pixelroot32::core::PlatformCapabilities& caps) override;
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
        uint64_t audioTimeSamples = 0;
        bool running = false;

        // Music Sequencer State (Phase 3)
        const MusicTrack* currentTrack = nullptr;
        size_t currentNoteIndex = 0;
        uint64_t nextNoteSample = 0;
        float tempoFactor = 1.0f;
        bool musicPlaying = false;
        bool musicPaused = false;

        void processCommands();
        void executePlayEvent(const AudioEvent& event);
        void updateMusicSequencer(int length);
        void playCurrentNote();
        
        AudioChannel* findFreeChannel(WaveType type);
        int16_t generateSampleForChannel(AudioChannel& ch);
    };

} // namespace pixelroot32::audio
