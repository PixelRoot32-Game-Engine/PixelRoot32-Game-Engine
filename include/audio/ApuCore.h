/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioTypes.h"
#include "AudioCommandQueue.h"
#include "AudioMusicTypes.h"

#include <atomic>
#include <cstdint>

namespace pixelroot32::audio {

    /**
     * @class ApuCore
     * @brief Shared NES-style APU core used by every AudioScheduler.
     *
     * Owns the 4 channels (2x PULSE, 1x TRIANGLE, 1x NOISE), the SPSC command
     * queue and the music sequencer. Platform-specific schedulers
     * (DefaultAudioScheduler, ESP32AudioScheduler, NativeAudioScheduler) are
     * thin orchestrators that decide *when* generateSamples() runs; all
     * synthesis, mixing and sequencing lives here to eliminate the three-way
     * duplication that existed before.
     *
     * Mixing curve:
     *   per channel: s_c = wave_c(phase) * volume_c * MIXER_SCALE
     *   sum:         S   = Σ s_c                         (bounded to [-1.6, 1.6])
     *   compressor:  y   = S / (1 + |S| * MIXER_K)
     *   output:      y * masterVolume * 32767, passed through a single-pole
     *                DC-blocker to remove offset + transient clicks.
     *
     * On cores without an FPU (ESP32-C3) the integer-optimised path uses
     * `audio_mixer_lut` which is pre-fitted to the same curve.
     */
    class ApuCore {
    public:
        static constexpr int NUM_CHANNELS = 4;
        static constexpr size_t MAX_MUSIC_TRACKS = 4;
        static constexpr int TICKS_PER_BEAT = 4;
        static constexpr float DEFAULT_BPM = 150.0f;

        // Mixing constants exposed for diagnostics / tests.
        static constexpr float MIXER_SCALE = 0.4f;
        static constexpr float MIXER_K     = 0.5f;

        ApuCore();

        /** Configure the output sample rate. Safe to call before start. */
        void init(int sampleRate);

        /** Enqueue a command. @return false if the SPSC queue was full. */
        bool submitCommand(const AudioCommand& cmd);

        /** Generate `length` mono int16 samples into `stream`. */
        void generateSamples(int16_t* stream, int length);

        /** Total commands dropped since boot (monotonic). */
        uint32_t getDroppedCommands() const {
            return droppedCommands.load(std::memory_order_relaxed);
        }

        /** Music transport reflected to the game thread (atomic). */
        bool isMusicPlaying() const { return musicPlayingFlag.load(std::memory_order_acquire); }
        bool isMusicPaused()  const { return musicPausedFlag.load(std::memory_order_acquire); }

        int getSampleRate() const { return sampleRate; }

        /** Reset all state. Intended for unit tests. */
        void reset();

        // -- Profiling API (public for Engine access) -------------
        static constexpr int PROFILE_RING_SIZE = 64;
        struct ProfileEntry {
            uint64_t audioTimeSamples;
            float peak;
            bool clipped;
        };
        void getAndResetProfileStats(ProfileEntry* out, uint8_t& count);

    private:
        void processCommands();
        void updateMusicSequencer();
        void executePlayEvent(const AudioEvent& event);
        AudioChannel* findFreeChannel(WaveType type);
        float generateSampleForChannel(AudioChannel& ch);

        // -- Channels and I/O ---------------------------------------------
        AudioChannel channels[NUM_CHANNELS];
        AudioCommandQueue commandQueue;
        std::atomic<uint32_t> droppedCommands{0};

        int sampleRate = 44100;
        float masterVolume = 1.0f;
        int32_t masterVolumeScale = 65536; // Q16 for integer/LUT path

        // -- Anti-click + DC removal --------------------------------------
        // Single-pole high-pass filter (R ~ 0.995 @ 22 kHz ≈ 35 Hz cutoff),
        // approximates the NES APU's output HPF and prevents DC from
        // asymmetric pulse duties.
        float hpfPrevIn = 0.0f;
        float hpfPrevOut = 0.0f;

        // -- Profiling ring buffer (thread-safe offload) --------------------
        ProfileEntry profileRing[PROFILE_RING_SIZE];
        uint8_t profileHead = 0;
        uint8_t profileCount = 0;
        std::atomic<uint8_t> profileWriteIdx{0};

        // -- Legacy diagnostics (keep for non-profiling builds) ----------
        float currentPeak = 0.0f;
        uint64_t samplesSinceLog = 0;

        // -- Timing -------------------------------------------------------
        uint64_t audioTimeSamples = 0;

        // -- Music sequencer ---------------------------------------------
        const MusicTrack* tracks[MAX_MUSIC_TRACKS] = {nullptr, nullptr, nullptr, nullptr};
        size_t currentNoteIndices[MAX_MUSIC_TRACKS] = {0, 0, 0, 0};
        uint64_t nextNoteTicks[MAX_MUSIC_TRACKS] = {0, 0, 0, 0};
        uint64_t globalTickCounter = 0;
        uint64_t tickDurationSamples = 0;
        float tempoBPM = DEFAULT_BPM;
        size_t activeTrackCount = 0;
        float tempoFactor = 1.0f;

        // Expose transport flags atomically so MusicPlayer can observe
        // natural end-of-track without polling private state.
        std::atomic<bool> musicPlayingFlag{false};
        std::atomic<bool> musicPausedFlag{false};
    };

} // namespace pixelroot32::audio
