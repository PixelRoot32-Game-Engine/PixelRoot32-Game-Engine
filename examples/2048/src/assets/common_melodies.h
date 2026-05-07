#pragma once

#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

namespace pr32 = pixelroot32;

namespace game2048 {
 /**
 * Durations in beats (quarter note = 1; ApuCore TICKS_PER_BEAT = 4).
 *
 * Voice budget: up to 4 logical parts (main + second + third + percussion), all sharing
 * ApuCore::MAX_VOICES (8). Density here favors short percussion/harmony notes so SFX can
 * coexist without constant voice stealing.
 */
static constexpr float S = 0.25f;
static constexpr float E = 0.5f;
static constexpr float Q = 1.0f;
static constexpr float H = 2.0f;
static constexpr float W = 4.0f;

static constexpr pr32::audio::WaveType kDemoArcadeLeadWave = pr32::audio::WaveType::SAW;
static constexpr pr32::audio::WaveType kDemoAdventureLeadWave = pr32::audio::WaveType::SINE;

static constexpr float ARP_STEP = 0.5f;   

/** Lead slightly tighter / shorter tail than default INSTR_PULSE_LEAD (SNES-style action role). */
static constexpr pr32::audio::InstrumentPreset DEMO_SNES_LEAD_TIGHT{
    0.34f,
    0.5f,
    5,
    0.0f,
    0,
    0.004f,
    0.18f,
    0.72f,
    0.09f,
    pr32::audio::LfoTarget::PITCH,
    5.0f,
    0.022f,
    0.12f,
    false,
    0.0f};

/** Drier bass so kicks stay clear in dense grooves. */
static constexpr pr32::audio::InstrumentPreset DEMO_SNES_BASS_STAC{
    0.29f,
    0.5f,
    2,
    0.0f,
    0,
    0.002f,
    0.22f,
    0.78f,
    0.0f,
    pr32::audio::LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    false,
    0.0f};

} // namespace game2048