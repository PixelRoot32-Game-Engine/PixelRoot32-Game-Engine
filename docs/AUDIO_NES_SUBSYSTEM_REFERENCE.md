# PixelRoot32 / PR32: NES-like Audio System – Implementation and Usage

This document describes how the NES-style audio subsystem is implemented in the
PixelRoot32 (PR32) engine and how to use it from your games. It covers both the
high-level architecture and the concrete implementation details.

- The system focuses on:
  - Being **deterministic** and low-cost in terms of CPU and RAM.
  - Respecting the existing engine architecture (`core`, `drivers`, `examples`) and the style guide.
  - Providing a **platform-agnostic** audio API:
    - Consistent with `Engine`, `Renderer`, and `InputManager`.
    - With implementations for ESP32 (I2S and DAC) and SDL2 (desktop).
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
  - Backends (SDL2, ESP32 I2S/DAC) only request PCM blocks.

Main files:

- Core: [`audio/AudioEngine.h`](include/audio/AudioEngine.h)
- Audio types: [`audio/AudioTypes.h`](include/audio/AudioTypes.h)
- SDL2 backend: [`SDL2_AudioBackend`](include/drivers/native/SDL2_AudioBackend.h)
- ESP32 I2S backend: [`ESP32_I2S_AudioBackend`](include/drivers/esp32/ESP32_I2S_AudioBackend.h)
- ESP32 DAC backend: [`ESP32_DAC_AudioBackend`](include/drivers/esp32/ESP32_DAC_AudioBackend.h)

---

## 2. Internal data model

### 2.1 Basic types

Defined in [`AudioTypes.h`](include/audio/AudioTypes.h):

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

- **No dynamic allocation**: all 4 channels live in a static array inside the audio subsystem (managed by the `AudioScheduler`).
- `frequency` and `phaseIncrement`:
  - `phaseIncrement = frequency / sampleRate`.
  - Phase runs in `[0.0, 1.0)`.
- **Sample-based timing**:
  - `durationSamples` and `remainingSamples`:
  - Control the lifetime of each sound event in absolute sample counts.
  - Managed by the `AudioScheduler` during sample generation.
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

Defined in [`AudioEngine.h`](include/audio/AudioEngine.h) and
[`AudioEngine.cpp`](src/audio/AudioEngine.cpp).

### 3.1 Channel initialization

In the constructor:

- `channels[0]` and `channels[1]` → `WaveType::PULSE`
- `channels[2]` → `WaveType::TRIANGLE`
- `channels[3]` → `WaveType::NOISE`

Each channel is reset by calling `reset()`.

### 3.2 Lifetime and time model (Sample-Based)

The audio system no longer uses `deltaTime` or frame-based updates. Instead, it uses **sample-accurate timing** managed by an `AudioScheduler`:

- **Audio Time**: Internal unit is samples (e.g., 1 second = 44100 samples at 44.1kHz).
- **Decoupled Logic**: The `AudioScheduler` runs in a separate thread (SDL2) or core (ESP32).
- **Lifetime**: For each active `AudioChannel`, the scheduler subtracts 1 from `remainingSamples` for every sample generated.
- When `remainingSamples` reaches 0, the channel is automatically disabled.

Important:

- **Game logic** runs at its own frame rate (e.g., 60 FPS).
- **Audio generation** runs at the hardware sample rate (e.g., 22050 Hz).
- Render stalls or frame drops **do not affect** audio pitch or tempo.

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

- Volume and the global master volume are applied and the result is scaled to `int16_t`:

```cpp
return (int16_t)(sample * ch.volume * masterVolume * 12000.0f);
```

- `ch.volume` is the per-event/channel volume in the range `[0.0f, 1.0f]`.
- `masterVolume` is a global scalar configured via `setMasterVolume` (also `[0.0f, 1.0f]`).
- The `12000.0f` factor gives a strong output on low-amplitude backends like the ESP32 DAC,
  while the mixer still applies hard clipping after summing all channels.

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

- Now acts as a **Command Producer**.
- It enqueues an `AudioCommand` into a lock-free **Single Producer / Single Consumer (SPSC)** queue.
- The `AudioScheduler` (running on Core 0 or a separate thread) consumes this command and:
  - Looks for a free channel of the requested type (`WaveType`).
  - Applies **voice stealing** if necessary (using the channel with the smallest `remainingSamples`).
  - Converts the event's duration (seconds) into `remainingSamples` based on the current sample rate.
  - Initializes the channel state (`enabled`, `frequency`, `phase`, `volume`, etc.).

---

## 4. Audio Schedulers and Backends

The system uses a decoupled architecture where an `AudioScheduler` owns the audio state and timing, while the `AudioBackend` handles the final hardware output.

### 4.1 AudioScheduler

The `AudioScheduler` is the heart of the decoupled audio system. It:

- Processes the `AudioCommandQueue`.
- Manages the `AudioChannel` states.
- Performs the actual software mixing (`generateSamples`).
- Handles music sequencing (notes, durations, loops).

There are two main implementations:

- **`NativeAudioScheduler`**: Used for SDL2. Runs in a dedicated high-priority thread.
- **`ESP32AudioScheduler`**: Used for ESP32. Runs as a pinned FreeRTOS task.

#### 4.1.1 Platform-Agnostic Core Management

The system no longer uses hardcoded core IDs for ESP32. Instead, it uses a `PlatformCapabilities` structure to detect hardware features at startup:

- **Dual-Core ESP32**: Audio task is pinned to **Core 0** (leaving Core 1 for the game loop).
- **Single-Core ESP32**: Audio task runs on **Core 0** with high priority, allowing the FreeRTOS scheduler to manage time-slicing.
- **Native (SDL2)**: Uses a standard system thread.

### 4.2 Platform Configuration and Build Flags

The audio system behavior can be customized via `platforms/PlatformDefaults.h` or compile-time flags.

#### 4.2.1 Core Affinity

- `PR32_DEFAULT_AUDIO_CORE`: Defines the default core for audio processing (Default: `0` on ESP32).
- `PR32_DEFAULT_MAIN_CORE`: Defines the default core for the main engine loop (Default: `1` on ESP32).

#### 4.2.2 Build Flags

| Flag | Description |
|------|-------------|
| `PIXELROOT32_NO_DAC_AUDIO` | Disables the Internal DAC backend on classic ESP32. |
| `PIXELROOT32_NO_I2S_AUDIO` | Disables the I2S audio backend. |
| `PIXELROOT32_USE_U8G2` | Enables support for the U8G2 display driver (future support). |
| `PIXELROOT32_NO_TFT_ESPI` | Disables the default TFT_eSPI display driver. |

### 4.3 Audio Backends

Backends implement the abstract `AudioBackend` interface:

```cpp
class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    virtual void init(AudioEngine* engine, const PlatformCapabilities& caps) = 0;
    virtual int getSampleRate() const = 0;
};
```

### 4.1 SDL2 backend (Windows / Linux / Mac)

Implemented in:

- Header: [`include/drivers/native/SDL2_AudioBackend.h`](include/drivers/native/SDL2_AudioBackend.h)
- Source: [`src/drivers/native/SDL2_AudioBackend.cpp`](src/drivers/native/SDL2_AudioBackend.cpp)

Key points:

- Uses `SDL_OpenAudioDevice` to open a mono device (`AUDIO_S16SYS`, 1 channel).
- Sets up a C callback (`SDLAudioCallbackWrapper`) that calls the member function
  `SDL2_AudioBackend::audioCallback`.
- In `audioCallback`:
  - Computes how many 16-bit samples are required from `len` (bytes).
  - Calls `engineInstance->generateSamples(...)` to fill the buffer directly.

This completely decouples **audio timing** from the SDL2 game loop.

### 4.2 ESP32 Backends

The engine provides two distinct backends for ESP32, allowing developers to choose between high-quality I2S (external DAC) or retro-style internal DAC.

#### A) ESP32 I2S Backend (External DAC)

- **Class**: `ESP32_I2S_AudioBackend`
- **Header**: [`include/drivers/esp32/ESP32_I2S_AudioBackend.h`](include/drivers/esp32/ESP32_I2S_AudioBackend.h)
- **Use case**: High-quality audio using external DACs like **MAX98357A** or **PCM5102**.
- **Key points**:
  - Uses ESP32 **I2S** peripheral with DMA (`I2S_NUM_0`).
  - Output is digital I2S (BCLK, LRCK, DOUT).
  - Runs in a dedicated FreeRTOS task to ensure smooth playback.
  - Supports standard sample rates (e.g., 22050Hz, 44100Hz).

#### B) ESP32 DAC Backend (Internal DAC)

- **Class**: `ESP32_DAC_AudioBackend`
- **Header**: [`include/drivers/esp32/ESP32_DAC_AudioBackend.h`](include/drivers/esp32/ESP32_DAC_AudioBackend.h)
- **Use case**: Retro audio using the ESP32's **internal 8-bit DAC** (GPIO 25 or 26), either
  driving a small speaker directly or feeding a simple amplifier like **PAM8302A**.
- **Key points**:
- Uses the ESP32 DAC driver (`dac_output_voltage`) for 0–255 output values.
- **No I2S** involved; samples are pushed from a dedicated FreeRTOS task at the configured sample rate.
- Lower resolution (8-bit) but perfect for "chiptune" and Game Boy–style sounds.
- Works well with small on-board speakers and low-cost mono amps.

### 4.3 Backend Configuration (in `main.cpp`)

To select a backend, simply instantiate the desired class and pass it to the `AudioConfig` struct.

**Example for Internal DAC (PAM8302A):**

```cpp
// 1. Instantiate the backend (GPIO 25, 11025Hz for retro feel)
pr32::drivers::esp32::ESP32_DAC_AudioBackend audioBackend(25, 11025);

// 2. Configure the engine
pr32::audio::AudioConfig audioConfig;
audioConfig.backend = &audioBackend;

// 3. Initialize engine
pr32::core::Engine engine(displayConfig, inputConfig, audioConfig);
```

**Example for I2S (MAX98357A):**

```cpp
// 1. Instantiate the backend (BCLK=26, LRCK=25, DOUT=22)
pr32::drivers::esp32::ESP32_I2S_AudioBackend audioBackend(26, 25, 22, 22050);

// 2. Configure the engine
pr32::audio::AudioConfig audioConfig;
audioConfig.backend = &audioBackend;

// 3. Initialize engine
pr32::core::Engine engine(displayConfig, inputConfig, audioConfig);
```

Advantages:

- Audio is generated in a dedicated task, independent from the game frame rate.
- The game loop can run at 60 FPS even while audio is streamed at 22050 Hz.

---

## 5. Integration with Engine and the game loop

The central `Engine` is responsible for:

- Creating the `AudioEngine` instance.
- Providing an `AudioConfig` with the appropriate backend and scheduler.
- Managing the lifecycle of the audio subsystem.

See [`core/Engine.h`](include/core/Engine.h) and
[`core/Engine.cpp`](src/core/Engine.cpp).

**Decoupled flow:**

1. The game creates `DisplayConfig`, `InputConfig`, and `AudioConfig`.
2. It constructs `Engine(displayConfig, inputConfig, audioConfig)`.
3. It calls `engine.init()`:
   - Initializes renderer, input, and audio.
   - The audio scheduler starts its dedicated thread/task.
4. On each frame:
   - `Engine::update`:
     - Computes `deltaTime`.
     - Updates input.
     - Calls `sceneManager.update(deltaTime)`.
     - **Note**: `AudioEngine` and `MusicPlayer` no longer need frame updates.
   - `Engine::draw`:
     - Renders the scene.

Meanwhile, the **Audio Scheduler** (Core 0 / Thread) runs independently at the target sample rate.

---

## 6. Using audio from a game

### 6.1 Accessing the AudioEngine

From any scene or actor that has access to `Engine`:

```cpp
auto& audio = engine.getAudioEngine();
```

### 6.2 Triggering a simple sound

Example of a “coin” sound when the player passes an obstacle in GeometryJump
([`GeometryJumpScene.cpp`](examples/GeometryJump/GeometryJumpScene.cpp)):

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

### 6.2.1 Global master volume

Games can control a global volume multiplier without changing individual events:

```cpp
auto& audio = engine.getAudioEngine();

audio.setMasterVolume(0.5f); // 50% of full volume
// ...
float current = audio.getMasterVolume(); // Query current setting
```

- `setMasterVolume` clamps the value to `[0.0f, 1.0f]`.
- It scales all channels uniformly on top of each event’s own `volume`.

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

Defined in [`AudioMusicTypes.h`](include/audio/AudioMusicTypes.h):

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

Defined in [`MusicPlayer.h`](include/audio/MusicPlayer.h) and
[`MusicPlayer.cpp`](src/audio/MusicPlayer.cpp).

**Responsibilities (Thin Client):**

- Acts as a **Command Producer** for the music system.
- Provides high-level controls: `play`, `stop`, `pause`, `resume`, `setTempoFactor`.
- Enqueues music commands to the `AudioScheduler`.

**Sequencing (Audio Thread):**

- The actual sequencing (advancing notes, timing) is handled by the **`MusicSequencer`** inside the `AudioScheduler`.
- Uses **sample-accurate timing** instead of `deltaTime`.
- Triggers internal audio events directly in the audio thread/core.

### 7.3 Integration with Engine

`MusicPlayer` is owned by `Engine` alongside `AudioEngine`:

- The `Engine` constructor creates `audioEngine` and `musicPlayer`.
- Games use:

```cpp
auto& music = engine.getMusicPlayer();
music.play(myTrack);
```

This keeps music sequencing **sample-accurate** and completely independent of the game frame rate. Render stalls or logic spikes will not cause music to jitter or slow down.

### 7.4 Example: GeometryJump background music

GeometryJump defines a simple looping melody using the helpers in
`AudioMusicTypes.h`
(see [`GeometryJumpScene.cpp`](examples/GeometryJump/GeometryJumpScene.cpp)):

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

With the **Multi-Core Architecture (v0.7.0-dev)**, many previous limitations were addressed, particularly regarding timing and stability.

### 8.1 Resolved / Improved

- **Sample-Accurate Timing**: The system now uses samples instead of `deltaTime` for all internal logic, eliminating jitter and drift.
- **Decoupled Execution**: Audio logic is completely isolated from the game's frame rate, preventing audio stuttering during heavy CPU load.
- **Music Tempo Control**: Added support for real-time tempo changes via `MUSIC_SET_TEMPO`.
- **Simple Volume Envelopes**: Basic volume interpolation (linear fade) is now supported in the scheduler.

### 8.2 Remaining Limitations

- No exact cycle-accurate emulation of the NES APU.
- **Noise Generator**: Still uses a simplified `rand()`-based approach instead of a precise deterministic LFSR.
- **Pitch Sweeps**: Frequency slides (pitch slides) are not yet implemented.
- **Complex Envelopes**: ADSR or complex multi-point envelopes are not supported (only linear interpolation).

### 8.3 Future Extensions

- **Deterministic LFSR**: Replace `rand()` with a proper 15-bit LFSR for authentic NES noise sounds.
- **Frequency Sweeps**: Add `frequencyDelta` to the scheduler for pitch slides.
- **High-Level SFX Helpers**: Add methods like `playJumpSfx()`, `playExplosionSfx()` to `AudioEngine` for easier use.
- **Advanced Music Tooling**: Better support for patterns and multi-track sequencing in the `MusicPlayer`.

---

## 9. Summary

- The NES-like audio system in PixelRoot32:
  - Uses 4 static channels (2 Pulse, 1 Triangle, 1 Noise).
  - Produces mono 16-bit audio via software mixing.
  - Is platform-agnostic thanks to the `AudioBackend` and `AudioScheduler` interfaces.
  - Is **decoupled** from the game loop, running on Core 0 (ESP32) or a separate thread (SDL2).
  - Uses **sample-accurate timing** for both SFX and music.
  - Is controlled from games through `AudioEngine` (SFX) and `MusicPlayer` (Music) via a lock-free command queue.
