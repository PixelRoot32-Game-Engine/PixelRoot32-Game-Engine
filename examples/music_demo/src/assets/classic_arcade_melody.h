#pragma once

#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

#include "common_melodies.h"

namespace musicdemo {

// =============================================================================
// MELODY 1 — Classic Arcade — 8 bars / 32 beats: A | A' | B | B'
// =============================================================================

static const pr32::audio::MusicNote sClassicArcadeLeadNotes[] = {
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 4, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::Ds, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 4, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 4, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, E),
    pr32::audio::makeRest(E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, Q),
    pr32::audio::makeRest(E),
};

static const pr32::audio::MusicNote sClassicArcadeBassNotes[] = {
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::A, 2, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::A, 2, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::C, 3, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::C, 3, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 3, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 3, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::A, 2, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::A, 2, W),
};

static const pr32::audio::MusicNote sClassicArcadeHarmonyNotes[] = {
    pr32::audio::makeRest(W),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::G, 4, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeRest(W),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::C, 5, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::A, 4, Q),
    pr32::audio::makeRest(W),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::G, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeRest(H),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::E, 4, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::G, 4, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeRest(W),
    pr32::audio::makeRest(H),
};

static const pr32::audio::MusicNote sClassicArcadeDrumsNotes[] = {
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeRest(E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, S),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, S),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
};

static const pr32::audio::MusicTrack sClassicArcadeLead = {
    sClassicArcadeLeadNotes,
    sizeof(sClassicArcadeLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sClassicArcadeBass = {
    sClassicArcadeBassNotes,
    sizeof(sClassicArcadeBassNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::TRIANGLE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sClassicArcadeHarmony = {
    sClassicArcadeHarmonyNotes,
    sizeof(sClassicArcadeHarmonyNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.125f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sClassicArcadeDrums = {
    sClassicArcadeDrumsNotes,
    sizeof(sClassicArcadeDrumsNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::NOISE,
    0.0f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sClassicArcadeTrack = {
    sClassicArcadeLeadNotes,
    sizeof(sClassicArcadeLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    kDemoArcadeLeadWave,
    0.5f,
    &sClassicArcadeBass,
    &sClassicArcadeHarmony,
    &sClassicArcadeDrums};

}
