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
        /** @brief Maximum number of voices (dynamic pool size). */
        static constexpr int MAX_VOICES = 8;
        // Backward-compatible alias used in some tests/diagnostics.
        /** @brief Alias for MAX_VOICES for backward compatibility. */
        static constexpr int NUM_CHANNELS = MAX_VOICES;
        /** @brief Maximum simultaneous music tracks. */
        static constexpr size_t MAX_MUSIC_TRACKS = 4;
        /** @brief Default ticks (subdivisions) per beat. */
        static constexpr int TICKS_PER_BEAT = 4;
        /** @brief Default tempo in BPM. */
        static constexpr float DEFAULT_BPM = 150.0f;

        // Sequencer note limit: max notes processed per frame to prevent unbounded catch-up
        /** @brief Default max notes per frame for the music sequencer. */
        static constexpr size_t DEFAULT_MAX_NOTES_PER_FRAME = 32;
        #ifndef AUDIO_SEQUENCER_MAX_NOTES
        /** @brief User-configurable max notes per frame. */
        static constexpr size_t MAX_NOTES_PER_FRAME = DEFAULT_MAX_NOTES_PER_FRAME;
        #else
        static_assert(AUDIO_SEQUENCER_MAX_NOTES >= 1 && AUDIO_SEQUENCER_MAX_NOTES <= 1024,
            "AUDIO_SEQUENCER_MAX_NOTES must be between 1 and 1024");
        static constexpr size_t MAX_NOTES_PER_FRAME = (AUDIO_SEQUENCER_MAX_NOTES > 1000) ? 32 : AUDIO_SEQUENCER_MAX_NOTES;
        #endif

        // Mixing constants exposed for diagnostics / tests.
        /** @brief Per-channel scaling factor before summation. */
        static constexpr float MIXER_SCALE = 0.4f;
        /** @brief Compressor nonlinearity constant. */
        static constexpr float MIXER_K     = 0.5f;

        /** @brief Default constructor. Initializes state but does not set sample rate. */
        ApuCore();

        /**
         * @brief Configures the output sample rate.
         * @param sampleRate Sample rate in Hz (e.g., 22050, 44100).
         * 
         * Safe to call before start(). Pre-calculates internal timing values.
         */
        void init(int sampleRate);

        /**
         * @brief Enqueues a command for processing.
         * @param cmd The audio command to enqueue.
         * @return true if the command was enqueued, false if the queue was full.
         */
        bool submitCommand(const AudioCommand& cmd);

        /**
         * @brief Generates audio samples.
         * @param stream Output buffer (mono, int16 samples).
         * @param length Number of samples to generate.
         * 
         * Processes all pending commands, updates music sequencer, and synthesizes
         * voices into the output buffer. Applies master volume, bitcrush, and DC blocking.
         */
        void generateSamples(int16_t* stream, int length);

        /** @brief Returns the total number of commands dropped since construction. @return Monotonic count. */
        uint32_t getDroppedCommands() const {
            return droppedCommands.load(std::memory_order_relaxed);
        }

        // Sequencer note limit API
        /**
         * @brief Sets the maximum notes processed per audio frame.
         * @param limit Max notes [1-1000], clamped to safe bounds internally.
         * 
         * Bounded processing prevents audio starvation when many notes queue up.
         */
        void setSequencerNoteLimit(size_t limit);

        /** @brief Gets current max notes per frame setting. @return Current limit. */
        size_t getSequencerNoteLimit() const { return sequencerNoteLimit.load(std::memory_order_acquire); }

        /** @brief Returns notes deferred to next frame due to note limit. @return Deferred count. */
        size_t getDeferredNotes() const { return deferredNotes.load(std::memory_order_relaxed); }

        /** @brief Reports if music is currently playing. @return true if playing. */
        bool isMusicPlaying() const { return musicPlayingFlag.load(std::memory_order_acquire); }
        /** @brief Reports if music is paused. @return true if paused. */
        bool isMusicPaused()  const { return musicPausedFlag.load(std::memory_order_acquire); }

        /** @brief Gets the configured sample rate. @return Sample rate in Hz. */
        int getSampleRate() const { return sampleRate; }

        /**
         * @brief Resets all state to initial values.
         * 
         * Intended for unit tests. Stops all voices, clears sequencer, resets counters.
         */
        void reset();

        /**
         * @brief Sets an optional post-mix callback.
         * @param fn Function pointer: void(int16_t* mono, int length, void* user).
         * @param user User data passed to the callback.
         * 
         * Called after bitcrush on the final mono buffer. Runs in audio thread context.
         */
        void setPostMixMono(void (*fn)(int16_t* mono, int length, void* user), void* user);

        // -- Profiling API (public for Engine access) -------------
        /** @brief Ring buffer size for profile entries. */
        static constexpr int PROFILE_RING_SIZE = 64;
        /**
         * @brief A single profile snapshot.
         */
        struct ProfileEntry {
            uint64_t audioTimeSamples; ///< Global sample counter at capture.
            float peak;                ///< Peak sample magnitude [0.0 - 1.0].
            bool clipped;              ///< Whether any sample exceeded ±32767.
        };
        /**
         * @brief Reads and clears all profile entries.
         * @param out Array of ProfileEntry to fill.
         * @param count On input: max entries. On output: actual count written.
         */
        void getAndResetProfileStats(ProfileEntry* out, uint8_t& count);

#if defined(UNIT_TEST)
        /** Diagnostic: count voices with enabled==true (native_test regression). */
        size_t countEnabledVoicesForTesting() const;
        /** Main music track note index after sequencer run (native_test regression). */
        size_t getSequencerMainNoteIndexForTesting() const;
#endif

    private:
        void processCommands();
        void updateMusicSequencer();
        void executePlayEvent(const AudioEvent& event);
        Voice* findVoiceForEvent(WaveType type);
        float generateSampleForVoice(Voice& voice);

        // -- Channels and I/O ---------------------------------------------
        Voice voices[MAX_VOICES];
        AudioCommandQueue commandQueue;
        std::atomic<uint32_t> droppedCommands{0};

        int sampleRate = 44100;
        float masterVolume = 1.0f;
        int32_t masterVolumeScale = 65536; // Q16 for integer/LUT path
        /** 0 = off; 1–15 = re-quantize final int16 (post-mixer). */
        uint8_t masterBitcrushBits_ = 0;

        // -- Anti-click + DC removal --------------------------------------
        // Single-pole high-pass filter (R ~ 0.995 @ 22 kHz ≈ 35 Hz cutoff),
        // approximates the NES APU's output HPF and prevents DC from
        // asymmetric pulse duties.
        float hpfPrevIn = 0.0f;
        float hpfPrevOut = 0.0f;

        // Q15 HPF state for no-FPU path (ESP32-C3, RISC-V cores)
        int32_t hpfPrevInQ15 = 0;
        int32_t hpfPrevOutQ15 = 0;

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

        // Sequencer note limit per frame (bounded processing to prevent audio starvation)
        std::atomic<size_t> sequencerNoteLimit{MAX_NOTES_PER_FRAME};
        std::atomic<size_t> deferredNotes{0};

        // Expose transport flags atomically so MusicPlayer can observe
        // natural end-of-track without polling private state.
        std::atomic<bool> musicPlayingFlag{false};
        std::atomic<bool> musicPausedFlag{false};

        /** After MUSIC_PLAY, allow one sequencer pass even when currentTick == startTick. */
        bool firstSequencerCallAfterPlay_ = false;

        // -- Post-mix hook -------------------------------------------------
        void (*postMixMono_)(int16_t* mono, int length, void* user) = nullptr;
        void* postMixUser_ = nullptr;
    };

} // namespace pixelroot32::audio
