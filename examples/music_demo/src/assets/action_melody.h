#pragma once

#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

#include "common_melodies.h"

namespace musicdemo {

// =============================================================================
// MELODY 3 — Action — 8 bars / 32 beats + break
// =============================================================================

#define ARP_EM() \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::E, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::G, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::B, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::E, 6, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::B, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::G, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::E, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::G, 5, S)

#define ARP_DM() \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::D, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::Fs, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::A, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::D, 6, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::A, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::Fs, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::D, 5, S), \
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::E, 5, S)

static const pr32::audio::MusicNote sActionLeadNotes[] = {
    ARP_EM(),
    ARP_EM(),
    ARP_EM(),
    ARP_EM(),
    ARP_DM(),
    ARP_DM(),
    ARP_DM(),
    ARP_DM(),
    ARP_EM(),
    ARP_EM(),
    ARP_EM(),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::B, 5, E),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::G, 5, E),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeRest(H),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::G, 5, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::B, 5, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::D, 6, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::G, 6, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::Fs, 5, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::D, 5, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::A, 5, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::D, 6, S),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeNote(DEMO_SNES_LEAD_TIGHT, pr32::audio::Note::G, 4, Q),
    pr32::audio::makeRest(Q),
};

#undef ARP_EM
#undef ARP_DM

static const pr32::audio::MusicNote sActionBassNotes[] = {
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::F, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::F, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeRest(H),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 3, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 3, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::A, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::A, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, E),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::B, 1, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeRest(Q),
};

static const pr32::audio::MusicNote sActionHarmonyNotes[] = {
    pr32::audio::makeRest(W),
    pr32::audio::makeRest(W),
    pr32::audio::makeRest(W),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::G, 5, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::D, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::Fs, 5, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeRest(W),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::G, 5, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_HARMONY, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeRest(Q),
};

static const pr32::audio::MusicNote sActionDrumsNotes[] = {
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, E),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, Q),
};

static const pr32::audio::MusicTrack sActionLead = {
    sActionLeadNotes,
    sizeof(sActionLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sActionBass = {
    sActionBassNotes,
    sizeof(sActionBassNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::TRIANGLE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sActionHarmony = {
    sActionHarmonyNotes,
    sizeof(sActionHarmonyNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.125f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sActionDrums = {
    sActionDrumsNotes,
    sizeof(sActionDrumsNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::NOISE,
    0.0f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sActionTrack = {
    sActionLeadNotes,
    sizeof(sActionLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.5f,
    &sActionBass,
    &sActionHarmony,
    &sActionDrums};
    
}
