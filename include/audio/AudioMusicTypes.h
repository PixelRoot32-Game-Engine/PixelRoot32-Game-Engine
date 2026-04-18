/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>
#include <cstddef> // Required for size_t
#include "AudioTypes.h"

namespace pixelroot32 {
namespace audio {

// Forward declarations
struct MusicTrack;
struct InstrumentPreset;

/**
 * Maximum simultaneous tracks supported (main + sub-tracks).
 */
constexpr size_t MAX_MUSIC_TRACKS = 4;

/**
 * Musical notes representing the 12 semitones of the chromatic scale.
 */
enum class Note : uint8_t {
    C = 0,
    Cs, // C# / Db
    D,
    Ds, // D# / Eb
    E,
    F,
    Fs, // F# / Gb
    G,
    Gs, // G# / Ab
    A,
    As, // A# / Bb
    B,
    Rest, // Silence
    COUNT
};

/**
 * Base frequencies for Octave 4 (A4 = 440.0 Hz).
 * Indexed by Note enum (0..11).
 */
static constexpr float NOTE_FREQUENCIES_OCTAVE_4[] = {
    261.63f, // C4
    277.18f, // C#4
    293.66f, // D4
    311.13f, // D#4
    329.63f, // E4
    349.23f, // F4
    369.99f, // F#4
    392.00f, // G4
    415.30f, // G#4
    440.00f, // A4
    466.16f, // A#4
    493.88f  // B4
};

/**
 * Helper to convert Note + Octave to Frequency (Hz).
 * @param note The note (C to B)
 * @param octave The octave index (standard piano: C4 is middle C, A4 is 440Hz).
 *               Common range: 0 to 8.
 * @return Frequency in Hz. Returns 0.0f if note is invalid.
 */
inline float noteToFrequency(Note note, int octave) {
    uint8_t noteIdx = static_cast<uint8_t>(note);
    if (noteIdx >= 12) return 0.0f;

    float base = NOTE_FREQUENCIES_OCTAVE_4[noteIdx];
    int shift = octave - 4;

    if (shift == 0) {
        return base;
    } else if (shift > 0) {
        // Higher octave: multiply by 2^shift
        // We use integer shift for powers of 2, which is efficient.
        // Cast to float after shift to multiply the base frequency.
        return base * static_cast<float>(1 << shift);
    } else {
        // Lower octave: divide by 2^(-shift)
        return base / static_cast<float>(1 << (-shift));
    }
}

/**
 * @struct MusicNote
 * @brief Represents a single note in a melody.
 * 
 * For percussion (note.preset && preset.duty == 0):
 *   - preset defines frequency and defaultDuration
 *   - octave still determines drum type if preset not available
 */
struct MusicNote {
    Note note;
    uint8_t octave; // 0-8 (for percussion: 1=Kick, 2=Snare, 3+=Hi-HAT)
    float duration; // Seconds
    float volume;   // 0.0 - 1.0
    const InstrumentPreset* preset = nullptr;  // nullptr = use track preset or legacy behavior
};

struct MusicTrack {
    const MusicNote* notes;
    size_t count;
    bool loop;
    WaveType channelType;
    float duty;
    // Optional parallel sub-tracks for multi-voice playback
    const MusicTrack* secondVoice = nullptr;  // nullptr = disabled
    const MusicTrack* thirdVoice = nullptr;   // nullptr = disabled
    const MusicTrack* percussion = nullptr;   // nullptr = disabled
};

/**
 * @struct InstrumentPreset
 * @brief Defines instrument characteristics for playback.
 * 
 * For melodic instruments (duty > 0):
 *   - defaultOctave: the base octave for notes
 *   - duty: duty cycle for PULSE wave (e.g., 0.5, 0.125)
 * 
 * For percussion instruments (duty == 0):
 *   - defaultOctave: drum type selector (1=Kick, 2=Snare, 3+=Hi-HAT)
 *   - defaultDuration: fixed duration for each hit (0.0 = use note.duration)
 *   - noisePeriod: LFSR period for noise channel (0 = calc from frequency, >0 = direct period)
 */
struct InstrumentPreset {
    float baseVolume;
    float duty;
    uint8_t defaultOctave;
    float defaultDuration = 0.0f;  // 0.0 = use note.duration, >0 = fixed duration (percussion)
    uint8_t noisePeriod = 0;       // 0 = calc from frequency, >0 = direct LFSR period (percussion)
};

constexpr InstrumentPreset INSTR_PULSE_LEAD{0.35f, 0.5f, 4};
constexpr InstrumentPreset INSTR_PULSE_HARMONY{0.22f, 0.125f, 5};
constexpr InstrumentPreset INSTR_TRIANGLE_BASS{0.30f, 0.5f, 3};
constexpr InstrumentPreset INSTR_KICK{0.40f, 0.0f, 1, 0.12f, 25};     // Kick: short punch, noise period 25
constexpr InstrumentPreset INSTR_SNARE{0.30f, 0.0f, 2, 0.15f, 50};   // Snare: medium decay, noise period 50
constexpr InstrumentPreset INSTR_HIHAT{0.20f, 0.0f, 3, 0.05f, 12};    // Hi-HAT: very short, noise period 12

inline MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration) {
    return MusicNote{note, preset.defaultOctave, duration, preset.baseVolume, &preset};
}

inline MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration) {
    return MusicNote{note, octave, duration, preset.baseVolume, &preset};
}

inline MusicNote makeRest(float duration) {
    return MusicNote{Note::Rest, 0, duration, 0.0f, nullptr};
}

/**
 * Helper to convert InstrumentPreset to frequency.
 * For percussion (duty==0): uses defaultOctave to determine drum type.
 * For melodic (duty>0): uses noteToFrequency.
 * @param preset The instrument preset.
 * @param note The note (ignored for percussion).
 * @param octave The octave (ignored for percussion).
 * @return Frequency in Hz.
 */
inline float instrumentToFrequency(const InstrumentPreset& preset, Note /*note*/, uint8_t /*octave*/) {
    // Percussion: duty == 0 means NOISE channel
    if (preset.duty == 0.0f) {
        // defaultOctave determines drum type:
        // 1 = Kick (low frequency)
        // 2 = Snare (mid frequency)
        // 3+ = Hi-HAT/Cymbal (high frequency)
        if (preset.defaultOctave <= 1) {
            return 80.0f;   // Kick
        } else if (preset.defaultOctave == 2) {
            return 150.0f;  // Snare
        } else {
            return 3000.0f; // Hi-HAT
        }
    }
    // Melodic: fallback to 440Hz (shouldn't be called for noise)
    return 440.0f;
}

} // namespace audio
} // namespace pixelroot32