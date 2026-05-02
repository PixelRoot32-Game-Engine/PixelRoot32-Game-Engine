#pragma once
#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>

/**
 * NES-style threat BGM: pulse bass (C–G–F–G), pulse harmony on fifths, noise drums.
 * Durations are in beats (quarter = 1.0) per ApuCore sequencer.
 */
namespace spaceinvaders::tracks {

namespace a = pixelroot32::audio;

static constexpr a::InstrumentPreset BASS = a::INSTR_PULSE_BASS;
static constexpr float S = 0.25f;
static constexpr float E = 0.5f;
static constexpr float Q = 1.0f;

// --- Bass: alternating sixteenth + rest (“machine tread”), 4 beats per chord × 16 beats loop ---
// Expanded manually; 8 pulses per chord bar.
#define SI_HR(N) a::makeNote(BASS, a::Note::N, S), a::makeRest(S)

static const a::MusicNote BGM_SLOW_NOTES[] = {
    // C
    SI_HR(C),
    SI_HR(C),
    SI_HR(C),
    SI_HR(C),
    SI_HR(C),
    SI_HR(C),
    SI_HR(C),
    SI_HR(C),
    // G
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    // F
    SI_HR(F),
    SI_HR(F),
    SI_HR(F),
    SI_HR(F),
    SI_HR(F),
    SI_HR(F),
    SI_HR(F),
    SI_HR(F),
    // G
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
    SI_HR(G),
};
#undef SI_HR

#define SI_STAC(N) a::makeNote(BASS, a::Note::N, S)

static const a::MusicNote BGM_MEDIUM_NOTES[] = {
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(C),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(F),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
    SI_STAC(G),
};
#undef SI_STAC

// Drum chunk helpers: 32 eighths = 16 beats (looped x1)
#define DK a::makeNote(a::INSTR_KICK, a::Note::Rest, 1, E)
#define DH a::makeNote(a::INSTR_HIHAT, a::Note::Rest, 1, E)
#define DS a::makeNote(a::INSTR_SNARE, a::Note::Rest, 1, E)

static const a::MusicNote DRUM_SLOW_NOTES[] = {
    DK, DH, DH, DS, DH, DK, DH, DS, DK, DH, DH, DS, DK, DK, DH, DS, DH, DH, DK, DS, DK, DK, DH, DS,
    DK, DK, DK, DK, DK, DK, DK, DK,
};

static const a::MusicTrack DRUM_SLOW = {
    DRUM_SLOW_NOTES,
    sizeof(DRUM_SLOW_NOTES) / sizeof(a::MusicNote),
    true,
    a::WaveType::NOISE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const a::MusicNote DRUM_MEDIUM_NOTES[] = {
    DK,
    DK,
    DH,
    DS,
    DH,
    DK,
    DH,
    DS,
    DK,
    DH,
    DK,
    DS,
    DH,
    DK,
    DH,
    DS,
    DK,
    DH,
    DH,
    DS,
    DH,
    DK,
    DH,
    DS,
    DK,
    DH,
    DH,
    DS,
    DK,
    DK,
    DH,
    DS,
};

static const a::MusicTrack DRUM_MEDIUM = {
    DRUM_MEDIUM_NOTES,
    sizeof(DRUM_MEDIUM_NOTES) / sizeof(a::MusicNote),
    true,
    a::WaveType::NOISE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const a::MusicNote DRUM_FAST_NOTES[] = {
    DK, DH, DK, DH, DS, DH, DK, DH, DK, DH, DK, DH, DS, DH, DK, DK, DK, DH, DK, DH, DS, DH, DK,
    DH, DK, DH, DK, DH, DK, DH, DK, DK};

#undef DK
#undef DH
#undef DS

static const a::MusicTrack DRUM_FAST = {
    DRUM_FAST_NOTES,
    sizeof(DRUM_FAST_NOTES) / sizeof(a::MusicNote),
    true,
    a::WaveType::NOISE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const a::MusicTrack BGM_SLOW_TRACK = {
    BGM_SLOW_NOTES,
    sizeof(BGM_SLOW_NOTES) / sizeof(a::MusicNote),
    true,
    a::WaveType::PULSE,
    BASS.duty,
    nullptr,
    nullptr,
    &DRUM_SLOW,
};

static const a::MusicTrack BGM_MEDIUM_TRACK = {
    BGM_MEDIUM_NOTES,
    sizeof(BGM_MEDIUM_NOTES) / sizeof(a::MusicNote),
    true,
    a::WaveType::PULSE,
    BASS.duty,
    nullptr,
    nullptr,
    &DRUM_MEDIUM,
};

static const a::MusicTrack BGM_FAST_TRACK = {
    BGM_MEDIUM_NOTES,
    sizeof(BGM_MEDIUM_NOTES) / sizeof(a::MusicNote),
    true,
    a::WaveType::PULSE,
    BASS.duty,
    nullptr,
    nullptr,
    &DRUM_FAST,
};

static const a::MusicNote WIN_NOTES[] = {
    a::makeNote(a::INSTR_PULSE_LEAD, a::Note::C, Q),
    a::makeNote(a::INSTR_PULSE_LEAD, a::Note::E, Q),
    a::makeNote(a::INSTR_PULSE_LEAD, a::Note::G, Q),
    a::makeNote(a::INSTR_PULSE_LEAD, a::Note::C, 5, Q * 1.5f),
    a::makeRest(E),
};

static const a::MusicTrack WIN_TRACK = {
    WIN_NOTES,
    sizeof(WIN_NOTES) / sizeof(a::MusicNote),
    false,
    a::WaveType::PULSE,
    a::INSTR_PULSE_LEAD.duty};

static const a::MusicNote GAME_OVER_NOTES[] = {
    a::makeNote(BASS, a::Note::G, Q),
    a::makeNote(BASS, a::Note::Fs, Q),
    a::makeNote(BASS, a::Note::E, Q),
    a::makeNote(BASS, a::Note::D, Q),
    a::makeNote(BASS, a::Note::C, Q * 2),
    a::makeRest(Q),
};

static const a::MusicTrack GAME_OVER_TRACK = {
    GAME_OVER_NOTES,
    sizeof(GAME_OVER_NOTES) / sizeof(a::MusicNote),
    false,
    a::WaveType::PULSE,
    BASS.duty};

} // namespace spaceinvaders::tracks

// Backward-compatible aliases for scene code expecting global static names:
static const pixelroot32::audio::MusicTrack& BGM_SLOW_TRACK = spaceinvaders::tracks::BGM_SLOW_TRACK;
static const pixelroot32::audio::MusicTrack& BGM_MEDIUM_TRACK = spaceinvaders::tracks::BGM_MEDIUM_TRACK;
static const pixelroot32::audio::MusicTrack& BGM_FAST_TRACK = spaceinvaders::tracks::BGM_FAST_TRACK;
static const pixelroot32::audio::MusicTrack& WIN_TRACK = spaceinvaders::tracks::WIN_TRACK;
static const pixelroot32::audio::MusicTrack& GAME_OVER_TRACK = spaceinvaders::tracks::GAME_OVER_TRACK;
