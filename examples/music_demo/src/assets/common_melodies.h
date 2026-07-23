#pragma once

#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

namespace pr32 = pixelroot32;

namespace musicdemo {
 /**
 * Durations in beats (quarter note = 1; ApuCore TICKS_PER_BEAT = 4).
 *
 * Voice layout (ApuCore 4+4): main + second + third + percussion map to music
 * slots 0–3; SFX keeps slots 4–7. Demo presets below are retuned for beat-accurate
 * gates (post upstream-v2) — slightly higher level and longer tails vs engine defaults.
 */
static constexpr float S = 0.25f;
static constexpr float E = 0.5f;
static constexpr float Q = 1.0f;
static constexpr float H = 2.0f;
static constexpr float W = 4.0f;

static constexpr pr32::audio::WaveType kDemoArcadeLeadWave = pr32::audio::WaveType::SAW;
static constexpr pr32::audio::WaveType kDemoAdventureLeadWave = pr32::audio::WaveType::SINE;

static constexpr float ARP_STEP = 0.5f;

/** Lead for action arpeggios — SNES-style, retuned for short beat gates. */
static constexpr pr32::audio::InstrumentPreset DEMO_SNES_LEAD_TIGHT{
    0.40f,
    0.5f,
    5,
    0.0f,
    0,
    0.004f,
    0.22f,
    0.88f,
    0.22f,
    pr32::audio::LfoTarget::PITCH,
    5.0f,
    0.022f,
    0.12f,
    false,
    0.0f};

/** Staccato bass — retuned for beat-accurate groove under dense drums. */
static constexpr pr32::audio::InstrumentPreset DEMO_SNES_BASS_STAC{
    0.35f,
    0.5f,
    2,
    0.0f,
    0,
    0.004f,
    0.10f,
    0.35f,
    0.11f,
    pr32::audio::LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    false,
    0.0f};

/** Demo lead (Melodies 1/2/4) — richer sustain/release than INSTR_PULSE_LEAD. */
static constexpr pr32::audio::InstrumentPreset DEMO_MELODY_LEAD{
    0.40f,
    0.5f,
    4,
    0.0f,
    0,
    0.005f,
    0.22f,
    0.88f,
    0.28f,
    pr32::audio::LfoTarget::PITCH,
    5.0f,
    0.025f,
    0.15f,
    false,
    0.0f};

/** Harmony stabs — louder and longer tail for fixed slot 2 under 4+4 mix. */
static constexpr pr32::audio::InstrumentPreset DEMO_HARMONY{
    0.28f,
    0.125f,
    5,
    0.0f,
    0,
    0.005f,
    0.55f,
    0.70f,
    0.38f,
    pr32::audio::LfoTarget::VOLUME,
    6.0f,
    0.30f,
    0.0f,
    false,
    0.15f};

/** Fast arpeggio sub-voice (Melody 4 secondVoice). */
static constexpr pr32::audio::InstrumentPreset DEMO_ARP_VOICE{
    0.38f,
    0.5f,
    5,
    0.0f,
    0,
    0.003f,
    0.18f,
    0.88f,
    0.24f,
    pr32::audio::LfoTarget::PITCH,
    4.0f,
    0.020f,
    0.20f,
    false,
    0.0f};

/** Demo kit — louder/longer one-shots for monophonic drum slot 3. */
static constexpr pr32::audio::InstrumentPreset DEMO_DRUM_KICK{
    0.52f,
    0.0f,
    1,
    0.14f,
    60,
    0.001f,
    0.12f,
    0.00f,
    0.03f,
    pr32::audio::LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    false,
    0.0f};

static constexpr pr32::audio::InstrumentPreset DEMO_DRUM_SNARE{
    0.42f,
    0.0f,
    2,
    0.18f,
    15,
    0.001f,
    0.10f,
    0.00f,
    0.06f,
    pr32::audio::LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    true,
    0.0f};

static constexpr pr32::audio::InstrumentPreset DEMO_DRUM_HIHAT{
    0.32f,
    0.0f,
    3,
    0.07f,
    12,
    0.0005f,
    0.022f,
    0.00f,
    0.008f,
    pr32::audio::LfoTarget::NONE,
    0.0f,
    0.0f,
    0.0f,
    true,
    0.0f};
}
