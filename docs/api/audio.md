# API Reference: Audio Module

> **Source of truth:** 
> - `include/audio/AudioEngine.h`
> - `include/audio/MusicPlayer.h`
> - `include/audio/AudioTypes.h`
> - `include/audio/AudioMusicTypes.h`
> - `include/audio/AudioConfig.h`
> - `include/audio/ApuCore.h`

## Overview

The Audio module provides a **NES-inspired** synthesis stack (Pulse, Triangle, Noise, **SINE**, **SAW**) backed by a **dynamic voice pool** inside **`ApuCore`** (default **`MAX_VOICES = 8`**), a lightweight melody subsystem for background music, an optional **master bitcrush**, **linear frequency sweep** on eligible one-shots, a **tick-aligned arpeggiator**, and an optional **post-mix mono hook** on the final buffer. Games still author sounds with **`WaveType`** on **`AudioEvent`** / **`MusicTrack`**; allocation and optional **voice stealing** happen inside `ApuCore`. Synthesis, mixing, and sequencing live in **`ApuCore`**; `AudioEngine` and the platform schedulers are thin facades.

> **Note**: The audio system is only available if `PIXELROOT32_ENABLE_AUDIO=1`

## Key Concepts

### WaveType & VoiceType

- `PULSE`: Square wave with variable duty cycle.
- `TRIANGLE`: Triangle wave (fixed volume/duty).
- `NOISE`: LFSR-based noise (deterministic, NES-style 15-bit polynomial).
- `SINE`: Band-limited sine via LUT.
- `SAW`: Sawtooth. `Legacy` and `Nes` render the ideal ramp, aliasing included; `Hybrid` reads a band-limited mipmap.

All `WaveType` values share the same **`MAX_VOICES`** pool. Under contention the implementation may steal a voice (lowest steal score; looping SFX score 0) to make room for a new event. Internally, `VoiceType` mirrors these for allocation logic.

### Sound profiles

`SoundProfile` names a character set applied to every voice. The default is `Legacy`, and it is a **frozen contract**: its rendered PCM is pinned by immutable goldens and does not change between releases.

| Profile | Render core | Character |
|---|---|---|
| `Legacy` (default) | float on FPU targets, integer elsewhere | the historical sound: rational mix curve, bipolar noise, continuous PULSE duty, ideal SAW ramp, single-pole HPF |
| `Nes` | integer everywhere | NES-weighted mix, unipolar noise on the NES period LUT, PULSE duty snapped to the four NES steps, and the NES output chain (HPF 90 Hz → HPF 440 Hz → Butterworth LPF) |
| `Hybrid` | integer everywhere | the `Nes` character plus an exponential DECAY/RELEASE envelope and a band-limited SAW read from a mipmap |

The profile is audio-thread state. From the game thread, switch it with a `SET_SOUND_PROFILE` command — the documented path, which produces exactly the state `ApuCore::setSoundProfile` produces on the audio thread:

```cpp
AudioCommand cmd{};
cmd.type = AudioCommandType::SET_SOUND_PROFILE;
cmd.soundProfile = static_cast<uint8_t>(SoundProfile::Hybrid);
audio.submitCommand(cmd);
```

`init()` deliberately does not reset it, so restarting the audio stream keeps the caller's choice. Switching profiles changes the character, not the loudness: `Nes` and `Hybrid` carry a level trim matched on crest-weighted loudness against `Legacy`. Their peaks are higher, though — on the canonical track about 37 % (`Nes`) and 41 % (`Hybrid`) of full scale against `Legacy`'s 18 %, and about 52 % / 51 % on a dense mix against 36 % — so a game that mixes by peak should still check its master level.

`Legacy` and `Nes` keep the ideal SAW ramp, aliasing included; only `Hybrid` band-limits it. `SINE` is identical on every profile. An explicit `setHpfCutoffHz()` call suppresses the `Nes`/`Hybrid` chain and its trim, which puts that voice back on the historical one-pole. The APU's `docs/ABI.md` §6 holds the full matrix and the per-character defaults.

### Predefined Instrument Presets

Melodic instruments: `INSTR_PULSE_LEAD`, `INSTR_PULSE_HARMONY`, `INSTR_PULSE_PAD`, `INSTR_PULSE_BASS`, `INSTR_TRIANGLE_LEAD`, `INSTR_TRIANGLE_PAD`, `INSTR_TRIANGLE_BASS`.

Percussion instruments (duty=0, use with WaveType::NOISE): `INSTR_KICK`, `INSTR_SNARE`, `INSTR_HIHAT`.

## Usage Examples

### Playing Sound Effects

```cpp
auto& audio = engine.getAudioEngine();

AudioEvent evt{};
evt.type = WaveType::PULSE;
evt.frequency = 1500.0f;
evt.duration = 0.12f;
evt.volume = 0.8f;
evt.duty = 0.5f;

audio.playEvent(evt);
```

Sweep example (melodic or noise period sweep):

```cpp
AudioEvent sweep{};
sweep.type = WaveType::PULSE;
sweep.frequency = 2000.0f;       // start Hz
sweep.sweepEndHz = 400.0f;       // end Hz (melodic) or target noise clock Hz for NOISE
sweep.sweepDurationSec = 0.15f;  // active when both this and sweepEndHz > 0
sweep.duration = 0.25f;
sweep.volume = 0.7f;
sweep.duty = 0.5f;
audio.playEvent(sweep);
```

Looping SFX (motor, ambience) — stop explicitly when done:

```cpp
AudioEvent motor{};
motor.type = WaveType::TRIANGLE;
motor.frequency = 110.0f;
motor.loop = true;
motor.volume = 0.35f;
motor.preset = &INSTR_TRIANGLE_BASS;
audio.playEvent(motor);
// Later: STOP_CHANNEL on the voice slot, or stop all SFX voices when leaving the scene.
```

Under SFX pool saturation, **looping voices are preferred steal targets** (steal score 0) so new one-shots are not starved.

### Playing Music

```cpp
using namespace pixelroot32::audio;

static const MusicNote MELODY[] = {
    makeNote(INSTR_PULSE_LEAD, Note::C, 0.20f),
    makeNote(INSTR_PULSE_LEAD, Note::E, 0.20f),
    makeNote(INSTR_PULSE_LEAD, Note::G, 0.25f),
    makeRest(0.10f),
};

static const MusicTrack GAME_MUSIC = {
    MELODY,
    sizeof(MELODY) / sizeof(MusicNote),
    true,
    WaveType::PULSE,
    0.5f
};

void MyScene::init() {
    engine.getMusicPlayer().play(GAME_MUSIC);
}
```

## Architecture Notes

- **ApuCore**: Synthesis, non-linear mixing, the output stage (profile-owned HPF chain and level trim), master bitcrush, post-mix hooks, sequencing, and the SPSC command queue live here. `DefaultAudioScheduler` and platform variants decide when `generateSamples` runs.
- **ESP32 buffer notes**: the render block is `AudioConfig::blockSize`, a multiple of 128 samples capped at `PR32_AUDIO_MAX_BLOCK_SAMPLES` (1024) and validated by the backends in `init()`. Defaults are **512** samples on FPU platforms and **1024** on no-FPU platforms. The DMA ring is derived from it — `dma_buf_len = 128`, `dma_buf_count = clamp(blockSize / 128, 2, 8)` — which puts the objective latency at **23.2 ms** (dual-core) / **46.4 ms** (single-core) at 22 050 Hz and **16 / 32 ms** at 32 000 Hz. Internal DAC output uses I2S in `I2S_MODE_DAC_BUILT_IN` and follows the same derivation. A backend instance is **2 096 B** on `esp32dev`: give it file scope or heap, never a local in a small-stack task.
  Underruns are counted per iteration: a write that barely blocked (`writeUs < budgetUs / 10`, meaning the DMA ring was already empty) or that failed/came back short, after 10 warm-up iterations. `getUnderrunCount()`, `getWorstBlockUs()`, `getWorstRenderUs()` and `getWorstWriteUs()` expose the counters. Each figure is its own maximum, so the render cost is not `worstBlockUs - worstWriteUs`: the render time is measured directly around `generateSamples()`.
- On **no-FPU** ESP32 (e.g. ESP32-C3), `ApuCore` uses an integer oscillator mirror, fixed-point HPF, and integer LFO to avoid soft-float in the inner loop.
- **Noise / LFSR**: Deterministic everywhere (no `rand()`).

## Configuration

| Flag / Concept | Default | Description |
|----------------|---------|-------------|
| `PIXELROOT32_ENABLE_AUDIO` | `1` | Enable/disable entire audio subsystem |
| `PIXELROOT32_NO_DAC_AUDIO` | - | Disable internal DAC backend on classic ESP32 |
| `PIXELROOT32_NO_I2S_AUDIO` | - | Disable I2S audio backend |
| `ApuCore::MAX_VOICES` | `8` | Synthesis voice pool size |
| `SoundProfile` | `Legacy` | Character applied to every voice; `Nes` and `Hybrid` also select the NES output chain and its level trim. Set with `SET_SOUND_PROFILE` from the game thread |
| `AudioCommandQueue::CAPACITY` | `64` | SPSC ring capacity (range 32–64; 63 usable commands) |
| `ESP32_I2S_AudioBackend::blockSize` | `512` (FPU) / `1024` (no FPU) | Render block in samples, read from `AudioConfig::blockSize`; multiple of 128, at most `PR32_AUDIO_MAX_BLOCK_SAMPLES` (1024) |

## Related Types

- `AudioEngine` → `include/audio/AudioEngine.h`
- `MusicPlayer` → `include/audio/MusicPlayer.h`
- `AudioEvent`, `AudioCommand` → `include/audio/AudioTypes.h`
- `SoundProfile`, `EnvelopeCurve`, `MixerCurve`, `AudioCommandType::SET_SOUND_PROFILE` → `include/pixelroot32/apu/AudioTypes.h` (re-exported by `include/audio/AudioTypes.h`)
- `MusicNote`, `MusicTrack`, `InstrumentPreset` → `include/audio/AudioMusicTypes.h`
- `AudioConfig` → `include/audio/AudioConfig.h`

## Related Documentation

- [API Reference](index.md) - Main index
- [API Core](core.md) - Engine, Scene
- [Architecture Index](../architecture/architecture-index.md)