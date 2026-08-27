#pragma once

#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

/**
 * Layered stage BGM-style NES-lite: triangle bass motion, pulse arpeggiate, drum grid.
 * Loop = 16 beats (32 eighths). Durations in beats — quarter note = 1.0 per ApuCore.
 */
namespace brickbreaker::audio {

namespace a = pixelroot32::audio;

static constexpr a::InstrumentPreset TRI_BASS = a::INSTR_TRIANGLE_BASS;
static constexpr a::InstrumentPreset LEAD = a::INSTR_PULSE_LEAD;
static constexpr float E = 0.5f;

// Triangle bass: root–fifth walk (Am / G / F / G) two bars each pattern × 16 beats total
static const a::MusicNote TRI_NOTES[] = {
    a::makeNote(TRI_BASS, a::Note::A, E),  a::makeNote(TRI_BASS, a::Note::E, E),
    a::makeNote(TRI_BASS, a::Note::A, E),  a::makeNote(TRI_BASS, a::Note::E, E),
    a::makeNote(TRI_BASS, a::Note::A, E),  a::makeNote(TRI_BASS, a::Note::E, E),
    a::makeNote(TRI_BASS, a::Note::A, E),  a::makeNote(TRI_BASS, a::Note::E, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
    a::makeNote(TRI_BASS, a::Note::F, E),  a::makeNote(TRI_BASS, a::Note::C, E),
    a::makeNote(TRI_BASS, a::Note::F, E),  a::makeNote(TRI_BASS, a::Note::C, E),
    a::makeNote(TRI_BASS, a::Note::F, E),  a::makeNote(TRI_BASS, a::Note::C, E),
    a::makeNote(TRI_BASS, a::Note::F, E),  a::makeNote(TRI_BASS, a::Note::C, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
    a::makeNote(TRI_BASS, a::Note::G, E),  a::makeNote(TRI_BASS, a::Note::D, E),
};

static const a::MusicNote ARP_NOTES[] = {
    a::makeNote(LEAD, a::Note::C, E), a::makeNote(LEAD, a::Note::E, E),
    a::makeNote(LEAD, a::Note::G, E), a::makeNote(LEAD, a::Note::E, E),
    a::makeNote(LEAD, a::Note::C, E), a::makeNote(LEAD, a::Note::E, E),
    a::makeNote(LEAD, a::Note::G, E), a::makeNote(LEAD, a::Note::E, E),
    a::makeNote(LEAD, a::Note::G, E), a::makeNote(LEAD, a::Note::B, E),
    a::makeNote(LEAD, a::Note::D, E), a::makeNote(LEAD, a::Note::B, E),
    a::makeNote(LEAD, a::Note::G, E), a::makeNote(LEAD, a::Note::B, E),
    a::makeNote(LEAD, a::Note::D, E), a::makeNote(LEAD, a::Note::B, E),
    a::makeNote(LEAD, a::Note::F, E), a::makeNote(LEAD, a::Note::A, E),
    a::makeNote(LEAD, a::Note::C, E), a::makeNote(LEAD, a::Note::A, E),
    a::makeNote(LEAD, a::Note::F, E), a::makeNote(LEAD, a::Note::A, E),
    a::makeNote(LEAD, a::Note::C, E), a::makeNote(LEAD, a::Note::A, E),
    a::makeNote(LEAD, a::Note::G, E), a::makeNote(LEAD, a::Note::B, E),
    a::makeNote(LEAD, a::Note::D, E), a::makeNote(LEAD, a::Note::B, E),
    a::makeNote(LEAD, a::Note::G, E), a::makeNote(LEAD, a::Note::B, E),
    a::makeNote(LEAD, a::Note::D, E), a::makeNote(LEAD, a::Note::B, E),
};

#define BK_DK a::makeNote(a::INSTR_KICK, a::Note::Rest, 1, E)
#define BK_DH a::makeNote(a::INSTR_HIHAT, a::Note::Rest, 1, E)
#define BK_DS a::makeNote(a::INSTR_SNARE, a::Note::Rest, 1, E)

static const a::MusicNote DRUM_NOTES[] = {
    BK_DK, BK_DH, BK_DH, BK_DS, BK_DH, BK_DK, BK_DH, BK_DS, BK_DK, BK_DH, BK_DH, BK_DS, BK_DK,
    BK_DK, BK_DH, BK_DS, BK_DH, BK_DH, BK_DK, BK_DS, BK_DK, BK_DK, BK_DH, BK_DS,
    BK_DK, BK_DK, BK_DK, BK_DK, BK_DK, BK_DK, BK_DK, BK_DK,
};

#undef BK_DK
#undef BK_DH
#undef BK_DS

static const a::MusicTrack BGM_STAGE = {
    TRI_NOTES,
    sizeof(TRI_NOTES) / sizeof(a::MusicNote),
    true,
    a::WaveType::TRIANGLE,
    0.5f,
    nullptr,
    nullptr,
    nullptr,
};

/** Tempo multiplier for Level 1 baseline; higher levels ramp up slowly in setupMusic/loadLevel. */
inline float tempoForLevel(int level) {
    if (level < 1) level = 1;
    return 1.18f + static_cast<float>(level - 1) * 0.032f;
}

} // namespace brickbreaker::audio
