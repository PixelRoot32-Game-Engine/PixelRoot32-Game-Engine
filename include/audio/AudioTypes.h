/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>

namespace pixelroot32::audio {

    // --- Channel Types ---

    enum class WaveType {
        PULSE,
        TRIANGLE,
        NOISE
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
        
        // Oscillator state
        float frequency = 0.0f;
        float phase = 0.0f;
        float phaseIncrement = 0.0f; // Pre-calculated (freq / sampleRate)

        // Envelope / Volume
        float volume = 0.0f;       // Current volume [0.0 - 1.0]
        float targetVolume = 0.0f; // Target volume for interpolation
        float volumeDelta = 0.0f;  // Volume change per sample
        
        // Wave specific parameters
        float dutyCycle = 0.5f;    // For Pulse wave [0.0 - 1.0]
        uint16_t noiseRegister = 1; // LFSR for Noise

        // Duration control
        unsigned long durationMs = 0;
        unsigned long remainingMs = 0; // Deprecated in Phase 2
        
        uint64_t remainingSamples = 0; // Sample-accurate duration

        void reset() {
            enabled = false;
            phase = 0.0f;
            volume = 0.0f;
            remainingMs = 0;
            remainingSamples = 0;
            noiseRegister = 1;
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
        MUSIC_SET_TEMPO
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
        };

        // Default constructor to allow use in arrays/buffers
        AudioCommand() : type(AudioCommandType::STOP_CHANNEL), channelIndex(0) {}
    };

}
