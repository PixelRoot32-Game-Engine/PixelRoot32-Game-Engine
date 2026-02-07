# PixelRoot32 Multi-Core Audio Architecture Design (Revised)

**Document Version:** 1.1  
**Date:** February 2026  
**Author:** PixelRoot32 Architecture Team  
**Reviewed by:** Engine Architecture Review (ChatGPT)

---

## Executive Summary

This document describes the phased plan to fully decouple audio processing from
the main game loop in PixelRoot32, leveraging the ESP32 dual-core architecture
while preserving SDL2 desktop parity.

Compared to version 1.0, this revision incorporates architectural refinements
focused on:

- Stronger ownership rules for audio state.
- Clear separation between game-time and audio-time.
- Safer and simpler synchronization semantics.
- NES-style, sample-accurate audio behavior.
- Reduced long-term maintenance and debugging risk.

The core principle is unchanged:

> Audio state must live in exactly one execution context.

Multi-core execution is treated as an optimization, not a dependency.

---

## Architecture Principles

1. Audio state is owned exclusively by the audio subsystem.
2. Game logic never mutates audio channels directly.
3. Communication between game and audio uses commands, not shared state.
4. Audio timing is sample-based, not frame-based.
5. SDL2 and ESP32 share the same high-level audio architecture.
6. Backward compatibility for existing games is mandatory.
7. Core 0 usage must remain flexible for WiFi/Bluetooth coexistence.

---

## Current State Analysis

```

┌─────────────────────────────────────────────────────────────┐
│                        CORE 1 (Main)                        │
│                                                             │
│  InputManager.update()                                      │
│  SceneManager.update()                                      │
│  MusicPlayer.update()  ← owns timing                        │
│                                                             │
│  AudioEngine.update()  ← mutates channel state              │
│  AudioEngine.playEvent()                                    │
│                                                             │
└───────────────────┬─────────────────────────────────────────┘
│ shared AudioChannel[]
▼
┌─────────────────────────────────────────────────────────────┐
│                    Audio Backend Task / Callback            │
│                                                             │
│  AudioEngine.generateSamples()  ← reads & mutates channels  │
│                                                             │
└─────────────────────────────────────────────────────────────┘

```

Problems:

- AudioChannel state is accessed from multiple execution contexts.
- Audio timing depends on frame deltaTime.
- Music sequencing is frame-driven.
- Race conditions are possible on ESP32 and SDL2.
- Render stalls can affect audio timing.

---

## Target Architecture Overview

```

┌─────────────────────────────────────────────────────────────┐
│                        CORE 1 (Game)                        │
│                                                             │
│  InputManager.update()                                      │
│  SceneManager.update()                                      │
│                                                             │
│  MusicPlayer (thin client)                                  │
│    - Enqueues commands only                                 │
│                                                             │
│  AudioEngine                                                │
│    - Command submission only                                │
│                                                             │
└───────────────────┬─────────────────────────────────────────┘
│ AudioCommandQueue (SPSC, lock-free)
▼
┌─────────────────────────────────────────────────────────────┐
│                        CORE 0 (Audio)                       │
│                                                             │
│  AudioScheduler                                             │
│    - Owns all AudioChannel state                            │
│    - Owns music sequencing                                  │
│    - Uses sample-based timing                               │
│                                                             │
│  AudioBackend                                               │
│    - I2S / DAC / SDL2 output                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘

````

SDL2 mirrors this architecture using either:
- A dedicated audio thread, or
- The existing callback-based model (fallback).

---

## Phase 1: Command Queue Foundation [x]

### Goals

- Eliminate direct mutation of audio state from the game thread.
- Introduce a platform-agnostic command queue.
- Preserve the existing AudioEngine API.

### Key Design Decisions

- Single Producer / Single Consumer (SPSC) ring buffer.
- Lock-free using atomic indices.
- Fixed-size buffer (no allocation).
- Overflow policy: drop newest command.

### AudioCommandQueue

- Producer: Core 1 (game loop)
- Consumer: Audio thread (Core 0 or SDL2 audio context)

```cpp
enum class AudioCommandType : uint8_t {
    PLAY_EVENT,
    STOP_CHANNEL,
    SET_MASTER_VOLUME,
    MUSIC_PLAY,
    MUSIC_STOP,
    MUSIC_PAUSE,
    MUSIC_RESUME,
    MUSIC_SET_TEMPO
};

struct AudioCommand {
    AudioCommandType type;
    union {
        AudioEvent event;
        uint8_t channelIndex;
        float volume;
        const MusicTrack* track;
        float tempoFactor;
    };
};
````

### Notes

* AudioEngine::playEvent() becomes a thin wrapper around enqueue().
* No audio behavior changes in this phase.
* Existing games remain unchanged.

---

## Phase 2: AudioScheduler Abstraction [x]

### Goals

* Centralize all audio state and timing.
* Make audio execution independent of frame rate.
* Prepare for multi-core execution without enforcing it.

### AudioScheduler Responsibilities

* Own AudioChannel array.
* Process AudioCommandQueue.
* Generate PCM samples.
* Maintain audio-time in samples.
* Output to AudioBackend.

### Audio Time Model

* Internal time unit: samples.
* No milliseconds inside the audio thread.
* Conversion from seconds/ms happens only at command ingestion.

```cpp
uint64_t audioTimeSamples;
```

### Scheduler Interface

```cpp
class AudioScheduler {
public:
    virtual void init(AudioBackend* backend, int sampleRate) = 0;
    virtual void submitCommand(const AudioCommand& cmd) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isIndependent() const = 0;
};
```

---

## Phase 3: Music System Decoupling [x]

### Goals

* Remove frame-driven music sequencing.
* Make music sample-accurate and audio-owned.
* Preserve MusicPlayer API.

### Revised Responsibilities

#### MusicPlayer (Game Thread)

* Thin client.
* Enqueues:

  * play / stop / pause / resume
  * tempo changes
* Does NOT advance notes.

#### MusicSequencer (Audio Thread)

* Owns:

  * current track
  * note index
  * remainingSamples
* Triggers AudioEvents internally.
* Uses only sample-based timing.

### Important Change

* durationMs and remainingMs are removed from runtime audio state.
* All durations are converted to sample counts at scheduling time.

---

## Phase 4: Timing Isolation [x]

### Goals

* Completely separate game-time and audio-time.
* Ensure render stalls never affect audio pitch or tempo.

### Rules

* Game loop uses deltaTime (ms).
* Audio uses sample counts only.
* No cross-domain timing math outside well-defined boundaries.

---

## Phase 5: ESP32 Multi-Core Execution [x]

### Goals

* Run audio on Core 0 when available.
* Keep Core 1 dedicated to game logic and rendering.
* Allow coexistence with WiFi/Bluetooth.

### FreeRTOS Strategy

* AudioScheduler runs as a pinned task.
* Priority configurable at runtime.
* Optional yielding between buffers.

```cpp
audioTaskCore = 0;
audioTaskPriority = configMAX_PRIORITIES - 1;
```

Fallback:

* If Core 0 unavailable, audio runs on Core 1 at lower priority.

---

## Phase 6: SDL2 Parity and Fallback Modes [x]

### SDL2 Execution Modes [x]

1. Threaded AudioScheduler (recommended for parity testing)
2. Callback-integrated scheduler (minimal change fallback)

Both modes share:

* Command queue
* Scheduler logic
* Sample-based timing

---

## Phase 7: Backward Compatibility Guarantees

* AudioEngine::playEvent() remains valid.
* MusicPlayer public API unchanged.
* Existing examples compile and behave identically.
* No required changes in game code.

---

## What Remains in Engine::run()

After full migration:

* Input
* Scene update
* Rendering
* MusicPlayer command submission

Removed:

* AudioEngine.update()
* Music sequencing logic
* Audio timing logic

---

## Key Improvements Over Version 1.0

* Stronger ownership guarantees for audio state.
* Sample-based timing enforced earlier.
* Clearer separation of MusicPlayer vs MusicSequencer roles.
* Reduced ambiguity around timing responsibilities.
* Safer overflow policy for command queue.
* Lower long-term debugging and maintenance cost.

---

## Final Assessment

This architecture:

* Matches real console audio pipelines.
* Preserves PixelRoot32’s simplicity.
* Scales cleanly to WiFi/Bluetooth usage.
* Maintains SDL2 development ergonomics.
* Eliminates race conditions by design.

Multi-core support becomes an implementation detail, not a structural dependency.

This design is suitable for production-quality retro hardware engines.