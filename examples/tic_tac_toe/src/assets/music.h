#pragma once
#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>

// Durations in beats (quarter = 1.0; ApuCore TICKS_PER_BEAT = 4).
// NES-style layering: pulse arpeggio + triangle bass + noise drums.

static constexpr float kTttS = 0.25f;
static constexpr float kTttE = 0.5f;
static constexpr float kTttQ = 1.0f;
static constexpr float kTttH = 2.0f;

// --- BGM: 8-beat loop — Cmaj arp | Dm arp ------------------------------------------------

static const pixelroot32::audio::MusicNote kTttBgLeadNotes[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::G, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::G, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::D, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::F, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::A, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::F, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::D, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::F, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::A, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::F, 5, kTttS),
};

static const pixelroot32::audio::MusicNote kTttBgDrumNotes[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_KICK, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_HIHAT, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_SNARE, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_HIHAT, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_KICK, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_HIHAT, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_SNARE, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_HIHAT, pixelroot32::audio::Note::Rest, kTttQ),
};

static const pixelroot32::audio::MusicTrack kTttBgDrumTrack = {
    kTttBgDrumNotes,
    sizeof(kTttBgDrumNotes) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::NOISE,
    0.0f,
    nullptr,
    nullptr,
    nullptr,
};

static const pixelroot32::audio::MusicTrack BG_MUSIC = {
    kTttBgLeadNotes,
    sizeof(kTttBgLeadNotes) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::TRIANGLE,
    0.25f,
    nullptr,
    nullptr,
    &kTttBgDrumTrack,
};

// --- Win: 4-beat fanfare + drums ---------------------------------------------------------

static const pixelroot32::audio::MusicNote kTttWinLeadNotes[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::G, 5, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 6, kTttS),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::E, 5, kTttE),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::G, 5, kTttE),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_PULSE_LEAD, pixelroot32::audio::Note::C, 6, kTttH),
};

static const pixelroot32::audio::MusicNote kTttWinDrumNotes[] = {
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_KICK, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_HIHAT, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_SNARE, pixelroot32::audio::Note::Rest, kTttQ),
    pixelroot32::audio::makeNote(pixelroot32::audio::INSTR_KICK, pixelroot32::audio::Note::Rest, kTttQ),
};

static const pixelroot32::audio::MusicTrack kTttWinDrumTrack = {
    kTttWinDrumNotes,
    sizeof(kTttWinDrumNotes) / sizeof(pixelroot32::audio::MusicNote),
    false,
    pixelroot32::audio::WaveType::NOISE,
    0.0f,
    nullptr,
    nullptr,
    nullptr,
};

static const pixelroot32::audio::MusicTrack WIN_MUSIC = {
    kTttWinLeadNotes,
    sizeof(kTttWinLeadNotes) / sizeof(pixelroot32::audio::MusicNote),
    false,
    pixelroot32::audio::WaveType::PULSE,
    0.5f,
    nullptr,
    nullptr,
    &kTttWinDrumTrack,
};
