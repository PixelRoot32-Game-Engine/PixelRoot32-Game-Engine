# PixelRoot32 / EDGE: NES-like Audio System – Implementation and Usage

This document describes how the NES-style audio subsystem is implemented in the
PixelRoot32 (EDGE) engine and how to use it from your games. It covers both the
high-level architecture and the concrete implementation details.

- The system focuses on:
  - Being **deterministic** and low-cost in terms of CPU and RAM.
  - Respecting the existing engine architecture (`core`, `drivers`, `examples`) and the style guide.
  - Providing a **platform-agnostic** audio API:
    - Consistent with `Engine`, `Renderer`, and `InputManager`.
    - With one implementation for ESP32 (I2S) and another for SDL2 on desktop.
  - Avoiding unnecessary complexity:
    - No exact emulation of the NES APU.
    - No heavy external audio libraries.
    - No breaking changes for existing games in `examples`.

---

## 1. Overview

- 4 fixed channels:
  - 2 `PULSE` channels (square wave with configurable duty cycle).
  - 1 `TRIANGLE` channel.
  - 1 `NOISE` channel.
- Software mixing into a **mono** 16-bit (`int16_t`) stream.
- **Event-driven** model: games fire short-lived `AudioEvent` instances (SFX, notes).
- Fully **platform-agnostic** core:
  - Wave, mixing, and timing logic lives in `AudioEngine`.
  - Backends (SDL2, ESP32 I2S) only request PCM blocks.

Main files:

- Core: [`audio/AudioEngine.h`](../lib/PixelRoot32-Game-Engine/include/audio/AudioEngine.h)
- Audio types: [`audio/AudioTypes.h`](../lib/PixelRoot32-Game-Engine/include/audio/AudioTypes.h)
- SDL2 backend: [`SDL2_AudioBackend`](../lib/PixelRoot32-Game-Engine/include/drivers/native/SDL2_AudioBackend.h)
- ESP32 I2S backend: [`ESP32_AudioBackend`](../lib/PixelRoot32-Game-Engine/include/drivers/esp32/ESP32_AudioBackend.h)

---

## 2. Internal data model

### 2.1 Basic types

Defined in [`AudioTypes.h`](../lib/PixelRoot32-Game-Engine/include/audio/AudioTypes.h):

```cpp
enum class WaveType {
    PULSE,
    TRIANGLE,
    NOISE
};
```

`WaveType` defines the waveform type for each channel.

### 2.2 AudioChannel

Each channel in the system is represented by a static `AudioChannel` struct:

```cpp
struct AudioChannel {
    bool enabled = false;
    WaveType type;
    float frequency = 0.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float volume = 0.0f;
    float targetVolume = 0.0f;
    float dutyCycle = 0.5f;
    uint16_t noiseRegister = 1;
    unsigned long durationMs = 0;
    unsigned long remainingMs = 0;

    void reset() {
        enabled = false;
        phase = 0.0f;
        volume = 0.0f;
        remainingMs = 0;
    }
};
```

Key characteristics:

- **No dynamic allocation**: all 4 channels live in a static array inside `AudioEngine`.
- `frequency` and `phaseIncrement`:
  - `phaseIncrement = frequency / sampleRate`.
  - Phase runs in `[0.0, 1.0)`.
- `durationMs` and `remainingMs`:
  - Control the lifetime of each sound event.
  - Updated in `AudioEngine::update`.
- `noiseRegister`:
  - Used as internal state for the noise channel.

### 2.3 AudioEvent

Also defined in `AudioTypes.h`:

```cpp
struct AudioEvent {
    WaveType type;
    float frequency;
    float duration; // seconds
    float volume;   // 0.0 - 1.0
    float duty;     // only for PULSE
};
```

- It is the basic unit used to trigger a sound.
- It is passed as a parameter to `AudioEngine::playEvent`.

---

## 3. AudioEngine: mixing core

Defined in [`AudioEngine.h`](../lib/PixelRoot32-Game-Engine/include/audio/AudioEngine.h) and
[`AudioEngine.cpp`](../lib/PixelRoot32-Game-Engine/src/audio/AudioEngine.cpp).

### 3.1 Channel initialization

In the constructor:

- `channels[0]` and `channels[1]` → `WaveType::PULSE`
- `channels[2]` → `WaveType::TRIANGLE`
- `channels[3]` → `WaveType::NOISE`

Each channel is reset by calling `reset()`.

### 3.2 Lifetime and time model

`AudioEngine::update(unsigned long deltaTime)`:

- Called once per frame from `Engine::update`.
- For each `AudioChannel`:
  - If the channel is active (`enabled`) and has `remainingMs > 0`:
    - Subtracts `deltaTime`.
    - If the duration is exhausted, disables the channel (`enabled = false`).

Important:

- **Game time** is synchronized with the main game loop.
- The **sample rate** is handled by the backend (audio callback / task).

### 3.3 Per-channel sample generation

`int16_t AudioEngine::generateSampleForChannel(AudioChannel& ch)`:

- If the channel is disabled, returns `0`.
- Depending on `ch.type`:

1. `PULSE`:
   - Square wave with duty cycle:
     - `sample = (phase < dutyCycle) ? 1.0f : -1.0f;`
2. `TRIANGLE`:
   - Approximate triangle wave in the range `[-1, 1]`:
     - First half: `0 → 0.5` rises from -1 to +1.
     - Second half: `0.5 → 1.0` falls from +1 to -1.
3. `NOISE`:
   - Simple noise using `noiseRegister`:
   - On each phase wrap (when `phase` resets) it updates:
     - `noiseRegister = rand() & 0xFFFF;`
   - The least significant bit decides the sign of the sample.

After computing the base value:

- Phase is advanced:

```cpp
ch.phase += ch.phaseIncrement;
if (ch.phase >= 1.0f) {
    ch.phase -= 1.0f;
}
```

- Volume is applied and the result is scaled to `int16_t`:

```cpp
return (int16_t)(sample * ch.volume * 4000.0f);
```

The `4000.0f` factor keeps enough headroom so that the sum of several channels does not clip.

### 3.4 Mixing all channels

`void AudioEngine::generateSamples(int16_t* stream, int length)`:

- Clears the buffer to 0.
- For each index from `0` to `length - 1`:
  - Initializes an accumulator `mixedSample = 0`.
  - Adds the result of `generateSampleForChannel` for each of the 4 channels.
  - Applies **hard clipping** to `[-32768, 32767]`.
  - Writes the result into `stream[i]`.

This produces a **mono** 16-bit stream, ready to be sent to SDL2 or I2S.

### 3.5 Event playback: playEvent

`void AudioEngine::playEvent(const AudioEvent& event)`:

- Looks for a free channel of the requested type (`WaveType`) using `findFreeChannel`.
  - If there is no free channel, applies a **voice stealing** policy:
    - Uses the channel with the smallest `remainingMs`.
- Initializes the channel:
  - `enabled = true`.
  - `frequency = event.frequency`.
  - `phase = 0.0f`.
  - `phaseIncrement = frequency / sampleRate`.
  - `volume = event.volume`.
  - `durationMs` and `remainingMs` based on `event.duration`.
  - If it is `PULSE`, sets `dutyCycle = event.duty`.

---

## 4. Audio backends

Backends implement the abstract `AudioBackend` interface:

```cpp
class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    virtual void init(AudioEngine* engine) = 0;
    virtual int getSampleRate() const = 0;
};
```

### 4.1 SDL2 backend (Windows / Linux / Mac)

Implemented in:

- Header: [`include/drivers/native/SDL2_AudioBackend.h`](../lib/PixelRoot32-Game-Engine/include/drivers/native/SDL2_AudioBackend.h)
- Source: [`src/drivers/native/SDL2_AudioBackend.cpp`](../lib/PixelRoot32-Game-Engine/src/drivers/native/SDL2_AudioBackend.cpp)

Key points:

- Uses `SDL_OpenAudioDevice` to open a mono device (`AUDIO_S16SYS`, 1 channel).
- Sets up a C callback (`SDLAudioCallbackWrapper`) that calls the member function
  `SDL2_AudioBackend::audioCallback`.
- In `audioCallback`:
  - Computes how many 16-bit samples are required from `len` (bytes).
  - Calls `engineInstance->generateSamples(...)` to fill the buffer directly.

This completely decouples **audio timing** from the SDL2 game loop.

### 4.2 ESP32 I2S backend

Implemented in:

- Header: [`include/drivers/esp32/ESP32_AudioBackend.h`](../lib/PixelRoot32-Game-Engine/include/drivers/esp32/ESP32_AudioBackend.h)
- Source: [`src/drivers/esp32/ESP32_AudioBackend.cpp`](../lib/PixelRoot32-Game-Engine/src/drivers/esp32/ESP32_AudioBackend.cpp)

Key points:

- Uses the ESP32 **I2S** peripheral with DMA:
  - Configures `I2S_NUM_0` in `MASTER | TX` mode.
  - `sample_rate` is configurable (22050 Hz in the examples).
  - 16 bits per sample (`I2S_BITS_PER_SAMPLE_16BIT`), mono (`I2S_CHANNEL_FMT_ONLY_RIGHT`).
- Configures I2S pins:
  - `bck_io_num` (BCLK).
  - `ws_io_num` (LRCK/WS).
  - `data_out_num` (DOUT).
- Creates a FreeRTOS task (`AudioTask`), usually pinned to **Core 0**.
- In `audioTaskLoop`:
  - Declares a small local sample buffer.
  - Calls `engineInstance->generateSamples(...)`.
  - Sends the block to I2S via `i2s_write` (blocking if DMA is full).

Advantages:

- Audio is generated in a dedicated task, independent from the game frame rate.
- The game loop can run at 60 FPS even while audio is streamed at 22050 Hz.

---

## 5. Integration with Engine and the game loop

The central `Engine` is responsible for:

- Creating the `AudioEngine` instance.
- Providing an `AudioConfig` with the appropriate backend.
- Calling `audioEngine.update(deltaTime)` every frame.

See [`core/Engine.h`](../lib/PixelRoot32-Game-Engine/include/core/Engine.h) and
[`core/Engine.cpp`](../lib/PixelRoot32-Game-Engine/src/core/Engine.cpp).

Simplified flow:

1. The game creates `DisplayConfig`, `InputConfig`, and `AudioConfig`.
2. It constructs `Engine(displayConfig, inputConfig, audioConfig)`.
3. It calls `engine.init()`:
   - Initializes renderer, input, and audio.
   - The audio backend opens the device (SDL2) or configures I2S (ESP32).
4. On each frame:
   - `Engine::update`:
     - Computes `deltaTime`.
     - Updates input.
     - Calls `sceneManager.update(deltaTime)`.
     - Calls `audioEngine.update(deltaTime)`.
   - `Engine::draw`:
     - Renders the scene.

Meanwhile, backends (SDL2/I2S) call `generateSamples` at high frequency.

---

## 6. Using audio from a game

### 6.1 Accessing the AudioEngine

From any scene or actor that has access to `Engine`:

```cpp
auto& audio = engine.getAudioEngine();
```

### 6.2 Triggering a simple sound

Example of a “coin” sound when the player passes an obstacle in GeometryJump
([`GeometryJumpScene.cpp`](../lib/PixelRoot32-Game-Engine/examples/GeometryJump/GeometryJumpScene.cpp)):

```cpp
pr32::audio::AudioEvent coinEvent{};
coinEvent.type = pr32::audio::WaveType::PULSE;
coinEvent.frequency = 1500.0f;
coinEvent.duration = 0.12f;
coinEvent.volume = 0.8f;
coinEvent.duty = 0.5f;
engine.getAudioEngine().playEvent(coinEvent);
```

Recommended patterns:

- Use `PULSE` for “blip”, “coin”, jumps, and UI sounds.
- Use `TRIANGLE` for bass lines or softer tones.
- Use `NOISE` for hits, explosions, and collisions.

### 6.3 Designing NES-like effects

Since there are no complex envelopes yet, effects are built by combining:

- `frequency`: lower or higher pitch.
- `duration`: effect length (seconds).
- `volume`: 0.0–1.0.
- `duty` (pulse only):
  - 0.125: thinner, sharper timbre.
  - 0.25: classic “NES lead”.
  - 0.5: symmetric square, “fatter” sound.

---

## 7. Melody subsystem (tracks and songs)

The audio system also includes a lightweight melody/music layer built on top of
`AudioEngine`. It is designed to stay simple and deterministic, while being easy
to use from games.

### 7.1 Data model (`AudioMusicTypes.h`)

Defined in [`AudioMusicTypes.h`](../lib/PixelRoot32-Game-Engine/include/audio/AudioMusicTypes.h):

```cpp
enum class Note : uint8_t {
    C = 0, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B,
    Rest,
    COUNT
};
```

- `Note::Rest` represents a silence.
- Frequencies are derived from an internal table for octave 4 combined with
  power-of-two shifts:

```cpp
inline float noteToFrequency(Note note, int octave);
```

Melodies are sequences of `MusicNote` elements:

```cpp
struct MusicNote {
    Note note;
    uint8_t octave;
    float duration; // seconds
    float volume;   // 0.0 - 1.0
};
```

A `MusicTrack` groups notes and defines how they are played:

```cpp
struct MusicTrack {
    const MusicNote* notes;
    size_t count;
    bool loop;
    WaveType channelType;
    float duty;
};
```

For convenience there are simple “instrument” presets and helpers:

```cpp
struct InstrumentPreset {
    float baseVolume;
    float duty;
    uint8_t defaultOctave;
};

inline constexpr InstrumentPreset INSTR_PULSE_LEAD{0.35f, 0.5f, 4};
inline constexpr InstrumentPreset INSTR_PULSE_BASS{0.30f, 0.25f, 3};
inline constexpr InstrumentPreset INSTR_PULSE_CHIP_HIGH{0.32f, 0.125f, 5};
inline constexpr InstrumentPreset INSTR_TRIANGLE_PAD{0.28f, 0.5f, 4};

inline MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration);
inline MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration);
inline MusicNote makeRest(float duration);
```

These helpers reduce boilerplate when defining tracks and keep note volumes and
octaves consistent per instrument.

### 7.2 MusicPlayer (`MusicPlayer.h`)

Defined in [`MusicPlayer.h`](../lib/PixelRoot32-Game-Engine/include/audio/MusicPlayer.h) and
[`MusicPlayer.cpp`](../lib/PixelRoot32-Game-Engine/src/audio/MusicPlayer.cpp).

Responsibilities:

- Maintain the current track, note index and timer.
- On `update(deltaTime)` advance through the sequence using game time.
- For each new note, call `AudioEngine::playEvent(...)` with the appropriate
  frequency, duration and volume.
- Support `play`, `stop`, `pause`, `resume` and simple looping
  (`MusicTrack::loop`).
- Treat `Note::Rest` as silence (no event is fired, only time passes).

Internally, `MusicPlayer` converts each `MusicNote` into an `AudioEvent` using
`noteToFrequency` and the instrument config.

### 7.3 Integration with Engine

`MusicPlayer` is owned by `Engine` alongside `AudioEngine`:

- The `Engine` constructor creates `audioEngine` and `musicPlayer` (passing a
  reference to `audioEngine`).
- `Engine::update` calls `musicPlayer.update(deltaTime)` after
  `audioEngine.update(deltaTime)`.
- Games use:

```cpp
auto& music = engine.getMusicPlayer();
music.play(myTrack);
```

This keeps music sequencing deterministic and synchronized with the main game
loop, reusing the same timing model as the rest of the engine.

### 7.4 Example: GeometryJump background music

GeometryJump defines a simple looping melody using the helpers in
`AudioMusicTypes.h`
(see [`GeometryJumpScene.cpp`](../lib/PixelRoot32-Game-Engine/examples/GeometryJump/GeometryJumpScene.cpp)):

```cpp
using namespace pixelroot32::audio;

static const MusicNote MELODY_NOTES[] = {
    makeNote(INSTR_PULSE_LEAD, Note::C, 0.20f),
    makeNote(INSTR_PULSE_LEAD, Note::E, 0.20f),
    makeNote(INSTR_PULSE_LEAD, Note::G, 0.25f),
    makeRest(0.10f),
    // ...
};

static const MusicTrack GAME_MUSIC = {
    MELODY_NOTES,
    sizeof(MELODY_NOTES) / sizeof(MusicNote),
    true,            // loop
    WaveType::PULSE, // use one PULSE channel
    0.5f             // duty cycle
};

void GeometryJumpScene::init() {
    // ...
    engine.getMusicPlayer().play(GAME_MUSIC);
}
```

- Music uses one `PULSE` channel; the remaining channels stay available for SFX.
- Because `MusicPlayer` is frame-driven, the melody timing is stable even if FPS
  varies.

---

## 8. Current limitations and future extensions

Current limitations (intentional to keep things simple):

- No exact emulation of the NES APU.
- Not implemented yet:
  - Complex volume envelopes.
  - Frequency sweeps (pitch slides).
  - Advanced music features (patterns, tempo changes, multi-track songs).
- Noise uses a simplified `rand()`-based approach instead of a precise LFSR.

Possible future improvements:

- Simple envelopes based on `targetVolume`.
- More advanced music tooling on top of `MusicPlayer`.
- Noise using a deterministic LFSR with configurable patterns.
- High-level helpers such as:
  - `playJumpSfx()`, `playHitSfx()`, `playCoinSfx()`.

---

## 9. Summary

- The NES-like audio system in PixelRoot32:
  - Uses 4 static channels (2 Pulse, 1 Triangle, 1 Noise).
  - Produces mono 16-bit audio via software mixing.
  - Is platform-agnostic thanks to the `AudioBackend` interface.
  - Is integrated with the game loop via `Engine::update`.
  - Is controlled from games through `AudioEvent` and `AudioEngine::playEvent`.
  - Provides a lightweight melody layer via `Note`, `MusicTrack` and
    `MusicPlayer` for background music.
