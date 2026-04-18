# API Reference: Audio Module

This document covers the audio system, sound effects, and music playback in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

> **Note**: The audio system is only available if `PIXELROOT32_ENABLE_AUDIO=1`

---

## Audio Module Overview

The Audio module provides a NES-like audio system with Pulse, Triangle, and Noise channels, plus a lightweight melody subsystem for background music. Synthesis, mixing, and sequencing are implemented once in **`ApuCore`**; `AudioEngine` and the platform schedulers are thin facades.

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
    Plays a one-shot audio event on the first available channel of the requested type.

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

---

## Data Structures

### WaveType (Enum)

- `PULSE`: Square wave with variable duty cycle.
- `TRIANGLE`: Triangle wave (fixed volume/duty).
- `NOISE`: LFSR-based noise (deterministic, NES-style 15-bit polynomial).

### AudioEvent (Struct)

Structure defining a sound effect to be played.

- **`WaveType type`**: Type of waveform to use.
- **`float frequency`**: Frequency in Hz.
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).
- **`float duty`**: Duty cycle for Pulse waves (0.0 to 1.0, typically 0.125, 0.25, 0.5, 0.75).
- **`uint8_t noisePeriod`**: For `NOISE`, `0` = derive LFSR step period from `frequency`; `> 0` = fixed period in samples (percussion presets).

---

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
- **`WaveType channelType`**: Which channel type to use (typically `PULSE`).
- **`float duty`**: Duty cycle for Pulse tracks.
- **`const MusicTrack* secondVoice`** (optional): Second melody voice for layered playback.
- **`const MusicTrack* thirdVoice`** (optional): Third melody voice.
- **`const MusicTrack* percussion`** (optional): Drum/percussion track.

> **Note:** The multi-track pointers default to `nullptr` for backward compatibility. Maximum 4 simultaneous tracks supported.

### MAX_MUSIC_TRACKS

- **`constexpr size_t MAX_MUSIC_TRACKS = 4`**: Maximum simultaneous tracks (main + 3 sub-tracks).

### InstrumentPreset (Struct)

Simple preset describing a reusable "instrument":

- **`float baseVolume`**: Default volume for notes.
- **`float duty`**: Duty cycle suggestion (for Pulse). For percussion (NOISE), use 0.0.
- **`uint8_t defaultOctave`**: Default octave for the instrument. For percussion: 1=Kick, 2=Snare, 3+=Hi-HAT.
- **`float defaultDuration`** (optional): Fixed duration for percussion hits. 0.0 = use note.duration.
- **`uint8_t noisePeriod`** (optional): LFSR period for noise channel. 0 = calc from frequency, >0 = direct period (Kick=25, Snare=50, Hi-HAT=12).

#### Predefined Presets

Melodic instruments:
- `INSTR_PULSE_LEAD` – main lead pulse in octave 4 (duty 0.5).
- `INSTR_PULSE_HARMONY` – harmony pulse in octave 5 (duty 0.125).
- `INSTR_TRIANGLE_BASS` – triangle bass in octave 3 (duty 0.5).

Percussion instruments (duty=0, use with WaveType::NOISE):
- `INSTR_KICK` – kick drum (defaultOctave=1, duration=0.12s, noisePeriod=25).
- `INSTR_SNARE` – snare drum (defaultOctave=2, duration=0.15s, noisePeriod=50).
- `INSTR_HIHAT` – hi-hat (defaultOctave=3, duration=0.05s, noisePeriod=12).

#### Helper Functions

- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration)`**
- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration)`**
- **`MusicNote makeRest(float duration)`**
- **`float instrumentToFrequency(const InstrumentPreset& preset, Note note, uint8_t octave)`** – converts preset to frequency (for percussion: Kick=80Hz, Snare=150Hz, Hi-HAT=3000Hz)

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
| `AudioCommandQueue::CAPACITY` | 128 | SPSC ring capacity (`AudioCommandQueue.h`) |
| `ApuCore::MIXER_SCALE` | `0.4f` | Per-channel gain before non-linear mix (FPU path) |
| `ApuCore::MIXER_K` | `0.5f` | Soft-knee compressor: `mixed = sum / (1 + \|sum\| * K)` |

On **no-FPU** ESP32 (e.g. ESP32-C3), `ApuCore` uses an **integer oscillator mirror** (`phaseQ32`, `phaseIncQ32`, …) plus the precomputed **`audio_mixer_lut`** (`inline constexpr` in `AudioMixerLUT.h`) so the inner loop avoids soft-float.

### Noise channel (`AudioChannel`)

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

---

## Implementation pointer (`ApuCore`)

Synthesis (oscillators, LFSR noise), **non-linear mixing**, optional **HPF** (FPU / native path), **fade-in** on new notes, music sequencing, and the **SPSC command queue** live in **`ApuCore`** (`include/audio/ApuCore.h`, `src/audio/ApuCore.cpp`). `DefaultAudioScheduler`, `ESP32AudioScheduler`, and `NativeAudioScheduler` are thin wrappers that decide **when** `ApuCore::generateSamples` runs.

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Scene
- [Architecture: Audio subsystem](../architecture/ARCH_AUDIO_SUBSYSTEM.md) - Deep dive