# API Reference: Audio Module

This document covers the audio system, sound effects, and music playback in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

> **Note**: The audio system is only available if `PIXELROOT32_ENABLE_AUDIO=1`

---

## Audio Module Overview

The Audio module provides a NES-like audio system with Pulse, Triangle, and Noise channels, plus a lightweight melody subsystem for background music.

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

- **`void setScheduler(std::shared_ptr<AudioScheduler> scheduler)`**
    Sets a custom audio scheduler. For advanced use.

- **`void submitCommand(const Command& command)`**
    Submits a low-level command to the audio thread. For advanced use.

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
- `NOISE`: Pseudo-random noise.

### AudioEvent (Struct)

Structure defining a sound effect to be played.

- **`WaveType type`**: Type of waveform to use.
- **`float frequency`**: Frequency in Hz.
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).
- **`float duty`**: Duty cycle for Pulse waves (0.0 to 1.0, typically 0.125, 0.25, 0.5, 0.75).
- **`uint8_t noisePeriod`** (optional): LFSR period for NOISE channel. 0 = calc from frequency, >0 = direct period (for percussion).

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

High-level sequencer for playing `MusicTrack` instances as background music.
Music timing is handled internally by the `AudioEngine`.

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
    Returns true if a track is currently playing and not finished.

- **`void setTempoFactor(float factor)`**
    Sets the global tempo scaling factor.
  - `1.0f` is normal speed.
  - `2.0f` is double speed.
  - `0.5f` is half speed.

- **`float getTempoFactor() const`**
    Gets the current tempo scaling factor (default 1.0f).

- **`size_t getActiveTrackCount() const`**
    Returns the number of currently active tracks (1-4). Returns 0 if not playing.

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

### ESP32 Buffer Configuration

| Constant | Default | Description |
|----------|---------|-------------|
| `ESP32_I2S_BUFFER_SIZE` | 1024 | I2S DMA buffer size in samples (must be power of 2) |
| `ESP32_DAC_USE_DAC_WRITE` | true | Use dacWrite() for direct register access (faster) |

### Audio Scheduler Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_NOTES_PER_FRAME` | 8 | Max notes processed per audio quantum to prevent CPU spikes |
| `AUDIO_COMMAND_QUEUE_SIZE` | 128 | SPSC queue capacity for audio commands |

> **Note:** When the audio clock jumps ahead (e.g., after frame drop), the scheduler may skip notes if `MAX_NOTES_PER_FRAME` is reached. This prevents audio thread starvation but may result in dropped notes during catch-up.

### Noise Channel Configuration

| Field | Type | Description |
|-------|------|-------------|
| `lfsrState` | `uint16_t` | NES-style 15-bit LFSR state for deterministic noise |
| `noisePeriodSamples` | `uint32_t` | Sample interval for noise clock (ESP32 only) |
| `noiseCountdown` | `uint16_t` | Countdown counter for noise timing |

> **Note:** The noise channel uses a 15-bit Linear Feedback Shift Register (LFSR) for deterministic, reproducible noise patterns, matching authentic NES behavior. The LFSR advances every output sample in DefaultAudioScheduler, or at the rate specified by `noisePeriodSamples` in ESP32AudioScheduler.

### Volume Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `MASTER_VOLUME_Q16` | Fixed16 | Master volume pre-computed as Q16 fixed-point for faster LUT mixing |
| `CHANNEL_GAIN` | 0.4f | Per-channel gain applied before mixing |

---

## AudioConfig

Configuration struct for the audio system.

- **`AudioBackend* backend`**: Pointer to the platform-specific audio backend (e.g., SDL2, I2S).
- **`int sampleRate`**: Audio sample rate in Hz (default: 22050).

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Scene