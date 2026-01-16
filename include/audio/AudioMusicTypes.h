#pragma once

#include <cstdint>
#include "AudioTypes.h"

namespace pixelroot32 {
namespace audio {

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
 */
struct MusicNote {
    Note note;
    uint8_t octave; // 0-8
    float duration; // Seconds
    float volume;   // 0.0 - 1.0
};

struct MusicTrack {
    const MusicNote* notes;
    size_t count;
    bool loop;
    WaveType channelType;
    float duty;
};

struct InstrumentPreset {
    float baseVolume;
    float duty;
    uint8_t defaultOctave;
};

inline constexpr InstrumentPreset INSTR_PULSE_LEAD{0.35f, 0.5f, 4};
inline constexpr InstrumentPreset INSTR_PULSE_BASS{0.30f, 0.25f, 3};
inline constexpr InstrumentPreset INSTR_PULSE_CHIP_HIGH{0.32f, 0.125f, 5};
inline constexpr InstrumentPreset INSTR_TRIANGLE_PAD{0.28f, 0.5f, 4};

inline MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration) {
    return MusicNote{note, preset.defaultOctave, duration, preset.baseVolume};
}

inline MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration) {
    return MusicNote{note, octave, duration, preset.baseVolume};
}

inline MusicNote makeRest(float duration) {
    return MusicNote{Note::Rest, 0, duration, 0.0f};
}

} // namespace audio
} // namespace pixelroot32
