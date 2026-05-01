# API Reference: Audio Module

This document covers the audio system, sound effects, and music playback in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

> **Note**: The audio system is only available if `PIXELROOT32_ENABLE_AUDIO=1`

---

## Audio Module Overview

The Audio module provides a **NES-inspired** synthesis stack (Pulse, Triangle, Noise, **SINE**, **SAW**) backed by a **dynamic voice pool** inside **`ApuCore`** (default **`MAX_VOICES = 8`**), a lightweight melody subsystem for background music, an optional **master bitcrush**, **linear frequency sweep** on eligible one-shots, a **tick-aligned arpeggiator**, and an optional **post-mix mono hook** on the final buffer. Games still author sounds with **`WaveType`** on **`AudioEvent`** / **`MusicTrack`**; allocation and optional **voice stealing** happen inside `ApuCore`. Synthesis, mixing, and sequencing live in **`ApuCore`**; `AudioEngine` and the platform schedulers are thin facades.

---

## AudioEngine

**Inherits:** None

The core class managing audio generation and playback.

### Public Methods

- **`AudioEngine(const AudioConfig& config, const PlatformCapabilities& caps)`**
    Constructs the AudioEngine with specific configuration and platform capabilities.

- **`AudioEngine(const AudioConfig& config)`**
    Constructs the AudioEngine with a specific configuration.

- **`void init()`**
    Initializes the audio backend.

- **`void generateSamples(int16_t* stream, int length)`**
    Fills the buffer with audio samples.

- **`void playEvent(const AudioEvent& event)`**
    Enqueues a one-shot `PLAY_EVENT`. The audio consumer (`ApuCore`) assigns a **voice** from the pool for the requested **`WaveType`**, preferring an inactive slot that already matches the type; if the pool is full, it may **steal** the active voice with the **shortest remaining note** (`remainingSamples`).

- **`void setMasterVolume(float volume)`**
    Sets the master volume level (0.0 to 1.0).

- **`void setScheduler(std::unique_ptr<AudioScheduler> scheduler)`**
    Replaces the active scheduler (advanced use; ownership transfers to the engine).

- **`void submitCommand(const AudioCommand& cmd)`**
    Submits a low-level command to the audio consumer (`ApuCore` via the active scheduler). Same SPSC contract as `playEvent`.

- **`bool isMusicPlaying() const`**
    True when the shared `ApuCore` sequencer reports active music transport (after `MUSIC_PLAY`, cleared on `MUSIC_STOP` or natural end of a non-looping track).

- **`bool isMusicPaused() const`**
    True after `MUSIC_PAUSE` until `MUSIC_RESUME`.

- **`float getMasterVolume() const`**
    Gets the current master volume level.

- **`void setMasterBitcrush(uint8_t bits)`**
    Sets global bit depth reduction on the final mono bus after the non-linear mixer (and HPF where applicable). **`0`** = off (default). **`1`–`15`** = reduce effective bit depth; values are clamped internally.

- **`uint8_t getMasterBitcrush() const`**
    Returns the current master bitcrush setting (0–15).

### Typical Usage

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

---

## Data Structures

### WaveType (Enum)

- `PULSE`: Square wave with variable duty cycle.
- `TRIANGLE`: Triangle wave (fixed volume/duty).
- `NOISE`: LFSR-based noise (deterministic, NES-style 15-bit polynomial).
- `SINE`: Band-limited sine via LUT.
- `SAW`: Sawtooth from a linear phase ramp.

All `WaveType` values (including **`SINE`** / **`SAW`**) share the same **`MAX_VOICES`** pool. Under contention the implementation may **steal** a voice (shortest `remainingSamples`) to make room for a new `PLAY_EVENT`.

### VoiceType (Enum, internal)

Defined in [`AudioTypes.h`](include/audio/AudioTypes.h) for maintainers: `VoiceType` mirrors the synthesis kinds (`PULSE`, `TRIANGLE`, `NOISE`, `SINE`, `SAW`) with **`constexpr`** mappers **`toVoiceType(WaveType)`** / **`toWaveType(VoiceType)`**. Game code does **not** need to use `VoiceType`; it exists to keep `ApuCore` allocation logic explicit while preserving the public **`WaveType`** surface.

### AudioEvent (Struct)

Structure defining a sound effect to be played.

- **`WaveType type`**: Type of waveform to use.
- **`float frequency`**: Frequency in Hz.
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).
- **`float duty`**: Duty cycle for Pulse waves (0.0 to 1.0, typically 0.125, 0.25, 0.5, 0.75).
- **`uint8_t noisePeriod`**: For `NOISE`, `0` = derive LFSR step period from `frequency`; `> 0` = fixed period in samples (percussion presets).
- **`const struct InstrumentPreset* preset`**: Optional pointer to instrument preset for ADSR/LFO/waveform parameters. When nullptr, falls back to legacy behavior (2ms attack, no decay, full sustain, 5ms release). Must point to static/constexpr/global instance.
- **`float sweepEndHz`**, **`float sweepDurationSec`**: Optional linear frequency sweep for **`PULSE`**, **`TRIANGLE`**, **`SINE`**, and **`SAW`**. Active when `sweepDurationSec > 0` and `sweepEndHz > 0`; starts at `frequency`, moves toward `sweepEndHz`, clamped to the note length. **`NOISE`** ignores these fields.

## Note (Enum)

Defined in `AudioMusicTypes.h`. Represents the 12 semitones plus a rest:

- `C`, `Cs`, `D`, `Ds`, `E`, `F`, `Fs`, `G`, `Gs`, `A`, `As`, `B`, `Rest`

Use `noteToFrequency(Note note, int octave)` to convert a note and octave to Hz.

---

## Music Data Structures

### MusicNote (Struct)

Represents a single musical note in a melody.

- **`Note note`**: Musical note (C, D, E, etc. or Rest).
- **`uint8_t octave`**: Octave index (0–8). For percussion: 1=Kick, 2=Snare, 3+=Hi-HAT.
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).
- **`const InstrumentPreset* preset`** (optional): Pointer to instrument preset. When set, overrides track defaults for percussion (duty==0).

### MusicTrack (Struct)

Represents a sequence of notes to be played as a track.

- **`const MusicNote* notes`**: Pointer to an array of notes.
- **`size_t count`**: Number of notes in the array.
- **`bool loop`**: If true, the track loops when it reaches the end.
- **`WaveType channelType`**: Which waveform the sequencer should emit for this layer (typically `PULSE`, or `TRIANGLE` / `SINE` / `SAW` when appropriate). This is **not** a fixed hardware channel index; each emitted note still goes through the shared **voice pool** in `ApuCore`.
- **`float duty`**: Duty cycle for Pulse tracks.
- **`const MusicTrack* secondVoice`** (optional): Second melody voice for layered playback.
- **`const MusicTrack* thirdVoice`** (optional): Third melody voice.
- **`const MusicTrack* percussion`** (optional): Drum/percussion track.

Fast repeated-note (“arpeggiated”) figures are authored as **`MusicNote`** sequences—typically on **`secondVoice`** / **`thirdVoice`** with short durations—rather than a separate engine subsystem.

> **Note:** The multi-track pointers default to `nullptr` for backward compatibility. Maximum 4 simultaneous tracks supported.

### MAX_MUSIC_TRACKS

- **`constexpr size_t MAX_MUSIC_TRACKS = 4`**: Maximum simultaneous tracks (main + 3 sub-tracks).

### InstrumentPreset (Struct)

Simple preset describing a reusable "instrument":

- **`float baseVolume`**: Default volume for notes.
- **`float duty`**: Duty cycle suggestion (for Pulse). For percussion (NOISE), use 0.0.
- **`uint8_t defaultOctave`**: Default octave for the instrument. For percussion: 1=Kick, 2=Snare, 3+=Hi-HAT.
- **`float defaultDuration`** (optional): Fixed duration for percussion hits. 0.0 = use note.duration, >0 = fixed (percussion)
- **`uint8_t noisePeriod`** (optional): LFSR period for noise channel. 0 = calc from frequency, >0 = direct period (Kick=25, Snare=50, Hi-HAT=12).

**ADSR Envelope**
- **`float attackTime`**: Attack time in seconds (0.0 = instant)
- **`float decayTime`**: Decay time in seconds (0.0 = skip decay, jump to sustain)
- **`float sustainLevel`**: Sustain level as fraction of peak volume (0.0-1.0)
- **`float releaseTime`**: Release time in seconds (0.0 = instant off, clamped to 100ms max)

**LFO Modulation**
- **`LfoTarget lfoTarget`**: Modulation target (NONE, PITCH, or VOLUME)
- **`float lfoFrequency`**: LFO frequency in Hz (0.0 = disabled)
- **`float lfoDepth`**: Modulation depth. For PITCH: ratio (e.g. 0.05 for ~0.34 semitones). For VOLUME: fraction (0.0-1.0 for attenuation depth)
- **`float lfoDelay`**: Delay before LFO starts in seconds

**Waveform Refinements**
- **`bool noiseShortMode`**: For NOISE channel: true = metallic 93-step LFSR, false = standard 32767-step
- **`float dutySweep`**: For PULSE channel: duty cycle change per second for PWM-like timbral effects

#### Predefined Presets

Melodic instruments:
- `INSTR_PULSE_LEAD` – main lead pulse in octave 4 (duty 0.5).
- `INSTR_PULSE_HARMONY` – harmony pulse in octave 5 (duty 0.125).
- `INSTR_PULSE_PAD` – atmospheric pad pulse in octave 4 (duty 0.25) with slow pitch drift.
- `INSTR_PULSE_BASS` – punchy bass pulse in octave 2 (duty 0.25).
- `INSTR_TRIANGLE_LEAD` – smooth triangle lead in octave 5 with gentle vibrato.
- `INSTR_TRIANGLE_PAD` – soft atmospheric triangle pad in octave 4 with tremolo.
- `INSTR_TRIANGLE_BASS` – triangle bass in octave 3 (duty 0.5).

Percussion instruments (duty=0, use with WaveType::NOISE):
- `INSTR_KICK` – kick drum (defaultOctave=1, duration=0.12s, noisePeriod=25).
- `INSTR_SNARE` – snare drum (defaultOctave=2, duration=0.15s, noisePeriod=50).
- `INSTR_HIHAT` – hi-hat (defaultOctave=3, duration=0.05s, noisePeriod=12).

#### Helper Functions

- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration)`**
- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration)`**
- **`MusicNote makeRest(float duration)`**
- **`float instrumentToFrequency(const InstrumentPreset& preset, Note note, uint8_t octave)`** – returns fixed LFSR clock rate for NOISE channel percussion (Kick=80Hz, Snare=150Hz, Hi-HAT=3000Hz). These control noise density/brightness, not musical pitch.

These helpers reduce boilerplate when defining melodies and keep instruments consistent.

---

## MusicPlayer

**Inherits:** None

High-level API for playing `MusicTrack` instances as background music. It only **enqueues** `AudioCommand`s; note advancement and `AudioEvent` triggering run inside **`ApuCore`** on the audio consumer thread/context.

### Public Methods

- **`MusicPlayer(AudioEngine& engine)`**
    Constructs a MusicPlayer bound to an existing `AudioEngine`.

- **`void play(const MusicTrack& track)`**
    Starts playing the given track from the beginning.

- **`void stop()`**
    Stops playback of the current track.

- **`void pause()`**
    Pauses playback (time does not advance).

- **`void resume()`**
    Resumes playback after pause.

- **`bool isPlaying() const`**
    Returns **true** only when music is **actively sequencing** and **not paused** (combines `AudioEngine::isMusicPlaying()` / `isMusicPaused()` with a short “command in flight” window after `play()` so callers do not flicker to false before the audio thread dequeues `MUSIC_PLAY`). While **paused**, returns **false**.

- **`void setTempoFactor(float factor)`**
    Sets the global tempo scaling factor.
  - `1.0f` is normal speed.
  - `2.0f` is double speed.
  - `0.5f` is half speed.

- **`float getTempoFactor() const`**
    Gets the current tempo scaling factor (default 1.0f).

- **`void setBPM(float bpm)`**
    Sets sequencer BPM (clamped roughly to 30–300); issues `MUSIC_SET_BPM`.

- **`float getBPM() const`**
    Gets the last BPM sent from the game side (default 150).

- **`size_t getActiveTrackCount() const`**
    Returns the number of layered tracks (1–4) based on the last `play()` and local `playing` state; returns **0** if not playing.

- **`void setMasterVolume(float volume)`**
    Sets the master volume level (0.0 to 1.0). Delegates directly to `AudioEngine::setMasterVolume`.

- **`float getMasterVolume() const`**
    Gets the current master volume level. Delegates directly to `AudioEngine::getMasterVolume`.

### Typical Usage

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

---

## Audio Configuration Constants

### Build Flags

| Flag | Default | Description |
|------|---------|-------------|
| `PIXELROOT32_ENABLE_AUDIO` | 1 | Enable/disable entire audio subsystem |
| `PIXELROOT32_NO_DAC_AUDIO` | - | Disable internal DAC backend on classic ESP32 |
| `PIXELROOT32_NO_I2S_AUDIO` | - | Disable I2S audio backend |

### ESP32 buffer notes

I2S backends typically aggregate **1024 samples** per DMA transaction (implementation detail in `ESP32_I2S_AudioBackend`). Internal DAC output uses **I2S in `I2S_MODE_DAC_BUILT_IN`** so samples reach the 8-bit DAC via DMA (not per-sample `dacWrite()`).

### Audio / queue constants

| Symbol / concept | Value | Description |
|------------------|-------|-------------|
| `ApuCore::MAX_VOICES` | `8` | Synthesis voice pool size (`ApuCore.h`) |
| `ApuCore::NUM_CHANNELS` | `MAX_VOICES` | Backward-compatible alias (same header) |
| `AudioCommandQueue::CAPACITY` | 128 | SPSC ring capacity (`AudioCommandQueue.h`) |
| `ApuCore::MIXER_SCALE` | `0.4f` | Per-voice gain before non-linear mix (FPU path) |
| `ApuCore::MIXER_K` | `0.5f` | Soft-knee compressor: `mixed = sum / (1 + \|sum\| * K)` |

On **no-FPU** ESP32 (e.g. ESP32-C3), `ApuCore` uses an **integer oscillator mirror** (`phaseQ32`, `phaseIncQ32`, …) plus the precomputed **`audio_mixer_lut`** (`inline constexpr` in `AudioMixerLUT.h`) so the inner loop avoids soft-float.

### Noise / LFSR state (`AudioChannel` / `Voice`)

| Field | Type | Description |
|-------|------|-------------|
| `lfsrState` | `uint16_t` | NES-style 15-bit LFSR (same polynomial on all platforms) |
| `noisePeriodSamples` | `uint32_t` | Samples between LFSR steps |
| `noiseCountdown` | `uint32_t` | Countdown to next LFSR tick |

Noise is **deterministic** everywhere (including native): no `rand()` in the audio path.

---

## AudioConfig

Configuration struct for the audio system.

- **`AudioBackend* backend`**: Pointer to the platform-specific audio backend (e.g., SDL2, I2S).
- **`int sampleRate`**: Audio sample rate in Hz (default: 22050).
- **`PostMixMonoFn postMixMono`**: Optional **`void (*)(int16_t* mono, int length, void* user)`** invoked on the **final** mono `int16_t` buffer **after** master bitcrush. Must be real-time safe (no allocations, locks, or heavy work). `nullptr` = default path (no callback).
- **`void* postMixUser`**: User pointer passed as the third argument to `postMixMono`.

---

## AudioCommand overview

Low-level control uses **`AudioCommand`** / **`AudioCommandType`** (`include/audio/AudioTypes.h`): `PLAY_EVENT`, `STOP_CHANNEL`, `SET_MASTER_VOLUME`, **`SET_MASTER_BITCRUSH`**, `MUSIC_*`. `AudioEngine::submitCommand` and `playEvent` enqueue into the same SPSC queue consumed by **`ApuCore`**.

---

## Implementation pointer (`ApuCore`)

Synthesis (oscillators, LFSR noise, optional SINE LUT / SAW ramp), **non-linear mixing**, optional **HPF** (FPU / native path), optional **master bitcrush**, optional **post-mix hook**, **fade-in** on new notes, music sequencing, **arpeggiator**, frequency sweep on eligible waves, and the **SPSC command queue** live in **`ApuCore`** (`include/audio/ApuCore.h`, `src/audio/ApuCore.cpp`). `DefaultAudioScheduler`, `ESP32AudioScheduler`, and `NativeAudioScheduler` are thin wrappers that decide **when** `ApuCore::generateSamples` runs.

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Scene
- [Architecture: Audio subsystem](../architecture/ARCH_AUDIO_SUBSYSTEM.md) - Deep dive