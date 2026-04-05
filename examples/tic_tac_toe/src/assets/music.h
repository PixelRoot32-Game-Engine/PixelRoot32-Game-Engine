#pragma once
#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>

static const pixelroot32::audio::MusicNote BG_MELODY[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_PAD, pixelroot32::audio::Note::C, 0.6f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_PAD, pixelroot32::audio::Note::G, 0.6f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_PAD, pixelroot32::audio::Note::E, 0.6f),
    pixelroot32::audio::makeRest(0.3f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_PAD, pixelroot32::audio::Note::D, 0.6f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_PAD, pixelroot32::audio::Note::A, 0.6f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_TRIANGLE_PAD, pixelroot32::audio::Note::F, 0.6f),
    pixelroot32::audio::makeRest(0.4f)
};

static const pixelroot32::audio::MusicTrack BG_MUSIC = {
    BG_MELODY,
    sizeof(BG_MELODY) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::TRIANGLE,
    0.5f
};

static const pixelroot32::audio::MusicNote WIN_MELODY[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 0.18f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 0.18f),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::G, 0.30f)
};

static const pixelroot32::audio::MusicTrack WIN_MUSIC = {
    WIN_MELODY,
    sizeof(WIN_MELODY) / sizeof(pixelroot32::audio::MusicNote),
    false,
    pixelroot32::audio::WaveType::PULSE,
    0.5f
};
