#pragma once

#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

#include "common_melodies.h"

namespace musicdemo {

// =============================================================================
// MELODY 4 — Em arpeggio + full arrangement (4 flat layers)
// =============================================================================

// 16 × (4 × ARP_STEP) = 32 beats to align with the main theme loop.
#define ARP_DEMO_CELL()                                                                                               \
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_LEAD, pr32::audio::Note::E, 3, ARP_STEP),                       \
        pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_LEAD, pr32::audio::Note::G, 3, ARP_STEP),                   \
        pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_LEAD, pr32::audio::Note::B, 3, ARP_STEP),                   \
        pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_LEAD, pr32::audio::Note::E, 4, ARP_STEP)

static const pr32::audio::MusicNote sArpDemoArpVoiceNotes[] = {
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
    ARP_DEMO_CELL(),
};
#undef ARP_DEMO_CELL

static const pr32::audio::MusicTrack sArpDemoArpVoice = {
    sArpDemoArpVoiceNotes,
    sizeof(sArpDemoArpVoiceNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::SINE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicNote sArpDemoLeadNotes[] = {
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 6, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::Fs, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, H),
    pr32::audio::makeRest(W),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 4, E),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, Q),
    pr32::audio::makeRest(Q),
    pr32::audio::makeRest(W),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, H),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, Q),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, E),
    pr32::audio::makeRest(Q),
};

static const pr32::audio::MusicNote sArpDemoBassNotes[] = {
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::B, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 3, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::C, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::B, 1, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::B, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 3, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::A, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::Fs, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::B, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 3, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, W),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::B, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::D, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::G, 2, Q),
    pr32::audio::makeNote(DEMO_SNES_BASS_STAC, pr32::audio::Note::E, 2, Q),
};

// Same grid as Classic (pattern A/B + fill): 32 beats for a stable loop with lead/bass.
static const pr32::audio::MusicNote sArpDemoDrumsNotes[] = {
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

static const pr32::audio::MusicTrack sArpDemoBass = {
    sArpDemoBassNotes,
    sizeof(sArpDemoBassNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::TRIANGLE,
    0.5f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sArpDemoDrums = {
    sArpDemoDrumsNotes,
    sizeof(sArpDemoDrumsNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::NOISE,
    0.0f,
    nullptr,
    nullptr,
    nullptr};

static const pr32::audio::MusicTrack sArpDemoTrack = {
    sArpDemoLeadNotes,
    sizeof(sArpDemoLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    kDemoArcadeLeadWave,
    0.5f,
    &sArpDemoArpVoice,
    &sArpDemoBass,
    &sArpDemoDrums};

}
