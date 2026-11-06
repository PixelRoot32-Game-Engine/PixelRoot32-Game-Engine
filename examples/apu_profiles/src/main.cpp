/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

/**
 * @file main.cpp
 * @brief Example: the three APU sound profiles, judged by ear.
 *
 * Nine 12-second blocks - `simple`, `medium` and `complex`, each under
 * `Legacy`, `Nes` and `Famicom` - loop forever. Only a human ear can answer
 * "which profile sounds better", so this example fixes everything else: the
 * programme, the order, the length and the hardware. It therefore does the two
 * things a listener needs and nothing else:
 *
 * - it plays a fixed programme, in a fixed order, for a fixed time, so two
 *   profiles are always compared on the same notes; and
 * - it prints one banner before and one report after each block, so the serial
 *   console is a transcript of what the speaker just played (`peak` also proves
 *   the block was not silent, which `underruns=0` alone cannot).
 *
 * The audio path is the engine's real one: `AudioEngine` +
 * `ESP32_I2S_AudioBackend` at 22 050 Hz. Music and events go through the APU
 * command queue, and the profile is switched with `SET_SOUND_PROFILE` because
 * the profile is audio-thread state and the command is the documented crossing
 * from the main thread.
 *
 * The pieces are deliberately cumulative, so a difference heard at one level can
 * be attributed to the number of voices:
 *   simple  - one PULSE melody (one voice).
 *   medium  - melody + TRIANGLE bass + NOISE hi-hat (three voices).
 *   complex - the Tool Suite sample song "Crystal Cascade" at 150 BPM: four
 *             sequencer tracks (12.5 % PULSE lead, TRIANGLE bass, 25 % PULSE
 *             harmony, NOISE drum kit) on one 16th-note grid, every track exactly
 *             64 steps long. Three melodic voices, plus drum hits that the
 *             sequencer plays on the SFX slots, up to three overlapping at the
 *             pattern's closing crash (six voices at the peak).
 *
 * A status panel (TFT_eSPI, ILI9341) is repainted once per block with the piece
 * and the profile, so the listener can see from across the room what the serial
 * log already says. The panel is optional by construction: the audio starts
 * before the display is touched and every draw is gated on a ready flag, so a
 * missing or broken screen can never silence the example.
 *
 * No buttons, no dynamic allocation in the audio path: every note table is
 * `constexpr` and lives in flash.
 *
 * This example is the APU listening harness: it replaced the PixelRoot32-APU
 * repository's `tools/apu_listen`, which moved here. The serial contract and
 * the panel layout are kept on purpose so older transcripts stay readable; the
 * complex piece is a transcription of a Tool Suite sample song since APU 2.3.0,
 * so its earlier `peak`/`hash` figures are not comparable.
 */

#if defined(PLATFORM_NATIVE)
#include <cstdarg>
#include <cstdio>

#include <drivers/native/SDL2_AudioBackend.h>
#include <platforms/mock/MockArduino.h>
#else
#include <Arduino.h>
#include <TFT_eSPI.h>

#include <drivers/esp32/ESP32_I2S_AudioBackend.h>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <audio/AudioConfig.h>
#include <audio/AudioEngine.h>
#include <pixelroot32/apu/ApuCore.h>
#include <pixelroot32/apu/AudioMusicTypes.h>
#include <pixelroot32/apu/AudioTypes.h>

namespace {

using pixelroot32::audio::ApuCore;
using pixelroot32::audio::AudioCommand;
using pixelroot32::audio::AudioCommandType;
using pixelroot32::audio::AudioConfig;
using pixelroot32::audio::AudioEngine;
using pixelroot32::audio::AudioEvent;
using pixelroot32::audio::AudioScheduler;
using pixelroot32::audio::EnvelopeCurve;
using pixelroot32::audio::InstrumentPreset;
using pixelroot32::audio::LfoTarget;
using pixelroot32::audio::MusicNote;
using pixelroot32::audio::MusicTrack;
using pixelroot32::audio::NoisePeriodMode;
using pixelroot32::audio::Note;
using pixelroot32::audio::SoundProfile;
using pixelroot32::audio::WaveType;

using pixelroot32::audio::INSTR_HIHAT;
using pixelroot32::audio::INSTR_KICK;
using pixelroot32::audio::INSTR_PULSE_LEAD;
using pixelroot32::audio::INSTR_SNARE;
using pixelroot32::audio::INSTR_TRIANGLE_BASS;

#if defined(PLATFORM_NATIVE)
using pixelroot32::drivers::native::SDL2_AudioBackend;
#else
using pixelroot32::drivers::esp32::ESP32_I2S_AudioBackend;
#endif

#if defined(PLATFORM_NATIVE)
// ---------------------------------------------------------------------------
// Native (PC) console.
//
// The engine already ships the Arduino timing mock (`MockArduino.h`, included
// above), so `millis()` and `delay()` compile here unchanged. What it does not
// mock is `Serial`, and the transcript is the whole point of this example: the
// shim below keeps every call site identical, printing the same lines to stdout
// instead of the UART.
// ---------------------------------------------------------------------------

/** @brief Stand-in for the Arduino `Serial` that writes to stdout. */
struct NativeConsole {
    void begin(unsigned long) {}
    void print(const char* text) { std::fputs(text, stdout); }
    void println(const char* text = "")
    {
        std::fputs(text, stdout);
        std::fputc('\n', stdout);
    }
    void printf(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        std::vprintf(format, args);
        va_end(args);
    }
    void flush() { std::fflush(stdout); }
};

NativeConsole Serial;

#endif

// ---------------------------------------------------------------------------
// Fixed hardware: the `pong` demo's PLATFORM_ESP32DEV_ILI9341 MAX98357A wiring.
// These are compile-time constants on purpose: a listening comparison is only
// meaningful on the board the profiles ship on, so there is nothing to configure.
// ---------------------------------------------------------------------------

#if !defined(PLATFORM_NATIVE)
/** @brief I2S bit clock. */
constexpr int kI2sBclk = 5;
/** @brief I2S word clock / left-right clock. */
constexpr int kI2sLrck = 21;
/** @brief I2S serial data out to the MAX98357A DIN. */
constexpr int kI2sDout = 19;
#endif

/** @brief Sample rate the comparison runs at. */
constexpr int kSampleRate = 22050;

#if !defined(PLATFORM_NATIVE)
/** @brief Logical panel size after the quarter turn (TFT_WIDTH/HEIGHT are 240x320). */
constexpr int kScreenWidth = 320;
constexpr int kScreenHeight = 240;
/** @brief Quarter turn applied to the physical portrait mounting. */
constexpr uint8_t kDisplayRotation = DISPLAY_ROTATION;
static_assert(kScreenWidth == TFT_HEIGHT && kScreenHeight == TFT_WIDTH,
              "the status layout is written for the 240x320 panel after its quarter turn");
#endif

/** @brief One block per piece x profile combination. */
constexpr uint32_t kBlockMs = 12000;

/**
 * @brief Tempo of the simple and medium pieces: one beat is 500 ms. The complex
 *        piece plays at its source song's own tempo (`kCascadeBpm`).
 */
constexpr float kBpm = 120.0f;

/** @brief Pieces and profiles, and therefore blocks, in the programme. */
constexpr uint8_t kPieceCount = 3;
constexpr uint8_t kProfileCount = 3;
constexpr uint8_t kBlockCount = kPieceCount * kProfileCount;
static_assert(kBlockCount == 9, "the programme is three pieces x three profiles");

/** @brief Defensive bound for a track table; a typo must not walk off the array. */
constexpr size_t kMaxTrackNotes = 64;

// ---------------------------------------------------------------------------
// Music tables.
//
// `MusicNote::duration` is in beats (quarter note = 1.0), not seconds: the
// sequencer multiplies it by the tick duration in `updateMusicSequencer`. The
// tables are `constexpr` so they live in flash, and the helpers below keep the
// positional initializers readable.
// ---------------------------------------------------------------------------

/**
 * @brief Builds one melodic note for a constexpr table.
 * @param preset Instrument preset supplying ADSR, LFO and duty defaults.
 * @param pitch Note name.
 * @param octave Octave index (C4 = middle C, A4 = 440 Hz).
 * @param beats Note length in beats.
 * @param volume Linear event volume [0, 1].
 * @return The note, pointing at the static preset.
 */
constexpr MusicNote note(const InstrumentPreset& preset, Note pitch, uint8_t octave,
                         float beats, float volume)
{
    return MusicNote{pitch, octave, beats, volume, &preset};
}

/**
 * @brief Builds one rest for a constexpr table.
 * @param beats Rest length in beats.
 * @return The rest note.
 */
constexpr MusicNote rest(float beats)
{
    return MusicNote{Note::Rest, 0, beats, 0.0f, nullptr};
}

/**
 * @brief Builds one percussion hit for a NOISE track.
 *
 * A percussion hit is a `Note::Rest` carrying a preset whose duty is 0: the
 * sequencer's `isPercussionHit` rule. Its duration is 0 beats on purpose, so the
 * hit fires without advancing the grid (the following rest does that).
 * @param preset Percussion preset (kick, snare, hi-hat).
 * @param volume Linear event volume [0, 1].
 * @return The hit note.
 */
constexpr MusicNote hit(const InstrumentPreset& preset, float volume)
{
    return MusicNote{Note::Rest, preset.defaultOctave, 0.0f, volume, &preset};
}

/** @brief Compile-time element count, so a table edit cannot desync its track. */
template <typename T, size_t N>
constexpr size_t countOf(const T (&)[N])
{
    return N;
}

// -- simple: one PULSE melody, sixteen eighth notes in A minor / C major ------

constexpr MusicNote kSimpleNotes[] = {
    note(INSTR_PULSE_LEAD, Note::A, 4, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::E, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::D, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::B, 4, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::A, 4, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::G, 4, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::A, 4, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::E, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::A, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::G, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::E, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::D, 5, 0.5f, 0.32f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.32f),
};

constexpr MusicTrack kSimpleTrack{kSimpleNotes, countOf(kSimpleNotes), true,
                                  WaveType::PULSE, 0.5f, nullptr, nullptr, nullptr};

// -- medium: melody + TRIANGLE root per bar + NOISE hi-hat on the beat --------

constexpr MusicNote kMediumMelodyNotes[] = {
    note(INSTR_PULSE_LEAD, Note::A, 4, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::D, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::E, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::G, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::E, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::D, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::A, 4, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::D, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::E, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::G, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::E, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::D, 5, 0.5f, 0.30f),
    note(INSTR_PULSE_LEAD, Note::C, 5, 0.5f, 0.30f),
};

// i - VI - III - VII in A minor, one whole-bar root each.
constexpr MusicNote kMediumBassNotes[] = {
    note(INSTR_TRIANGLE_BASS, Note::A, 2, 4.0f, 0.28f),
    note(INSTR_TRIANGLE_BASS, Note::F, 2, 4.0f, 0.28f),
    note(INSTR_TRIANGLE_BASS, Note::C, 3, 4.0f, 0.28f),
    note(INSTR_TRIANGLE_BASS, Note::G, 2, 4.0f, 0.28f),
};

constexpr MusicNote kMediumHatNotes[] = {
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
    hit(INSTR_HIHAT, 0.12f), rest(1.0f), hit(INSTR_HIHAT, 0.12f), rest(1.0f),
};

constexpr MusicTrack kMediumMelodyTrack{kMediumMelodyNotes, countOf(kMediumMelodyNotes),
                                        true, WaveType::PULSE, 0.5f, nullptr, nullptr,
                                        nullptr};
constexpr MusicTrack kMediumBassTrack{kMediumBassNotes, countOf(kMediumBassNotes), true,
                                      WaveType::TRIANGLE, 0.5f, nullptr, nullptr, nullptr};
constexpr MusicTrack kMediumHatTrack{kMediumHatNotes, countOf(kMediumHatNotes), true,
                                     WaveType::NOISE, 0.5f, nullptr, nullptr, nullptr};

// -- complex: "Crystal Cascade", transcribed from the Tool Suite ---------------
//
// The complex piece is the PixelRoot32 Tool Suite sample song "Crystal Cascade"
// (`toolsuite_app/samples/music/Crystal_Cascade.pr32music`, 150 BPM, 4 steps per
// beat, 64-step patterns): the first pattern of each of its four tracks (L_a,
// B_a, H_a, D_a), written out the way the Tool Suite's preview builds a song.
// That is the Tool Suite's sequencing model, and it is what keeps the piece in
// time:
//
// - every timed voice is a sequencer track; nothing is fired from `millis()`;
// - notes sit on one 16th-note step grid, by start step and length in steps;
//   a gap between notes is one merged rest;
// - every track spans exactly the same 64 steps (16 beats), so the four tracks
//   loop together forever (checked at compile time below);
// - drum-kit hits on the same step stack with duration 0, and the last hit of a
//   step carries the time to the next one, exactly like the Tool Suite's
//   drum-kit timeline (several lanes, one NOISE track);
// - a note's volume is `preset.baseVolume * track.volume * note.volume`.
//
// The song's library instruments are not shipped by the APU, so the ones it
// uses are defined here with the Tool Suite catalogue's values. Kick, snare and
// hi-hat are the APU's own `INSTR_*` presets, which is what the song names.

/** @brief Tempo of the complex piece, the song's own. */
constexpr float kCascadeBpm = 150.0f;
/** @brief Grid resolution of the song: four 16th-note steps per beat. */
constexpr int kCascadeStepsPerBeat = 4;
/** @brief Length of every track of the piece: one 64-step pattern. */
constexpr int kCascadePatternSteps = 64;
static_assert(kCascadeStepsPerBeat == ApuCore::TICKS_PER_BEAT,
              "one song step is one sequencer tick, so no step is ever rounded");

/** @brief Track volumes from the song (`arrangement.tracks[].volume`). */
constexpr float kCascadeLeadVolume = 0.72f;
constexpr float kCascadeBassVolume = 0.75f;
constexpr float kCascadeHarmonyVolume = 0.48f;
constexpr float kCascadeDrumsVolume = 0.5f;

/**
 * @brief Melodic library preset with the Tool Suite catalogue's field order.
 * @param volume Base volume.
 * @param duty Pulse duty.
 * @param octave Default octave.
 * @param attack Attack time (s).
 * @param decay Decay time (s).
 * @param sustain Sustain level [0, 1].
 * @param release Release time (s).
 * @return The preset; every other field keeps its default.
 */
constexpr InstrumentPreset libMelodic(float volume, float duty, uint8_t octave, float attack,
                                      float decay, float sustain, float release)
{
    return InstrumentPreset{volume, duty, octave, 0.0f, 0, attack, decay, sustain, release};
}

/**
 * @brief NOISE drum library preset with the Tool Suite catalogue's field order.
 *
 * Like the Tool Suite's drum kit (and the APU's own `INSTR_*` drums), the
 * authored noise period is pinned per note, so a NES profile does not retune it.
 * @param volume Base volume.
 * @param octave Drum-type selector (1 kick, 2 snare, 3+ hat/cymbal).
 * @param duration Fixed hit length (s).
 * @param noisePeriod Authored LFSR period.
 * @param attack Attack time (s).
 * @param decay Decay time (s).
 * @param release Release time (s).
 * @param shortLfsr True for the metallic 93-step LFSR.
 * @return The preset.
 */
constexpr InstrumentPreset libDrum(float volume, uint8_t octave, float duration,
                                   uint8_t noisePeriod, float attack, float decay,
                                   float release, bool shortLfsr)
{
    return InstrumentPreset{volume,
                            0.0f,
                            octave,
                            duration,
                            noisePeriod,
                            attack,
                            decay,
                            0.0f,
                            release,
                            LfoTarget::NONE,
                            0.0f,
                            0.0f,
                            0.0f,
                            shortLfsr,
                            0.0f,
                            0.0f,
                            0.0f,
                            false,      // nesAccurate
                            shortLfsr,  // noiseShortMode, kept in sync with noiseLfsrShort
                            EnvelopeCurve::ProfileDefault,
                            NoisePeriodMode::Note};
}

// Tool Suite library instruments the song uses (music_instrument_catalog.cpp).
constexpr InstrumentPreset kLibPulseSpark = libMelodic(0.36f, 0.125f, 5, 0.001f, 0.05f, 0.35f, 0.06f);
constexpr InstrumentPreset kLibPulseThin = libMelodic(0.36f, 0.125f, 5, 0.001f, 0.10f, 0.80f, 0.16f);
constexpr InstrumentPreset kLibPulsePluck = libMelodic(0.38f, 0.25f, 4, 0.001f, 0.12f, 0.40f, 0.08f);
constexpr InstrumentPreset kLibPulseMarimba = libMelodic(0.32f, 0.25f, 5, 0.001f, 0.20f, 0.05f, 0.12f);
constexpr InstrumentPreset kLibTriangleSubBass = libMelodic(0.40f, 0.5f, 1, 0.004f, 0.12f, 0.65f, 0.16f);
constexpr InstrumentPreset kLibDrumCrash = libDrum(0.26f, 7, 0.22f, 10, 0.0005f, 0.28f, 0.08f, true);
constexpr InstrumentPreset kLibDrumOpenHat = libDrum(0.22f, 4, 0.12f, 11, 0.0005f, 0.15f, 0.02f, true);

/**
 * @brief Converts a length in song steps to sequencer beats.
 * @param count Number of 16th-note steps.
 * @return Length in beats.
 */
constexpr float steps(int count)
{
    return static_cast<float>(count) / static_cast<float>(kCascadeStepsPerBeat);
}

/**
 * @brief One melodic note of the song, as the Tool Suite builds it.
 * @param preset Instrument the note (or its track) names.
 * @param trackVolume The track's volume.
 * @param pitch Note name.
 * @param octave Octave.
 * @param lengthSteps Length in steps.
 * @param noteVolume The note's own volume.
 * @return The note; its volume is `baseVolume * trackVolume * noteVolume`.
 */
constexpr MusicNote tone(const InstrumentPreset& preset, float trackVolume, Note pitch,
                         uint8_t octave, int lengthSteps, float noteVolume)
{
    return MusicNote{pitch, octave, steps(lengthSteps),
                     preset.baseVolume * trackVolume * noteVolume, &preset};
}

/**
 * @brief One drum-kit hit of the song, as the Tool Suite builds it.
 *
 * A `Note::Rest` with a duty-0 preset is the sequencer's percussion hit. A
 * length of 0 steps stacks the hit on the same step as the next one; the last
 * hit of a step carries the time to the next step that has a hit.
 * @param preset Drum preset.
 * @param lengthSteps Steps until the next hit (0 = stacked).
 * @param noteVolume The hit's own volume.
 * @return The hit; octave 1 as the song stores it (the preset picks the drum).
 */
constexpr MusicNote drum(const InstrumentPreset& preset, int lengthSteps, float noteVolume)
{
    return MusicNote{Note::Rest, 1, steps(lengthSteps),
                     preset.baseVolume * kCascadeDrumsVolume * noteVolume, &preset};
}

/**
 * @brief Total length of a note table in beats.
 * @param notes Note table.
 * @return Sum of every note's duration.
 */
template <size_t N>
constexpr float beatsOf(const MusicNote (&notes)[N])
{
    float total = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        total += notes[i].duration;
    }
    return total;
}

// Lead (L_a): PULSE, 12.5 % duty, `LIB_PULSE_SPARK` with `LIB_PULSE_THIN` on steps
// 3, 11, 19, ... (the last 16th of every other beat). One 16th per step, no gaps.
constexpr MusicNote kComplexMelodyNotes[] = {
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 4, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::C, 5, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 4, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::A, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::F, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::F, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::F, 3, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::B, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::D, 4, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::G, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::D, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::B, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 3, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::D, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 4, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::A, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::B, 3, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::E, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::B, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::E, 3, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::B, 2, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::F, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::F, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::A, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::F, 3, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::C, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 3, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::B, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::D, 4, 1, 0.68f),
    tone(kLibPulseThin, kCascadeLeadVolume, Note::G, 4, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::D, 4, 1, 0.82f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::B, 3, 1, 0.55f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::G, 3, 1, 0.68f),
    tone(kLibPulseSpark, kCascadeLeadVolume, Note::D, 3, 1, 0.55f),
};

// Bass (B_a): TRIANGLE, `LIB_TRIANGLE_SUB_BASS`. Two chord roots per bar, each a
// dotted eighth on the root and a 16th pickup, so the bass already lands on every
// kick of the drum track; the triangle kick (`INSTR_KICK_TRI`) is not added here,
// because it would replace those roots and change the song.
constexpr MusicNote kComplexBassNotes[] = {
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::C, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::C, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::C, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::G, 2, 1, 0.6f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::A, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::A, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::A, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::E, 3, 1, 0.6f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::F, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::F, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::F, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::C, 3, 1, 0.6f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::G, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::G, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::G, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::D, 3, 1, 0.6f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::A, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::A, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::A, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::E, 3, 1, 0.6f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::E, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::E, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::E, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::B, 2, 1, 0.6f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::F, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::F, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::F, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::C, 3, 1, 0.6f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::G, 2, 3, 0.88f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::G, 3, 1, 0.55f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::G, 2, 3, 0.78f),
    tone(kLibTriangleSubBass, kCascadeBassVolume, Note::D, 3, 1, 0.6f),
};

// Harmony (H_a): PULSE, 25 % duty, `LIB_PULSE_PLUCK`, with `LIB_PULSE_MARIMBA`
// opening each chord of the second half. Repeated eighth notes.
constexpr MusicNote kComplexHarmonyNotes[] = {
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::G, 4, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::G, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::C, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::C, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::E, 4, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::E, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::A, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::A, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::C, 4, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::C, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::F, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::F, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::D, 4, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::D, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::G, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::G, 3, 2, 0.38f),
    tone(kLibPulseMarimba, kCascadeHarmonyVolume, Note::E, 4, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::E, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::A, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::A, 3, 2, 0.38f),
    tone(kLibPulseMarimba, kCascadeHarmonyVolume, Note::B, 3, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::B, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::E, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::E, 3, 2, 0.38f),
    tone(kLibPulseMarimba, kCascadeHarmonyVolume, Note::C, 4, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::C, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::F, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::F, 3, 2, 0.38f),
    tone(kLibPulseMarimba, kCascadeHarmonyVolume, Note::D, 4, 2, 0.5f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::D, 4, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::G, 3, 2, 0.38f),
    tone(kLibPulsePluck, kCascadeHarmonyVolume, Note::G, 3, 2, 0.38f),
};

// Drums (D_a): one NOISE drum-kit track. Kick on beats 1 and 3, snare on 2 and 4,
// hi-hat on every eighth, and a crash and an open hat closing the pattern. Hits on
// the same step are stacked (length 0); the last one carries the step's time.
constexpr MusicNote kComplexPercussionNotes[] = {
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.75f),  // step 0
    drum(INSTR_HIHAT, 2, 0.18f),  // step 2
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 2, 0.55f),  // step 4
    drum(INSTR_HIHAT, 2, 0.18f),  // step 6
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.65f),  // step 8
    drum(INSTR_HIHAT, 2, 0.18f),  // step 10
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 2, 0.5f),  // step 12
    drum(INSTR_HIHAT, 2, 0.18f),  // step 14
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.75f),  // step 16
    drum(INSTR_HIHAT, 2, 0.18f),  // step 18
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 2, 0.55f),  // step 20
    drum(INSTR_HIHAT, 2, 0.18f),  // step 22
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.65f),  // step 24
    drum(INSTR_HIHAT, 2, 0.18f),  // step 26
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 2, 0.5f),  // step 28
    drum(INSTR_HIHAT, 2, 0.18f),  // step 30
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.75f),  // step 32
    drum(INSTR_HIHAT, 2, 0.18f),  // step 34
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 2, 0.55f),  // step 36
    drum(INSTR_HIHAT, 2, 0.18f),  // step 38
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.65f),  // step 40
    drum(INSTR_HIHAT, 2, 0.18f),  // step 42
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 2, 0.5f),  // step 44
    drum(INSTR_HIHAT, 2, 0.18f),  // step 46
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.75f),  // step 48
    drum(INSTR_HIHAT, 2, 0.18f),  // step 50
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 2, 0.55f),  // step 52
    drum(INSTR_HIHAT, 2, 0.18f),  // step 54
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_KICK, 2, 0.65f),  // step 56
    drum(INSTR_HIHAT, 2, 0.18f),  // step 58
    drum(INSTR_HIHAT, 0, 0.26f), drum(INSTR_SNARE, 0, 0.5f), drum(kLibDrumCrash, 2, 0.4f),  // step 60
    drum(INSTR_HIHAT, 0, 0.18f), drum(kLibDrumOpenHat, 2, 0.35f),  // step 62
};

// The Tool Suite pads every track to the pattern length so the tracks loop
// together; this is that guarantee, checked by the compiler.
constexpr float kCascadePatternBeats = steps(kCascadePatternSteps);
static_assert(beatsOf(kComplexMelodyNotes) == kCascadePatternBeats,
              "the complex lead must span exactly one 64-step pattern");
static_assert(beatsOf(kComplexHarmonyNotes) == kCascadePatternBeats,
              "the complex harmony must span exactly one 64-step pattern");
static_assert(beatsOf(kComplexBassNotes) == kCascadePatternBeats,
              "the complex bass must span exactly one 64-step pattern");
static_assert(beatsOf(kComplexPercussionNotes) == kCascadePatternBeats,
              "the complex drums must span exactly one 64-step pattern");

// Track duty is what a PULSE voice plays (the sequencer reads `MusicTrack::duty`),
// so the lead and the harmony carry the song's 12.5 % and 25 %.
constexpr MusicTrack kComplexMelodyTrack{kComplexMelodyNotes, countOf(kComplexMelodyNotes),
                                         true, WaveType::PULSE, 0.125f, nullptr, nullptr,
                                         nullptr};
constexpr MusicTrack kComplexHarmonyTrack{
    kComplexHarmonyNotes, countOf(kComplexHarmonyNotes), true, WaveType::PULSE, 0.25f,
    nullptr, nullptr, nullptr};
constexpr MusicTrack kComplexBassTrack{kComplexBassNotes, countOf(kComplexBassNotes), true,
                                       WaveType::TRIANGLE, 0.5f, nullptr, nullptr, nullptr};
constexpr MusicTrack kComplexPercussionTrack{
    kComplexPercussionNotes, countOf(kComplexPercussionNotes), true, WaveType::NOISE, 0.5f,
    nullptr, nullptr, nullptr};

// ---------------------------------------------------------------------------
// Programme description.
// ---------------------------------------------------------------------------

/** @brief One piece of the programme and the tracks it needs. */
struct Piece {
    const char* name;                                  ///< Name used in the serial lines.
    const MusicTrack* mainTrack;                       ///< MUSIC_PLAY's primary track.
    const MusicTrack* subTracks[AudioCommand::MAX_SUB_TRACKS];  ///< MUSIC_PLAY's sub-tracks.
    uint8_t subTrackCount;                             ///< Number of valid sub-track entries.
    float bpm;                                         ///< Tempo sent with MUSIC_SET_BPM.
};

constexpr Piece kPieces[] = {
    {"simple", &kSimpleTrack, {nullptr, nullptr, nullptr}, 0, kBpm},
    {"medium", &kMediumMelodyTrack, {&kMediumBassTrack, &kMediumHatTrack, nullptr}, 2, kBpm},
    {"complex", &kComplexMelodyTrack,
     {&kComplexHarmonyTrack, &kComplexBassTrack, &kComplexPercussionTrack}, 3, kCascadeBpm},
};

constexpr SoundProfile kProfiles[] = {SoundProfile::Legacy, SoundProfile::Nes,
                                      SoundProfile::Famicom};
constexpr const char* kProfileNames[] = {"Legacy", "Nes", "Famicom"};
/** @brief Uppercase piece names for the panel; the serial lines keep lower case. */
constexpr const char* kPieceLabels[] = {"SIMPLE", "MEDIUM", "COMPLEX"};
/** @brief Uppercase profile names for the panel. */
constexpr const char* kProfileLabels[] = {"LEGACY", "NES", "FAMICOM"};

// ---------------------------------------------------------------------------
// Engine wiring.
// ---------------------------------------------------------------------------

/// File scope on purpose: the backend carries a 1 KB render buffer and the
/// Arduino loop task stack is 8 KB (see the engine's audio guide).
#if defined(PLATFORM_NATIVE)
/// SDL2 opens the device with `bufferSize` samples, so 512 keeps the native
/// latency in the same range as the I2S backend's default block.
SDL2_AudioBackend gBackend(kSampleRate, 512);
#else
ESP32_I2S_AudioBackend gBackend(kI2sBclk, kI2sLrck, kI2sDout, kSampleRate);
#endif
AudioConfig gConfig(&gBackend, kSampleRate);

/**
 * @brief Largest absolute mixed sample of the block in progress.
 *
 * Written by the audio task through the post-mix hook, reset by the main loop at
 * every block boundary. Atomic because those are two different tasks and a torn
 * read would report a peak that never happened.
 */
std::atomic<int32_t> gBlockPeak{0};

/**
 * @brief FNV-1a hash of everything the mix produced during the current block.
 *
 * `peak` cannot tell a block that replayed the same audio from one that changed
 * profile: two different waveforms can share a peak. The hash is what makes the
 * listening record defensible - if two profiles report the same hash for the
 * same piece, the profile did not reach the renderer and the ear is being asked
 * about nothing. Audio thread only; the main thread reads it between blocks.
 */
std::atomic<uint32_t> gBlockHash{2166136261u};

/**
 * @brief Post-mix hook: tracks the peak of the buffer the audio task just mixed.
 *
 * `AudioEngine` copies its `AudioConfig` in the constructor, so setting
 * `gConfig.postMixMono` after construction never reaches the copy `init()`
 * reads. Attaching it to the scheduler's own core after `init()` is the only
 * path guaranteed to land on the instance the audio task calls.
 * @param mono Mixed mono buffer; owned by the audio task.
 * @param length Number of samples in the buffer.
 * @param user Unused.
 */
void postMixMono(int16_t* mono, int length, void* user)
{
    (void)user;
    if (mono == nullptr || length <= 0) {
        return;
    }
    int32_t peak = 0;
    uint32_t hash = gBlockHash.load(std::memory_order_relaxed);
    for (int i = 0; i < length; ++i) {
        const int32_t value = mono[i] < 0 ? -static_cast<int32_t>(mono[i]) : mono[i];
        if (value > peak) {
            peak = value;
        }
        // FNV-1a over the mixed samples: cheap, order sensitive and good enough
        // to tell two renders apart.
        hash ^= static_cast<uint32_t>(static_cast<uint16_t>(mono[i]));
        hash *= 16777619u;
    }
    gBlockHash.store(hash, std::memory_order_relaxed);
    int32_t current = gBlockPeak.load(std::memory_order_relaxed);
    while (peak > current &&
           !gBlockPeak.compare_exchange_weak(current, peak, std::memory_order_relaxed)) {
    }
}

AudioEngine gEngine(gConfig);

// ---------------------------------------------------------------------------
// Status panel.
//
// Used directly instead of through the engine's Renderer: this is one static
// screen per block, not a game, and pulling the renderer/scene/input stack in
// would cost flash and bring configuration the harness does not need.
// ---------------------------------------------------------------------------

/** @brief One drawing target for the whole firmware. */
#if !defined(PLATFORM_NATIVE)
TFT_eSPI gTft;
#endif

/**
 * @brief True only after a completed display init.
 *
 * `TFT_eSPI::init()` returns void, so there is no failure code to branch on and
 * no panel probe is safe to trust (a read can fail on a healthy panel and would
 * then disable a working screen). What this flag does guarantee is that no draw
 * is ever attempted before the bus and controller were configured, and every
 * draw returns immediately while it is false: a missing panel costs the screen
 * and nothing else.
 */
bool gDisplayReady = false;

/** @brief Per-block numbers carried from the report to the next block's panel. */
struct BlockStats {
    uint32_t underruns = 0;      ///< Backend underrun delta of the block.
    int32_t peak = 0;            ///< Largest absolute mixed sample.
    uint32_t hash = 0;           ///< FNV-1a of the block's mixed samples.
    bool valid = false;          ///< False until at least one block has ended.
};

BlockStats gLastStats;

// ---------------------------------------------------------------------------
// Block state.
// ---------------------------------------------------------------------------

uint8_t gBlockIndex = 0;            ///< 0..8, printed as 1..9.
uint8_t gPieceIndex = 0;            ///< Index into `kPieces` for the current block.
uint32_t gBlockStartMs = 0;         ///< `millis()` at the current block's start.
#if !defined(PLATFORM_NATIVE)
uint32_t gUnderrunsAtBlockStart = 0;    ///< Backend counter snapshot, block start.
uint32_t gWorstRenderAtBlockStart = 0;  ///< Backend counter snapshot, block start.
bool gLivenessChecked = false;      ///< One-shot I2S failure warning.
#endif

// ---------------------------------------------------------------------------
// Helpers.
// ---------------------------------------------------------------------------

/**
 * @brief Validates a track before it is handed to the sequencer.
 *
 * The tables are static, so this only ever fires on a typo in this file, but the
 * sequencer dereferences `notes` and walks `count` entries: a bad pointer is a
 * crash, not a wrong note, and a listening harness must not crash mid-programme.
 * @param track Track to check.
 * @return true when the track is safe to play.
 */
bool isValidTrack(const MusicTrack* track)
{
    if (track == nullptr || track->notes == nullptr) {
        return false;
    }
    if (track->count == 0 || track->count > kMaxTrackNotes) {
        return false;
    }
    if (static_cast<uint8_t>(track->channelType) > static_cast<uint8_t>(WaveType::SAW)) {
        return false;
    }
    return true;
}

/**
 * @brief Validates a piece's whole track set before it is submitted.
 * @param piece Piece to check.
 * @return true when the main track and every declared sub-track are playable.
 */
bool isValidPiece(const Piece& piece)
{
    if (!isValidTrack(piece.mainTrack)) {
        return false;
    }
    if (piece.subTrackCount > AudioCommand::MAX_SUB_TRACKS) {
        return false;
    }
    for (uint8_t i = 0; i < piece.subTrackCount; ++i) {
        if (!isValidTrack(piece.subTracks[i])) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Hands one command to the engine.
 * @param cmd Command to enqueue.
 */
void submit(const AudioCommand& cmd)
{
    gEngine.submitCommand(cmd);
}

/**
 * @brief Colour the profile is coded with on the panel.
 *
 * The three colours are the "read it from across the room" part of the screen,
 * so they are fixed here and not derived from the theme: white is the flat
 * reference, green the stepped-NES profile and cyan the Famicom one (the same
 * character through the Famicom's gentler output stage).
 * @param profileIndex Index into `kProfiles`.
 * @return RGB565 colour for that profile.
 */
#if !defined(PLATFORM_NATIVE)
uint16_t profileColor(uint8_t profileIndex)
{
    switch (profileIndex) {
        case 1:
            return TFT_GREEN;
        case 2:
            return TFT_CYAN;
        case 0:
        default:
            return TFT_WHITE;
    }
}

/**
 * @brief Configures the panel, if there is one.
 *
 * Called after the first block is already playing, so nothing here can delay
 * the audio. `init()` returns void and shifts its commands out over SPI without
 * waiting for an answer, so it cannot block on an absent panel either.
 * @return true when the panel reported the expected logical size.
 */
bool initDisplay()
{
    gTft.init();
    gTft.setRotation(kDisplayRotation);

    // The dimension check catches a mis-set driver or rotation, which would make
    // the fixed 320x240 layout overflow. It says nothing about an absent panel:
    // that case still "succeeds" and simply draws into nothing.
    const int width = gTft.width();
    const int height = gTft.height();
    if (width != kScreenWidth || height != kScreenHeight) {
        Serial.printf("# WARN display reports %dx%d, expected %dx%d; screen disabled\n", width,
                      height, kScreenWidth, kScreenHeight);
        Serial.flush();
        return false;
    }
    gTft.fillScreen(TFT_BLACK);
    return true;
}

/**
 * @brief Repaints the whole status screen for the block that is starting.
 *
 * Called once per block and never from the audio path. The SPI burst is one
 * screen of work (a few tens of milliseconds at 55 MHz), and it runs on the main
 * task while the audio task keeps feeding I2S on its own, so a redraw cannot
 * starve DMA; that is also why there is no animation and no delay here.
 */
void drawStatusScreen()
{
    if (!gDisplayReady || gTft.width() != kScreenWidth || gTft.height() != kScreenHeight) {
        return;
    }

    const uint8_t profileIndex = static_cast<uint8_t>(gBlockIndex % kProfileCount);
    const uint16_t colour = profileColor(profileIndex);
    const int centreX = kScreenWidth / 2;

    gTft.fillScreen(TFT_BLACK);
    gTft.setTextDatum(TL_DATUM);
    gTft.setTextSize(1);

    // Alive line first: it is on screen even before the first report exists,
    // so a black panel is a display problem and never mistaken for a dead board.
    gTft.setTextFont(1);
    gTft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    gTft.drawString("APU LISTEN  22050 Hz  I2S 5/21/19", 8, 4);

    gTft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    gTft.drawString("PIECE", 8, 18);

    // 26 px letters at x2: blocky enough to read across a room, and the built-in
    // font 6 is digits-only, so the height has to come from scaling, not fatter
    // glyphs.
    gTft.setTextDatum(TC_DATUM);
    gTft.setTextFont(4);
    gTft.setTextSize(2);
    gTft.setTextColor(TFT_WHITE, TFT_BLACK);
    gTft.drawString(kPieceLabels[gPieceIndex], centreX, 28);

    gTft.setTextDatum(TL_DATUM);
    gTft.setTextSize(1);
    gTft.setTextFont(1);
    gTft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    gTft.drawString("PROFILE", 8, 86);

    gTft.setTextDatum(TC_DATUM);
    gTft.setTextFont(4);
    gTft.setTextSize(3);
    gTft.setTextColor(colour, TFT_BLACK);
    gTft.drawString(kProfileLabels[profileIndex], centreX, 96);

    // The bar repeats the profile colour in a shape that reads without letters.
    gTft.fillRect(0, 178, kScreenWidth, 6, colour);

    gTft.setTextDatum(TL_DATUM);
    gTft.setTextSize(1);
    gTft.setTextFont(2);
    gTft.setTextColor(TFT_WHITE, TFT_BLACK);
    char line[48];
    std::snprintf(line, sizeof(line), "BLOCK %u/%u",
                  static_cast<unsigned>(gBlockIndex) + 1u,
                  static_cast<unsigned>(kBlockCount));
    gTft.drawString(line, 8, 190);

    gTft.setTextFont(1);
    gTft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    if (gLastStats.valid) {
        std::snprintf(line, sizeof(line), "PREV UNDERRUNS %lu   PEAK %ld",
                      static_cast<unsigned long>(gLastStats.underruns),
                      static_cast<long>(gLastStats.peak));
        gTft.drawString(line, 8, 210);
        std::snprintf(line, sizeof(line), "PREV HASH %08lX",
                      static_cast<unsigned long>(gLastStats.hash));
        gTft.drawString(line, 8, 221);
    } else {
        gTft.drawString("PREV none yet - first block", 8, 210);
    }

    gTft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    gTft.drawString("prev = the block that just ended; serial is the record", 8, 230);
}
#else
// The PC build has no panel. The transcript is the record there, and the display
// stays off rather than pretending to draw: `initDisplay()` reports failure and
// the caller says so once.
bool initDisplay() { return false; }
void drawStatusScreen() {}
#endif

/**
 * @brief Prints the block banner and starts everything the block needs.
 * @param blockIndex 0-based programme position (piece x profile).
 */
void startBlock(uint8_t blockIndex)
{
    if (blockIndex >= kBlockCount) {
        blockIndex = 0;
    }
    gBlockIndex = blockIndex;
    gPieceIndex = static_cast<uint8_t>(blockIndex / kProfileCount);
    const uint8_t profileIndex = static_cast<uint8_t>(blockIndex % kProfileCount);
    const Piece& piece = kPieces[gPieceIndex];

    // Banner first: the human must be able to associate what he is about to
    // hear with the line already on screen.
    Serial.printf("# LISTEN piece=%s profile=%s block=%u of %u\n", piece.name,
                  kProfileNames[profileIndex], static_cast<unsigned>(blockIndex) + 1u,
                  static_cast<unsigned>(kBlockCount));
    Serial.flush();

    gBlockPeak.store(0, std::memory_order_relaxed);
    gBlockHash.store(2166136261u, std::memory_order_relaxed);
#if !defined(PLATFORM_NATIVE)
    gUnderrunsAtBlockStart = gBackend.getUnderrunCount();
    gWorstRenderAtBlockStart = gBackend.getWorstRenderUs();
#endif

    // Hard cut at the boundary: MUSIC_STOP clears the sequencer, but the voices
    // it owns keep their remaining gate, so they are reset explicitly. The click
    // that makes is also the audible marker of a block change.
    AudioCommand stopMusic{};
    stopMusic.type = AudioCommandType::MUSIC_STOP;
    submit(stopMusic);
    for (uint8_t channel = 0; channel < static_cast<uint8_t>(ApuCore::MAX_VOICES);
         ++channel) {
        AudioCommand stopVoice{};
        stopVoice.type = AudioCommandType::STOP_CHANNEL;
        stopVoice.channelIndex = channel;
        submit(stopVoice);
    }

    // Each piece carries its tempo (the complex piece keeps its source song's
    // 150 BPM); MUSIC_PLAY derives the tick length from it, so it goes first.
    AudioCommand tempo{};
    tempo.type = AudioCommandType::MUSIC_SET_BPM;
    tempo.bpm = piece.bpm;
    submit(tempo);

    // The profile is audio-thread state; the command is the documented path from
    // here, and it is queued before MUSIC_PLAY so the first note already sounds
    // under the new profile.
    AudioCommand profile{};
    profile.type = AudioCommandType::SET_SOUND_PROFILE;
    profile.soundProfile = static_cast<uint8_t>(kProfiles[profileIndex]);
    submit(profile);

    if (isValidPiece(piece)) {
        AudioCommand music{};
        music.type = AudioCommandType::MUSIC_PLAY;
        music.track = piece.mainTrack;
        music.subTrackCount = piece.subTrackCount;
        for (uint8_t i = 0; i < piece.subTrackCount; ++i) {
            music.subTracks[i] = piece.subTracks[i];
        }
        submit(music);
    } else {
        Serial.printf("# ERROR piece=%s has an invalid track table; playing silence\n",
                      piece.name);
        Serial.flush();
    }

    // The audio commands are already queued; the panel is painted while the
    // audio task starts the block (a screen-less board simply returns here).
    // Every timed voice is a sequencer track, so nothing musical depends on this
    // timestamp: it only measures the block's wall-clock listening length.
    drawStatusScreen();

    gBlockStartMs = millis();
}

/**
 * @brief Prints the report for the block that just ended.
 *
 * The backend counters are monotonic, so the block value is the delta from the
 * snapshot taken at the block start. `worst_render_us` is a running maximum, so
 * its delta is the increase (0 means no new maximum was recorded in the block).
 */
void reportBlock()
{
#if defined(PLATFORM_NATIVE)
    // The SDL2 backend exposes no underrun or render-time counters: it hands the
    // callback's sample count straight to `generateSamples`. The fields are kept
    // so the transcript shape stays the same on both builds; `peak` and `hash`
    // are the numbers that mean something here.
    const uint32_t underruns = 0;
    const uint32_t worstRender = 0;
#else
    const uint32_t underrunsNow = gBackend.getUnderrunCount();
    const uint32_t worstRenderNow = gBackend.getWorstRenderUs();
    const uint32_t underruns = underrunsNow - gUnderrunsAtBlockStart;
    const uint32_t worstRender = (worstRenderNow > gWorstRenderAtBlockStart)
                                     ? (worstRenderNow - gWorstRenderAtBlockStart)
                                     : 0u;
#endif
    const long peak = static_cast<long>(gBlockPeak.load(std::memory_order_relaxed));
    const unsigned long hash = static_cast<unsigned long>(
        gBlockHash.load(std::memory_order_relaxed));

    // Keep the same numbers for the next block's panel, so the screen and the
    // serial report can never disagree.
    gLastStats.underruns = underruns;
    gLastStats.peak = static_cast<int32_t>(peak);
    gLastStats.hash = static_cast<uint32_t>(hash);
    gLastStats.valid = true;

    Serial.printf("REPORT piece=%s profile=%s underruns=%lu worst_render_us=%lu peak=%ld hash=%08lX\n",
                  kPieces[gPieceIndex].name,
                  kProfileNames[gBlockIndex % kProfileCount],
                  static_cast<unsigned long>(underruns),
                  static_cast<unsigned long>(worstRender), peak, hash);
    Serial.flush();
}

/**
 * @brief Warns once if the I2S backend never rendered a block.
 *
 * A failed `i2s_driver_install` / `i2s_set_pin` makes the backend return before
 * creating its task; every counter then reads zero forever and the report would
 * look like a healthy run. One warning is enough to send the listener to the
 * wiring instead of to the profiles.
 */
void checkBackendLiveness()
{
#if defined(PLATFORM_NATIVE)
    // Nothing to check: SDL2 either opened the device or `init()` reported it.
#else
    if (gLivenessChecked || millis() < 3000u) {
        return;
    }
    gLivenessChecked = true;
    if (gBackend.getWorstBlockUs() == 0u) {
        Serial.printf("# WARN backend never iterated: i2s init failed? pins=%d/%d/%d\n",
                      kI2sBclk, kI2sLrck, kI2sDout);
        Serial.flush();
    }
#endif
}

/**
 * @brief Refuses to start on a render block the backend cannot honour.
 *
 * The backend derives its DMA depth and its fixed render buffer from the block
 * size, so a bad value is not degraded audio: it is an init failure that would
 * render the whole harness meaningless.
 */
void validateBlockSize()
{
    const int size = gConfig.blockSize;
    if (size > 0 && size <= pixelroot32::audio::PR32_AUDIO_MAX_BLOCK_SAMPLES &&
        (size % 128) == 0) {
        return;
    }
    Serial.printf("# ERROR invalid block size %d (expected a positive multiple of 128 up to %d)\n",
                  size, pixelroot32::audio::PR32_AUDIO_MAX_BLOCK_SAMPLES);
    Serial.flush();
    while (true) {
        delay(1000);
    }
}

/**
 * @brief Connects the peak meter to the APU instance the audio task renders with.
 */
void attachPeakMeter()
{
    AudioScheduler* scheduler = gEngine.getScheduler();
    if (scheduler == nullptr) {
        Serial.println("# ERROR no scheduler: peak will read 0");
        Serial.flush();
        return;
    }
    scheduler->getApuCore().setPostMixMono(&postMixMono, nullptr);
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500);

    Serial.println();
#if defined(PLATFORM_NATIVE)
    Serial.printf("# apu_listen rate=%d backend=sdl2 block_ms=%u\n", kSampleRate,
                  static_cast<unsigned>(kBlockMs));
#else
    Serial.printf("# apu_listen rate=%d pins=%d/%d/%d block_ms=%u\n", kSampleRate, kI2sBclk,
                  kI2sLrck, kI2sDout, static_cast<unsigned>(kBlockMs));
#endif
    Serial.printf("# order piece=simple,medium,complex x profile=Legacy,Nes,Famicom; "
                  "repeats forever\n");
    Serial.println(
        "# profiles: Legacy=flat reference, Nes=stepped duty+unipolar noise, "
        "Famicom=Nes character+37 Hz HPF output stage");
    Serial.printf("# sizes MusicNote=%u MusicTrack=%u AudioEvent=%u AudioCommand=%u\n",
                  static_cast<unsigned>(sizeof(MusicNote)),
                  static_cast<unsigned>(sizeof(MusicTrack)),
                  static_cast<unsigned>(sizeof(AudioEvent)),
                  static_cast<unsigned>(sizeof(AudioCommand)));
    Serial.flush();

    validateBlockSize();

    gEngine.init();
    attachPeakMeter();

    // Audio before the panel: the display is optional, so nothing about it may
    // stand between the engine starting and the first block being queued. The
    // first screen is painted right after, once the panel has been configured.
    startBlock(0);

    gDisplayReady = initDisplay();
    if (gDisplayReady) {
        drawStatusScreen();
#if defined(PLATFORM_NATIVE)
        Serial.println("# display ILI9341 is the board build; see README");
#else
        Serial.printf("# display ILI9341 %dx%d rot=%u, one repaint per block\n",
                      kScreenWidth, kScreenHeight,
                      static_cast<unsigned>(kDisplayRotation));
#endif
    } else {
        Serial.println("# display disabled; audio continues without a screen");
    }
    Serial.flush();
}

void loop()
{
    if (static_cast<uint32_t>(millis() - gBlockStartMs) >= kBlockMs) {
        reportBlock();
        startBlock(static_cast<uint8_t>((gBlockIndex + 1u) % kBlockCount));
        return;
    }

    checkBackendLiveness();
    delay(2);
}

#if defined(PLATFORM_NATIVE)
/**
 * @brief Entry point for the PC build.
 *
 * The Arduino core calls `setup()` once and `loop()` forever; a PC has no core, so
 * the same two functions are driven from here. `[base_native]` sets
 * `SDL_MAIN_HANDLED`, which is what makes a plain `main()` legal with SDL2.
 */
int main()
{
    setup();
    while (true) {
        loop();
    }
}
#endif
