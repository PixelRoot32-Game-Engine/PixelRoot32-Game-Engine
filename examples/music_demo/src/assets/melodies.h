
#pragma once

#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

namespace pr32 = pixelroot32;

namespace musicdemo {

static constexpr float STEP = 0.125f; // base grid (8th note)

// Lead wave for layered tracks: SAW / SINE when extra waves are compiled in (Phase B demo).
#if PIXELROOT32_ENABLE_AUDIO_EXTRA_WAVES
static constexpr pr32::audio::WaveType kDemoArcadeLeadWave = pr32::audio::WaveType::SAW;
static constexpr pr32::audio::WaveType kDemoAdventureLeadWave = pr32::audio::WaveType::SINE;
#else
static constexpr pr32::audio::WaveType kDemoArcadeLeadWave = pr32::audio::WaveType::PULSE;
static constexpr pr32::audio::WaveType kDemoAdventureLeadWave = pr32::audio::WaveType::PULSE;
#endif

// ========== MELODY 1: CLASSIC ARCADE (Tetris-style) ==========
// ===================== LEAD =====================
// Inspired by Tetris Theme (Korobeiniki) - A minor, iconic Russian folk tune
// Structure: Phrase A (mm.1-4) -> Phrase B (mm.5-8) -> bridge -> repeat
static const pr32::audio::MusicNote sClassicArcadeLeadNotes[] = {
    // Phrase A (mm.1-4): E5-B4-C5-D5 | C5-B4-A4-r
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, STEP),

    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 4, STEP * 2),

    // Phrase B (mm.5-8): C-E-G-A | G-E-A-r (descending from A minor up to A major feel)
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 5, STEP),

    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 4, STEP * 2),
};

static const pr32::audio::MusicTrack sClassicArcadeLead = {
    sClassicArcadeLeadNotes,
    sizeof(sClassicArcadeLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.5f,
    nullptr, nullptr, nullptr
};

// ===================== BASS =====================
// A minor progression: Am - C - Em - Am (correct root notes)
static const pr32::audio::MusicNote sClassicArcadeBassNotes[] = {
    // Bar 1: A2 (Am)
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::A, 2, STEP * 2),
    // Bar 2: C3 (C)
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::C, 3, STEP * 2),
    // Bar 3: E3 (Em)
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 3, STEP * 2),
    // Bar 4: A2 (Am)
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::A, 2, STEP * 2),
    // Repeat
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::A, 2, STEP * 2),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::C, 3, STEP * 2),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 3, STEP * 2),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::A, 2, STEP * 2),
};

static const pr32::audio::MusicTrack sClassicArcadeBass = {
    sClassicArcadeBassNotes,
    sizeof(sClassicArcadeBassNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::TRIANGLE,
    0.3f,
    nullptr, nullptr, nullptr
};

// ===================== DRUMS =====================
// Classic arcade drum pattern: Kick on 1, Snare on 3, hi-hats on every beat
static const pr32::audio::MusicNote sClassicArcadeDrumsNotes[] = {
    // Bar 1: Kick - HiHat - HiHat - Snare
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),    // Beat 1
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),   // Beat 2
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),   // Beat 3
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),   // Beat 4

    // Bar 2: HiHat - HiHat - HiHat - HiHat (fill)
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),

    // Bar 3: Kick - HiHat - HiHat - Snare
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),

    // Bar 4: Kick - Snare - Kick - Snare (variation)
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),
};

static const pr32::audio::MusicTrack sClassicArcadeDrums = {
    sClassicArcadeDrumsNotes,
    sizeof(sClassicArcadeDrumsNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::NOISE,
    0.4f,
    nullptr, nullptr, nullptr
};

static const pr32::audio::MusicTrack sClassicArcadeTrack = {
    sClassicArcadeLeadNotes,
    sizeof(sClassicArcadeLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    kDemoArcadeLeadWave,
    0.5f,
    &sClassicArcadeBass,
    nullptr,
    &sClassicArcadeDrums
};

// ========== MELODY 2: ADVENTURE (Zelda-style) ==========
// ===================== LEAD =====================
// Inspired by Zelda Overworld + Metroid - C major, exploration feel
// Structure: Ascending build -> descending release (typical overworld theme)
static const pr32::audio::MusicNote sAdventureLeadNotes[] = {
    // Phrase 1: C5-D5-E5-F5 | G5-A5-B5-C6 (ascending major scale)
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::F, 5, STEP),

    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 6, STEP * 2),

    // Phrase 2: B5-G5-E5-C5 | D5-E5-F5-G5 (descending with resolution)
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, STEP * 2),

    // Bridge: D5-E5-F5-G5 (short build)
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::F, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP * 2),
};

static const pr32::audio::MusicTrack sAdventureLead = {
    sAdventureLeadNotes,
    sizeof(sAdventureLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.5f,
    nullptr, nullptr, nullptr
};

// ===================== BASS =====================
// C major: C2 - G2 - A2 - F2 (I-V-vi-IV progression - classic)
static const pr32::audio::MusicNote sAdventureBassNotes[] = {
    // Bar 1: C2
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::C, 2, STEP * 2),
    // Bar 2: G2
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::G, 2, STEP * 2),
    // Bar 3: A2
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::A, 2, STEP * 2),
    // Bar 4: F2
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::F, 2, STEP * 2),
    // Repeat
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::C, 2, STEP * 2),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::G, 2, STEP * 2),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::A, 2, STEP * 2),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::F, 2, STEP * 2),
};

static const pr32::audio::MusicTrack sAdventureBass = {
    sAdventureBassNotes,
    sizeof(sAdventureBassNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::TRIANGLE,
    0.3f,
    nullptr, nullptr, nullptr
};

// ===================== DRUMS =====================
// Adventure drum pattern: More complex, fills on beat 4
static const pr32::audio::MusicNote sAdventureDrumsNotes[] = {
    // Bar 1: Kick - HiHat - Snare - HiHat
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),

    // Bar 2: Kick - HiHat - Kick - Snare
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),

    // Bar 3: HiHat - Snare - HiHat - Snare (offbeat snare)
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),

    // Bar 4: Kick - HiHat - HiHat - Fill
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),  // Fill
};

static const pr32::audio::MusicTrack sAdventureDrums = {
    sAdventureDrumsNotes,
    sizeof(sAdventureDrumsNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::NOISE,
    0.4f,
    nullptr, nullptr, nullptr
};

// ===================== MAIN TRACK =====================
static const pr32::audio::MusicTrack sAdventureTrack = {
    sAdventureLeadNotes,
    sizeof(sAdventureLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    kDemoAdventureLeadWave,
    0.5f,
    &sAdventureBass,
    nullptr,
    &sAdventureDrums
};

// ========== MELODY 3: ACTION (MegaMan-style) ==========
// ===================== LEAD =====================
// Inspired by Mega Man 2 - Metal Man / Crash Man - E minor, arpeggiated style
// Fast, punchy, 8-bit action game feel
static const pr32::audio::MusicNote sActionLeadNotes[] = {
    // Phrase 1: Arpeggio pattern - E minor (E-G-B)
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 6, STEP * 0.5f),

    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP * 0.5f),

    // Phrase 2: Variation - D major (D-F#-A)
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::F, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 6, STEP * 0.5f),

    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::A, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::F, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP * 0.5f),

    // Phrase 3: Return to E minor with octave jump
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 6, STEP),

    // Phrase 4: Descending finish
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP * 2),
};

static const pr32::audio::MusicTrack sActionLead = {
    sActionLeadNotes,
    sizeof(sActionLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.5f,
    nullptr, nullptr, nullptr
};

// ===================== BASS =====================
// E minor: E2 - D2 - E2 (more active pattern, not just root notes)
static const pr32::audio::MusicNote sActionBassNotes[] = {
    // Bar 1: E2 - E2 - G2
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::G, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP),

    // Bar 2: D2 - D2 - F2
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::D, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::D, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::F, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::D, 2, STEP),

    // Bar 3: E2 - E2 - G2 (return to E minor)
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::G, 2, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP),

    // Bar 4: E2 - rest - E2 (rhythmic variation)
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP),
    pr32::audio::makeRest(STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_TRIANGLE_BASS, pr32::audio::Note::E, 2, STEP * 2),
};

static const pr32::audio::MusicTrack sActionBass = {
    sActionBassNotes,
    sizeof(sActionBassNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::TRIANGLE,
    0.3f,
    nullptr, nullptr, nullptr
};

// ===================== DRUMS =====================
// Action drum pattern: 16th note hi-hats, syncopated snare
static const pr32::audio::MusicNote sActionDrumsNotes[] = {
    // Bar 1: Kick - hhh - hhh - Snare
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP * 0.5f),

    // Bar 2: hhh - Kick - hhh - Snare
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP * 0.5f),

    // Bar 3: Kick - Snare - Kick - Snare
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP * 0.5f),

    // Bar 4: hhh - hhh - Kick - Fill
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_HIHAT, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_KICK, pr32::audio::Note::Rest, 1, STEP * 0.5f),
    pr32::audio::makeNote(pr32::audio::INSTR_SNARE, pr32::audio::Note::Rest, 1, STEP),  // Fill
};

static const pr32::audio::MusicTrack sActionDrums = {
    sActionDrumsNotes,
    sizeof(sActionDrumsNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::NOISE,
    0.4f,
    nullptr, nullptr, nullptr
};

// ===================== MAIN TRACK =====================
static const pr32::audio::MusicTrack sActionTrack = {
    sActionLeadNotes,
    sizeof(sActionLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::PULSE,
    0.5f,
    &sActionBass,
    nullptr,
    &sActionDrums
};

// ========== MELODY 4: E-minor loop + manual arpeggio (second voice, SINE when extra waves) ==========
#if PIXELROOT32_ENABLE_AUDIO_EXTRA_WAVES
// ~2 sequencer ticks per step (duration 0.5 beat @ TICKS_PER_BEAT 4).
static constexpr float ARP_DEMO_STEP_BEATS = 0.5f;
static const pr32::audio::MusicNote sArpDemoArpVoiceNotes[] = {
    {pr32::audio::Note::E, 3, ARP_DEMO_STEP_BEATS, 0.3f, nullptr},
    {pr32::audio::Note::G, 3, ARP_DEMO_STEP_BEATS, 0.3f, nullptr},
    {pr32::audio::Note::B, 3, ARP_DEMO_STEP_BEATS, 0.3f, nullptr},
    {pr32::audio::Note::E, 4, ARP_DEMO_STEP_BEATS, 0.3f, nullptr},
};
static const pr32::audio::MusicTrack sArpDemoArpVoice = {
    sArpDemoArpVoiceNotes,
    sizeof(sArpDemoArpVoiceNotes) / sizeof(pr32::audio::MusicNote),
    true,
    pr32::audio::WaveType::SINE,
    0.5f,
    nullptr,
    nullptr,
    nullptr,
};
#endif

static const pr32::audio::MusicNote sArpDemoLeadNotes[] = {
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::G, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::E, 6, STEP * 2),

    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::D, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::C, 5, STEP),
    pr32::audio::makeNote(pr32::audio::INSTR_PULSE_LEAD, pr32::audio::Note::B, 4, STEP * 2),
};

static const pr32::audio::MusicTrack sArpDemoTrack = {
    sArpDemoLeadNotes,
    sizeof(sArpDemoLeadNotes) / sizeof(pr32::audio::MusicNote),
    true,
    kDemoArcadeLeadWave,
    0.5f,
#if PIXELROOT32_ENABLE_AUDIO_EXTRA_WAVES
    &sArpDemoArpVoice,
#else
    nullptr,
#endif
    nullptr,
    nullptr,
};

} // namespace musicdemo