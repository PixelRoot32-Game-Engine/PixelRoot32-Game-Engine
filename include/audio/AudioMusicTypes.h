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
    
    // ADSR Envelope
    float attackTime = 0.002f;     // seconds
    float decayTime = 0.0f;        // seconds
    float sustainLevel = 1.0f;     // 0.0 - 1.0
    float releaseTime = 0.005f;    // seconds

    // LFO Modulation
    LfoTarget lfoTarget = LfoTarget::NONE;
    float lfoFrequency = 0.0f;     // Hz
    float lfoDepth = 0.0f;         // Pitch: ratio (e.g. 0.05 for 5%). Volume: 0.0-1.0 depth.
    float lfoDelay = 0.0f;         // seconds before LFO starts

    // Waveform Refinements
    bool noiseShortMode = false;   // For NOISE: true = metallic timbre (93-step LFSR)
    float dutySweep = 0.0f;        // For PULSE: duty cycle change per second
};

constexpr InstrumentPreset INSTR_PULSE_LEAD{
    0.35f,    // baseVolume
    0.5f,     // duty (square)
    4,        // defaultOctave
    0.0f,     // defaultDuration (use note.duration)
    0,        // noisePeriod (unused)
    0.005f,   // attackTime  – fast attack for lead
    0.20f,    // decayTime   – noticeable decay to shape note
    0.70f,    // sustainLevel– moderate sustain
    0.15f,    // releaseTime – shorter release for fast passages
    LfoTarget::PITCH,   // lfoTarget – vibrato
    5.0f,     // lfoFrequency (Hz)
    0.025f,   // lfoDepth    – slightly more vibrato
    0.15f,    // lfoDelay    – delayed vibrato for natural feel
    false,    // noiseShortMode (unused for pulse)
    0.0f      // dutySweep   – no sweep by default
};

constexpr InstrumentPreset INSTR_TRIANGLE_LEAD{
    0.32f,    // baseVolume – slightly lower than pulse lead (triangle is softer)
    0.5f,     // duty (unused for triangle, API uniformity)
    5,        // defaultOctave – higher range for lead melodies
    0.0f,     // defaultDuration
    0,        // noisePeriod (unused)
    0.003f,   // attackTime – fast attack
    0.15f,    // decayTime
    0.75f,    // sustainLevel – smooth triangle sustain
    0.12f,    // releaseTime
    LfoTarget::PITCH,   // lfoTarget – vibrato
    4.0f,     // lfoFrequency (Hz) – gentle vibrato
    0.020f,   // lfoDepth – subtle vibrato
    0.20f,    // lfoDelay – delayed vibrato
    false,    // noiseShortMode (unused)
    0.0f      // dutySweep
};

constexpr InstrumentPreset INSTR_TRIANGLE_PAD{
    0.28f,    // baseVolume – soft atmospheric pad
    0.5f,     // duty (unused for triangle)
    4,        // defaultOctave – mid range for pad chords
    0.0f,     // defaultDuration
    0,        // noisePeriod
    0.015f,   // attackTime – slow attack for pad swells
    0.40f,    // decayTime – long decay
    0.65f,    // sustainLevel
    0.50f,    // releaseTime – long release
    LfoTarget::VOLUME,  // lfoTarget – gentle tremolo
    2.5f,     // lfoFrequency (Hz) – slow modulation
    0.15f,    // lfoDepth – subtle volume movement
    0.50f,    // lfoDelay
    false,    // noiseShortMode
    0.0f      // dutySweep
};

constexpr InstrumentPreset INSTR_PULSE_PAD{
    0.26f,    // baseVolume
    0.25f,    // duty (1/4) – hollow pulse sound
    4,        // defaultOctave
    0.0f,     // defaultDuration
    0,        // noisePeriod
    0.020f,   // attackTime – pad-like slow attack
    0.60f,    // decayTime – very long decay
    0.55f,    // sustainLevel
    0.45f,    // releaseTime
    LfoTarget::PITCH,   // lfoTarget – slow pitch drift
    3.0f,     // lfoFrequency (Hz)
    0.035f,   // lfoDepth – noticeable pitch modulation
    0.30f,    // lfoDelay
    false,    // noiseShortMode
    0.08f     // dutySweep – gentle PWM movement for evolving texture
};


constexpr InstrumentPreset INSTR_PULSE_HARMONY{
    0.22f,    // baseVolume
    0.125f,   // duty (1/8)
    5,        // defaultOctave
    0.0f,     // defaultDuration
    0,        // noisePeriod
    0.005f,   // attackTime
    0.50f,    // decayTime   – long decay for evolving pad
    0.60f,    // sustainLevel– slightly higher sustain for more presence
    0.30f,    // releaseTime
    LfoTarget::VOLUME,  // lfoTarget – tremolo
    6.0f,     // lfoFrequency (Hz)
    0.30f,    // lfoDepth    – 30 % volume modulation
    0.0f,     // lfoDelay
    false,    // noiseShortMode
    0.15f     // dutySweep   – more pronounced PWM-like movement
};

constexpr InstrumentPreset INSTR_TRIANGLE_BASS{
    0.30f,    // baseVolume
    0.5f,     // duty (unused for triangle, kept for API uniformity)
    3,        // defaultOctave
    0.0f,     // defaultDuration
    0,        // noisePeriod
    0.005f,   // attackTime
    0.10f,    // decayTime
    0.20f,    // sustainLevel– low sustain for tight bass
    0.10f,    // releaseTime
    LfoTarget::NONE,
    0.0f,     // lfoFrequency
    0.0f,     // lfoDepth
    0.0f,     // lfoDelay
    false,    // noiseShortMode
    0.0f      // dutySweep
};

constexpr InstrumentPreset INSTR_PULSE_BASS{
    0.30f,    // baseVolume – punchy pulse bass
    0.25f,    // duty (1/4) – tighter, hollow sound
    2,        // defaultOctave – low range for bass
    0.0f,     // defaultDuration
    0,        // noisePeriod
    0.001f,   // attackTime – fast punch
    0.08f,    // decayTime – tight decay
    0.35f,    // sustainLevel – medium sustain
    0.08f,    // releaseTime – quick release
    LfoTarget::NONE,
    0.0f,     // lfoFrequency
    0.0f,     // lfoDepth
    0.0f,     // lfoDelay
    false,    // noiseShortMode
    0.0f      // dutySweep
};


constexpr InstrumentPreset INSTR_KICK{
    0.45f,    // baseVolume – more punch
    0.0f,     // duty (0 → noise channel)
    1,        // defaultOctave (kick selector)
    0.12f,    // defaultDuration (fixed length)
    15,       // noisePeriod – lower for deeper kick sound
    0.001f,   // attackTime  – instantaneous click
    0.10f,    // decayTime   – slightly longer for more body
    0.00f,    // sustainLevel– no sustain
    0.02f,    // releaseTime – short tail to kill clicks
    LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    false,    // noiseShortMode – kick is not metallic
    0.0f      // dutySweep
};

constexpr InstrumentPreset INSTR_SNARE{
    0.35f,    // baseVolume – balanced with kick
    0.0f,     // duty → noise
    2,        // defaultOctave (snare selector)
    0.15f,    // defaultDuration
    60,       // noisePeriod – brighter, crisper noise
    0.001f,   // attackTime
    0.08f,    // decayTime   – shorter for a snappier sound
    0.00f,    // sustainLevel– no sustain
    0.05f,    // releaseTime
    LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    true,     // noiseShortMode – metallic 93‑step LFSR for snare
    0.0f      // dutySweep
};

constexpr InstrumentPreset INSTR_HIHAT{
    0.25f,    // baseVolume – cuts through the mix
    0.0f,     // duty → noise
    3,        // defaultOctave (hi‑hat selector)
    0.05f,    // defaultDuration (very short)
    12,       // noisePeriod
    0.0005f,  // attackTime  – extremely fast
    0.015f,   // decayTime   – shorter for a more "ticky" sound
    0.00f,    // sustainLevel– no sustain
    0.005f,   // releaseTime – matches current anti‑click, removes click
    LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    true,     // noiseShortMode – metallic timbre for closed hat
    0.0f      // dutySweep
};

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