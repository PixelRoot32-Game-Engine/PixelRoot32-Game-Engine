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
- `SAW`: Sawtooth from a linear phase ramp.

All `WaveType` values share the same **`MAX_VOICES`** pool. Under contention the implementation may steal a voice (shortest remaining time) to make room for a new event. Internally, `VoiceType` mirrors these for allocation logic.

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

Sweep example (pulse or triangle only; ignored for `NOISE`):

```cpp
AudioEvent sweep{};
sweep.type = WaveType::PULSE;
sweep.frequency = 2000.0f;       // start Hz
sweep.sweepEndHz = 400.0f;       // end Hz
sweep.sweepDurationSec = 0.15f;  // active when both this and sweepEndHz > 0
sweep.duration = 0.25f;
sweep.volume = 0.7f;
sweep.duty = 0.5f;
audio.playEvent(sweep);
```

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

- **ApuCore**: Synthesis, non-linear mixing, HPF, master bitcrush, post-mix hooks, sequencing, and the SPSC command queue live here. `DefaultAudioScheduler` and platform variants decide when `generateSamples` runs.
- **ESP32 buffer notes**: I2S backends typically aggregate 1024 samples per DMA transaction. Internal DAC output uses I2S in `I2S_MODE_DAC_BUILT_IN`.
- On **no-FPU** ESP32, `ApuCore` uses an integer oscillator mirror and a precomputed mixer LUT so the inner loop avoids soft-float.
- **Noise / LFSR**: Deterministic everywhere (no `rand()`).

## Configuration

| Flag / Concept | Default | Description |
|----------------|---------|-------------|
| `PIXELROOT32_ENABLE_AUDIO` | `1` | Enable/disable entire audio subsystem |
| `PIXELROOT32_NO_DAC_AUDIO` | - | Disable internal DAC backend on classic ESP32 |
| `PIXELROOT32_NO_I2S_AUDIO` | - | Disable I2S audio backend |
| `ApuCore::MAX_VOICES` | `8` | Synthesis voice pool size |
| `AudioCommandQueue::CAPACITY` | `128` | SPSC ring capacity |

## Related Types

- `AudioEngine` → `include/audio/AudioEngine.h`
- `MusicPlayer` → `include/audio/MusicPlayer.h`
- `AudioEvent`, `AudioCommand` → `include/audio/AudioTypes.h`
- `MusicNote`, `MusicTrack`, `InstrumentPreset` → `include/audio/AudioMusicTypes.h`
- `AudioConfig` → `include/audio/AudioConfig.h`

## Related Documentation

- [API Reference](index.md) - Main index
- [API Core](core.md) - Engine, Scene
- [Architecture Overview](../architecture/overview.md)