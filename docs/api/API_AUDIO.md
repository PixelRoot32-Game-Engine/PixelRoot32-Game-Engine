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
- **`uint8_t octave`**: Octave index (0–8).
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).

### MusicTrack (Struct)

Represents a sequence of notes to be played as a track.

- **`const MusicNote* notes`**: Pointer to an array of notes.
- **`size_t count`**: Number of notes in the array.
- **`bool loop`**: If true, the track loops when it reaches the end.
- **`WaveType channelType`**: Which channel type to use (typically `PULSE`).
- **`float duty`**: Duty cycle for Pulse tracks.

### InstrumentPreset (Struct)

Simple preset describing a reusable "instrument":

- **`float baseVolume`**: Default volume for notes.
- **`float duty`**: Duty cycle suggestion (for Pulse).
- **`uint8_t defaultOctave`**: Default octave for the instrument.

#### Predefined Presets

- `INSTR_PULSE_LEAD` – main lead pulse in octave 4.
- `INSTR_PULSE_BASS` – bass pulse in octave 3.
- `INSTR_PULSE_CHIP_HIGH` – high-pitched chiptune pulse in octave 5.
- `INSTR_TRIANGLE_PAD` – soft triangle pad in octave 4.

#### Helper Functions

- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration)`**
- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration)`**
- **`MusicNote makeRest(float duration)`**

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

## AudioConfig

Configuration struct for the audio system.

- **`AudioBackend* backend`**: Pointer to the platform-specific audio backend (e.g., SDL2, I2S).
- **`int sampleRate`**: Audio sample rate in Hz (default: 22050).

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Scene