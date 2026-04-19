#pragma once
#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>

// Relaxed background melody for tic-tac-toe - gentle and soothing
static const pixelroot32::audio::MusicNote BG_MELODY[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_HARMONY, pixelroot32::audio::Note::C, 4, 0.6f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_HARMONY, pixelroot32::audio::Note::E, 4, 0.4f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_HARMONY, pixelroot32::audio::Note::G, 4, 0.6f),
    pixelroot32::audio::makeRest(0.3f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_HARMONY, pixelroot32::audio::Note::A, 4, 0.4f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_HARMONY, pixelroot32::audio::Note::G, 4, 0.4f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_HARMONY, pixelroot32::audio::Note::E, 4, 0.6f),
    pixelroot32::audio::makeRest(0.5f)
};

// Gentle bass accompaniment
static const pixelroot32::audio::MusicNote BG_BASS[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_BASS, pixelroot32::audio::Note::C, 2, 1.2f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_BASS, pixelroot32::audio::Note::G, 2, 1.2f)
};

static const pixelroot32::audio::MusicTrack BG_BASS_TRACK = {
    BG_BASS,
    sizeof(BG_BASS) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::TRIANGLE,
    0.5f
};

static const pixelroot32::audio::MusicTrack BG_MUSIC = {
    BG_MELODY,
    sizeof(BG_MELODY) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::PULSE,
    0.125f,  // Using harmony duty cycle
    &BG_BASS_TRACK  // Add bass accompaniment
};

// Improved victory melody - fuller chord progression with arpeggio
static const pixelroot32::audio::MusicNote WIN_MELODY[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 4, 0.15f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 4, 0.15f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::G, 4, 0.15f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 5, 0.3f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::B, 4, 0.15f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::G, 4, 0.15f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 4, 0.15f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 4, 0.3f)
};

// Percussion for victory sound
static const pixelroot32::audio::MusicNote WIN_PERC[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_KICK, pixelroot32::audio::Note::Rest, 0.1f),
    pixelroot32::audio::makeRest(0.2f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_SNARE, pixelroot32::audio::Note::Rest, 0.1f),
    pixelroot32::audio::makeRest(0.2f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_KICK, pixelroot32::audio::Note::Rest, 0.1f)
};

static const pixelroot32::audio::MusicTrack WIN_PERC_TRACK = {
    WIN_PERC,
    sizeof(WIN_PERC) / sizeof(pixelroot32::audio::MusicNote),
    false,
    pixelroot32::audio::WaveType::NOISE,
    0.0f
};

static const pixelroot32::audio::MusicTrack WIN_MUSIC = {
    WIN_MELODY,
    sizeof(WIN_MELODY) / sizeof(pixelroot32::audio::MusicNote),
    false,
    pixelroot32::audio::WaveType::PULSE,
    0.5f,
    nullptr,  // No second voice
    nullptr,  // No third voice
    &WIN_PERC_TRACK  // Add percussion
};
