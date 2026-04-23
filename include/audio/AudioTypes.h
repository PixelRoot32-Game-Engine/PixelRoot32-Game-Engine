/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>
#include <cstddef>

namespace pixelroot32::audio {

    // --- Channel Types ---

    enum class WaveType {
        PULSE,
        TRIANGLE,
        NOISE
    };

    struct EnvelopeState {
        enum class Stage : uint8_t { ATTACK, DECAY, SUSTAIN, RELEASE, OFF };
        Stage stage = Stage::OFF;
        
        // Timing (in samples, pre-calculated from seconds * sampleRate)
        uint32_t attackSamples = 0;
        uint32_t decaySamples = 0;
        float    sustainLevel = 1.0f;
        uint32_t releaseSamples = 0;
        
        // Runtime state
        uint32_t sampleCounter = 0;
        float    currentLevel = 0.0f;

        float attackDelta = 0.0f;    // 1.0 / attackSamples
        float decayDelta = 0.0f;     // (1.0 - sustainLevel) / decaySamples
        float releaseDelta = 0.0f;   // sustainLevel / releaseSamples`

        void reset() {
            stage = Stage::OFF;
            attackSamples = 0;
            decaySamples = 0;
            sustainLevel = 1.0f;
            releaseSamples = 0;
            sampleCounter = 0;
            currentLevel = 0.0f;
        }
    };

    // --- LFO Types ---
    enum class LfoTarget : uint8_t { NONE, PITCH, VOLUME };

    struct LfoState {
        bool enabled = false;
        LfoTarget target = LfoTarget::NONE;
        
        float depth = 0.0f;
        uint32_t periodSamples = 0;
        
        uint32_t sampleCounter = 0;
        float currentValue = 0.0f;

        uint16_t delaySamples = 0;
        uint16_t delayCounter = 0;
        
        void reset() {
            enabled = false;
            target = LfoTarget::NONE;
            depth = 0.0f;
            periodSamples = 0;
            sampleCounter = 0;
            currentValue = 0.0f;
            delaySamples = 0;
            delayCounter = 0;
        }
    };

    /**
     * @struct AudioChannel
     * @brief Represents the internal state of a single audio channel.
     * 
     * Designed to be static and memory-efficient.
     */
    struct AudioChannel {
        bool enabled = false;
        WaveType type;
        
        // Oscillator state (float — used by FPU and native paths)
        float frequency = 0.0f;
        float phase = 0.0f;
        float phaseIncrement = 0.0f; // Pre-calculated (freq / sampleRate)

        // Fixed-point mirror of the oscillator state — used by the no-FPU
        // (ESP32-C3 RISC-V) hot path so we avoid soft-float emulation for
        // the phase accumulator, which runs NUM_CHANNELS × sampleRate
        // times per second.
        // phaseQ32 wraps naturally at 2^32, encoding one period.
        uint32_t phaseQ32 = 0;
        uint32_t phaseIncQ32 = 0;
        uint32_t dutyCycleQ32 = 0x80000000u; // 50% default
        uint32_t basePhaseIncQ32 = 0;

        // Envelope / Volume
        EnvelopeState envelope;
        float volume = 0.0f;       // Current volume [0.0 - 1.0]
        float targetVolume = 0.0f; // Target volume for interpolation
        float volumeDelta = 0.0f;  // Volume change per sample
        
        // LFO
        LfoState lfo;

        // Wave specific parameters
        float dutyCycle = 0.5f;      // For Pulse wave [0.0 - 1.0]
        float dutySweep = 0.0f;      // Duty cycle change per sample
        int32_t dutySweepQ32 = 0;    // Fixed-point duty sweep
        uint16_t lfsrState = 0x4000; // NES-style 15-bit LFSR for deterministic noise
        bool noiseShortMode = false; // true = 93-step sequence (metallic), false = 32767-step

        /** Samples until next LFSR step on NOISE; `frequency` sets noise clock rate (not pitch). */
        uint32_t noisePeriodSamples = 1;
        /** Counts down each output sample; at 0 the LFSR advances and reloads to noisePeriodSamples. */
        uint32_t noiseCountdown = 0;

        // Duration control (sample-accurate timing)
        uint64_t remainingSamples = 0;

        void reset() {
            enabled = false;
            phase = 0.0f;
            phaseQ32 = 0;
            phaseIncQ32 = 0;
            dutyCycleQ32 = 0x80000000u;
            dutySweep = 0.0f;
            dutySweepQ32 = 0;
            envelope.reset();
            lfo.reset();
            volume = 0.0f;
            remainingSamples = 0;
            lfsrState = 0x4000; // Initialize LFSR to non-zero state
            noiseShortMode = false;
            noisePeriodSamples = 1;
            noiseCountdown = 0;
        }
    };

    // --- Event Types ---
    
    /**
     * @struct AudioEvent
     * @brief A fire-and-forget sound event triggered by the game.
     */
    struct AudioEvent {
        WaveType type;
        float frequency;
        float duration; // seconds
        float volume;   // 0.0 - 1.0
        float duty;     // For pulse only
        uint8_t noisePeriod = 0;  // For NOISE channel: 0 = calc from frequency, >0 = direct LFSR period
        
        /**
         * Optional preset driving ADSR and LFO parameters. 
         * MUST be a pointer to a static, constexpr, or global instance.
         * If nullptr, falls back to legacy default behavior.
         */
        const struct InstrumentPreset* preset = nullptr;
    };

    // --- Command Types (Phase 1) ---

    enum class AudioCommandType : uint8_t {
        PLAY_EVENT,
        STOP_CHANNEL,
        SET_MASTER_VOLUME,
        MUSIC_PLAY,
        MUSIC_STOP,
        MUSIC_PAUSE,
        MUSIC_RESUME,
        MUSIC_SET_TEMPO,
        MUSIC_SET_BPM
    };

    // Forward declaration for MusicTrack
    struct MusicTrack;

    /**
     * @struct AudioCommand
     * @brief Internal command to communicate between game and audio threads.
     */
    struct AudioCommand {
        AudioCommandType type;
        union {
            AudioEvent event;
            uint8_t channelIndex;
            float volume;
            const MusicTrack* track;
            float tempoFactor;
            float bpm;
        };

        // Multi-track support
        static constexpr size_t MAX_SUB_TRACKS = 3;
        const MusicTrack* subTracks[MAX_SUB_TRACKS];
        size_t subTrackCount;

        // Default constructor to allow use in arrays/buffers
        AudioCommand() : type(AudioCommandType::STOP_CHANNEL), channelIndex(0), subTracks{nullptr, nullptr, nullptr}, subTrackCount(0) {}
    };

}
