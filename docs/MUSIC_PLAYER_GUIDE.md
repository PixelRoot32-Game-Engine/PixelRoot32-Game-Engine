# MusicPlayer Integration Guide - PixelRoot32

## Overview

The `MusicPlayer` class provides a simple yet powerful way to add background music and melodies to your PixelRoot32 games. It integrates seamlessly with the NES-style audio system and supports tempo control, looping, dynamic music switching, and **multi-track layering** (`secondVoice`, `thirdVoice`, `percussion`) so you can spell out arpeggiated figures with ordinary **`MusicNote`** data.

**Modular Compilation:** The MusicPlayer is only compiled when `PIXELROOT32_ENABLE_AUDIO=1`. When disabled, all music-related functionality is excluded from the build, saving both firmware size and RAM usage. `MusicTrack::channelType` supports `PULSE`, `TRIANGLE`, `NOISE`, `SINE`, and `SAW`.

This guide covers everything from basic music playback to advanced patterns like adaptive soundtracks and smooth transitions.

---

## Quick Start

### Basic Music Playback

```cpp
#include "audio/MusicPlayer.h"
#include "audio/AudioMusicTypes.h"
#include "audio/AudioTypes.h"

using namespace pixelroot32::audio;

// Define your melody
static const MusicNote SIMPLE_MELODY[] = {
    makeNote(INSTR_PULSE_LEAD, Note::C, 4, 0.25f),  // C4 quarter note
    makeNote(INSTR_PULSE_LEAD, Note::E, 4, 0.25f),  // E4 quarter note  
    makeNote(INSTR_PULSE_LEAD, Note::G, 4, 0.25f),  // G4 quarter note
    makeNote(INSTR_PULSE_LEAD, Note::C, 5, 0.5f),  // C5 half note
};

static const MusicTrack SIMPLE_TRACK = {
    SIMPLE_MELODY,
    sizeof(SIMPLE_MELODY) / sizeof(MusicNote),
    true,                    // Loop enabled
    WaveType::PULSE,         // Use pulse wave
    0.5f                     // 50% duty cycle
};

// In your scene's init() method
void MyScene::init() {
#if PIXELROOT32_ENABLE_AUDIO
    // Get music player from engine
    auto& musicPlayer = engine.getMusicPlayer();
    
    // Start playing track
    musicPlayer.play(SIMPLE_TRACK);
#endif
}
```

---

## Music Track Structure

### MusicTrack Definition

```cpp
struct MusicTrack {
    const MusicNote* notes;      // Array of notes
    size_t count;                // Number of notes
    bool loop;                   // Whether to loop the track
    WaveType channelType;        // PULSE, TRIANGLE, NOISE, SINE, SAW
    float duty;                  // Duty cycle for pulse waves (0.0-1.0)
    
    // Multi-track support (optional)
    const MusicTrack* secondVoice = nullptr;  // Second melody voice
    const MusicTrack* thirdVoice = nullptr;   // Third melody voice
    const MusicTrack* percussion = nullptr;  // Drum/percussion track
};
```

### Multi-Track Music Playback

The MusicPlayer supports up to 4 simultaneous tracks playing in parallel:

```cpp
// Define three separate tracks
static const MusicNote MELODY_NOTES[] = {
    makeNote(INSTR_PULSE_LEAD, Note::C, 4, 0.25f),
    makeNote(INSTR_PULSE_LEAD, Note::E, 4, 0.25f),
    makeNote(INSTR_PULSE_LEAD, Note::G, 4, 0.25f),
};

static const MusicNote BASS_NOTES[] = {
    makeNote(INSTR_TRIANGLE_BASS, Note::C, 2, 0.5f),
    makeNote(INSTR_TRIANGLE_BASS, Note::G, 2, 0.5f),
};

static const MusicNote DRUM_NOTES[] = {
    makeNote(INSTR_KICK, Note::Rest, 0.25f),
    makeRest(0.25f),
    makeNote(INSTR_SNARE, Note::Rest, 0.25f),
    makeRest(0.25f),
};

// Create individual tracks
static const MusicTrack MELODY_TRACK = {
    MELODY_NOTES, 3, true, WaveType::PULSE, 0.5f
};

static const MusicTrack BASS_TRACK = {
    BASS_NOTES, 2, true, WaveType::TRIANGLE, 0.5f
};

static const MusicTrack DRUM_TRACK = {
    DRUM_NOTES, 4, true, WaveType::NOISE, 0.0f
};

// Combine into main track with sub-tracks
static const MusicTrack FULL_MUSIC = {
    MELODY_NOTES, 3, true, WaveType::PULSE, 0.5f,
    &BASS_TRACK,      // secondVoice - bass line
    nullptr,          // thirdVoice - not used
    &DRUM_TRACK       // percussion - drums
};

// Play all 3 tracks simultaneously
musicPlayer.play(FULL_MUSIC);

// Query active track count
size_t count = musicPlayer.getActiveTrackCount(); // Returns 3
```

> **Note:** All sub-track pointers default to `nullptr` for backward compatibility with existing single-track code.

---

## Manual arpeggios (extra voice)

There is **no separate arpeggiator API**. To get a rapid broken-chord line under a lead, add a **`MusicTrack`** hooked via **`secondVoice`** or **`thirdVoice`** whose **`MusicNote`** entries use **short `duration` values** (in **beats**, same grid as the rest of the sequencer). You can use **`WaveType::SINE`** / **`SAW`** directly on that layer. See **Melody 4** in `examples/music_demo/src/assets/melodies.h`.

### MusicNote Definition

```cpp
struct MusicNote {
    Note note;                        // Musical note (C, D, E, etc.)
    uint8_t octave;                   // Octave number (0-8). For percussion: 1=Kick, 2=Snare, 3+=Hi-HAT
    float duration;                   // Duration in seconds
    float volume;                     // Volume (0.0-1.0)
    const InstrumentPreset* preset;   // Optional: pointer to instrument preset for percussion
};
```

---

## Creating Music

### Note Helper Functions

The engine provides convenient helper functions for creating notes:

```cpp
// Basic note with default octave (4)
makeNote(INSTR_PULSE_LEAD, Note::C, 0.25f);

// Note with specific octave
makeNote(INSTR_PULSE_LEAD, Note::C, 5, 0.25f);

// Rest (silence)
makeRest(0.5f);

// Using predefined instruments
static const InstrumentPreset INSTR_PULSE_LEAD = {
    0.8f,    // Base volume
    0.5f     // Duty cycle
};
```

### Musical Notes and Octaves

```cpp
enum class Note : uint8_t {
    C = 0, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B, Rest, COUNT
};

// Octave 4 is middle C (C4)
// Standard range: Octave 0-8
// A4 = 440 Hz (concert pitch)
```

### Frequency Reference (Octave 4)

| Note | Frequency (Hz) |
|------|-----------------|
| C4   | 261.63          |
| D4   | 293.66          |
| E4   | 329.63          |
| F4   | 349.23          |
| G4   | 392.00          |
| A4   | 440.00          |
| B4   | 493.88          |

---

## Advanced Music Patterns

### 1. Tempo Control

```cpp
#if PIXELROOT32_ENABLE_AUDIO
class AdaptiveMusicScene : public Scene {
private:
    float currentTempo = 1.0f;
    
public:
    void update(unsigned long deltaTime) override {
        auto& musicPlayer = engine.getMusicPlayer();
        
        // Speed up music as score increases
        if (score > 1000 && currentTempo < 1.5f) {
            currentTempo += 0.01f;
            musicPlayer.setTempoFactor(currentTempo);
        }
        
        // Slow down when player is low on health
        if (playerHealth < 25 && currentTempo > 0.7f) {
            currentTempo -= 0.02f;
            musicPlayer.setTempoFactor(currentTempo);
        }
    }
};
#endif
```

### 2. Dynamic Music Switching

```cpp
void GameScene::switchToBattleMusic() {
    auto& musicPlayer = engine.getMusicPlayer();
    
    // Fade out current music (gradually reduce tempo)
    for (int i = 0; i < 10; i++) {
        float factor = musicPlayer.getTempoFactor() * 0.9f;
        musicPlayer.setTempoFactor(factor);
        delay(50); // 50ms delay for smooth transition
    }
    
    // Switch to battle music
    musicPlayer.stop();
    musicPlayer.setTempoFactor(1.2f); // Slightly faster
    musicPlayer.play(BATTLE_MUSIC);
}
```

### 3. Layered Music System

```cpp
// Background layer (bass line)
static const MusicNote BASS_LINE[] = {
    makeNote(INSTR_TRIANGLE_BASS, Note::C, 2, 0.5f),
    makeNote(INSTR_TRIANGLE_BASS, Note::G, 2, 0.5f),
    makeNote(INSTR_TRIANGLE_BASS, Note::A, 2, 0.5f),
    makeNote(INSTR_TRIANGLE_BASS, Note::F, 2, 0.5f),
};

// Melody layer (lead)
static const MusicNote MELODY[] = {
    makeNote(INSTR_PULSE_LEAD, Note::C, 4, 0.25f),
    makeNote(INSTR_PULSE_LEAD, Note::E, 4, 0.25f),
    makeNote(INSTR_PULSE_LEAD, Note::G, 4, 0.25f),
    makeNote(INSTR_PULSE_LEAD, Note::C, 5, 0.25f),
};

// Use different wave types for variety
static const MusicTrack BASS_TRACK = {
    BASS_LINE, sizeof(BASS_LINE)/sizeof(MusicNote), true, WaveType::TRIANGLE, 0.5f
};

static const MusicTrack MELODY_TRACK = {
    MELODY, sizeof(MELODY)/sizeof(MusicNote), true, WaveType::PULSE, 0.75f
};
```

### 4. Adaptive Soundtrack

```cpp
class AdaptiveMusic {
private:
    enum class MusicState {
        EXPLORATION,
        COMBAT,
        VICTORY,
        DEFEAT
    };
    
    MusicState currentState = MusicState::EXPLORATION;
    MusicState targetState = MusicState::EXPLORATION;
    
public:
    void updateMusic(float threatLevel, bool inCombat, bool victory) {
        auto& musicPlayer = engine.getMusicPlayer();
        
        // Determine target state
        if (victory) {
            targetState = MusicState::VICTORY;
        } else if (threatLevel > 0.7f) {
            targetState = MusicState::COMBAT;
        } else if (inCombat) {
            targetState = MusicState::COMBAT;
        } else {
            targetState = MusicState::EXPLORATION;
        }
        
        // Handle state transitions
        if (currentState != targetState) {
            switch (targetState) {
                case MusicState::COMBAT:
                    musicPlayer.setTempoFactor(1.3f);
                    if (currentState != MusicState::COMBAT) {
                        musicPlayer.play(COMBAT_MUSIC);
                    }
                    break;
                    
                case MusicState::VICTORY:
                    musicPlayer.stop();
                    musicPlayer.setTempoFactor(1.0f);
                    musicPlayer.play(VICTORY_MUSIC);
                    break;
                    
                case MusicState::EXPLORATION:
                    musicPlayer.setTempoFactor(0.9f);
                    if (currentState == MusicState::COMBAT) {
                        musicPlayer.play(EXPLORATION_MUSIC);
                    }
                    break;
            }
            currentState = targetState;
        }
    }
};
```

---

## Integration Patterns

### 1. Scene-Based Music Management

```cpp
class GameScene : public Scene {
private:
    MusicState currentMusic = MusicState::MENU;
    
public:
    void init() override {
        // Start with menu music
        engine.getMusicPlayer().play(MENU_MUSIC);
        engine.getMusicPlayer().setTempoFactor(1.0f);
    }
    
    void onSceneEnter() override {
        // Resume music when returning to this scene
        if (!engine.getMusicPlayer().isPlaying()) {
            engine.getMusicPlayer().resume();
        }
    }
    
    void onSceneExit() override {
        // Pause music when leaving scene
        engine.getMusicPlayer().pause();
    }
    
    void reset() override {
        // Stop and restart music
        engine.getMusicPlayer().stop();
        engine.getMusicPlayer().play(MENU_MUSIC);
    }
};
```

### 2. Music with Sound Effects Mixing

```cpp
void GameScene::playAttackSound() {
    // Lower music volume slightly during SFX
    auto& musicPlayer = engine.getMusicPlayer();
    float originalTempo = musicPlayer.getTempoFactor();
    
    // Slight tempo reduction for dramatic effect
    musicPlayer.setTempoFactor(originalTempo * 0.95f);
    
    // Play attack sound effect (AudioEvent: type, frequency, duration, volume, duty, ...)
    AudioEvent hit{};
    hit.type = WaveType::NOISE;
    hit.frequency = 200.0f;   // noise clock / density
    hit.duration = 0.1f;
    hit.volume = 0.8f;
    hit.duty = 0.5f;
    engine.getAudioEngine().playEvent(hit);
    
    // Restore tempo after delay
    delay(100);
    musicPlayer.setTempoFactor(originalTempo);
}
```

### 3. Music Synchronization with Gameplay

```cpp
void RhythmGameScene::update(unsigned long deltaTime) {
    auto& musicPlayer = engine.getMusicPlayer();
    
    // Check if we're on a beat (every 0.5 seconds)
    static float beatTimer = 0.0f;
    beatTimer += deltaTime * 0.001f;
    
    if (beatTimer >= 0.5f) {
        beatTimer = 0.0f;
        
        // Spawn enemy on beat
        spawnEnemy();
        
        // Flash screen on beat
        screenFlash = 255;
    }
    
    // Update screen flash
    if (screenFlash > 0) {
        screenFlash -= 5;
    }
}
```

---

## Common Music Patterns

### 8-Bar Blues Progression

```cpp
static const MusicNote BLUES_PROGRESSION[] = {
    // Bar 1-2: C7
    makeNote(INSTR_PULSE_LEAD, Note::C, 4, 1.0f),
    makeNote(INSTR_PULSE_LEAD, Note::E, 4, 1.0f),
    makeNote(INSTR_PULSE_LEAD, Note::G, 4, 1.0f),
    makeNote(INSTR_PULSE_LEAD, Note::Bb, 4, 1.0f),
    
    // Bar 3-4: C7
    makeNote(INSTR_PULSE_LEAD, Note::C, 4, 1.0f),
    makeNote(INSTR_PULSE_LEAD, Note::E, 4, 1.0f),
    makeNote(INSTR_PULSE_LEAD, Note::G, 4, 1.0f),
    makeNote(INSTR_PULSE_LEAD, Note::Bb, 4, 1.0f),
    
    // Continue with F7, G7, etc.
};
```

### Broken chord / arpeggiated bass (manual)

```cpp
static const MusicNote ARPEGGIATED_BASS[] = {
    makeNote(INSTR_TRIANGLE_BASS, Note::C, 4, 0.25f),
    makeNote(INSTR_TRIANGLE_BASS, Note::E, 4, 0.25f),
    makeNote(INSTR_TRIANGLE_BASS, Note::G, 4, 0.25f),
    makeNote(INSTR_TRIANGLE_BASS, Note::C, 5, 0.25f),
    makeNote(INSTR_TRIANGLE_BASS, Note::G, 4, 0.25f),
    makeNote(INSTR_TRIANGLE_BASS, Note::E, 4, 0.25f),
    makeNote(INSTR_TRIANGLE_BASS, Note::C, 4, 0.25f),
    makeRest(0.25f),
};
```

### Percussive Rhythm

```cpp
// Use INSTR_KICK, INSTR_SNARE, INSTR_HIHAT presets with WaveType::NOISE
static const MusicNote DRUM_PATTERN[] = {
    makeNote(INSTR_KICK, Note::Rest, 0.25f),    // Kick on beat 1
    makeRest(0.25f),
    makeNote(INSTR_SNARE, Note::Rest, 0.25f),   // Snare on beat 2
    makeRest(0.25f),
    makeNote(INSTR_KICK, Note::Rest, 0.125f),   // Kick (eighth note)
    makeNote(INSTR_HIHAT, Note::Rest, 0.125f),  // Hi-HAT (eighth note)
    makeRest(0.25f),
    makeNote(INSTR_SNARE, Note::Rest, 0.25f),   // Snare on beat 4
    makeRest(0.125f),
    makeNote(INSTR_HIHAT, Note::Rest, 0.125f),  // Hi-HAT
};
```

---

## Best Practices

### 1. Memory Efficiency

- Define music tracks as `static const` to store in flash memory
- Use fixed-size arrays instead of dynamic allocation
- Pre-calculate note frequencies at compile time when possible

### 2. Performance Optimization

- Keep music tracks reasonably short (under 100 notes)
- Use looping instead of very long sequences
- Consider using different wave types for variety vs. complexity

### 3. User Experience

- Provide volume controls in your game's settings
- Allow players to disable music separately from sound effects
- Use smooth transitions between music states
- Consider platform limitations (ESP32 vs PC)

### 4. Platform Considerations

**ESP32:**

- Music timing is **sample-accurate** inside `ApuCore` (shared by all schedulers). The backend’s audio task or I2S callback calls `AudioEngine::generateSamples`, which advances the sequencer and mixes PCM.
- Limited memory: keep tracks short and efficient; prefer `static const` data in flash.
- Subsystem is compiled only when `PIXELROOT32_ENABLE_AUDIO=1`.

**Native (PC/Mac/Linux):**

- `NativeAudioScheduler` runs `ApuCore` in a dedicated `std::thread` and double-buffers PCM for the SDL2 callback—same synthesis and music logic as ESP32.
- More headroom for longer tracks; mixing path uses the same non-linear curve as ESP32 (FPU).

### `isPlaying()` and transport state

`MusicPlayer::isPlaying()` reflects **whether music is actively being sequenced**, not only a client-side flag:

- The authoritative signal is `AudioEngine::isMusicPlaying()` / `isMusicPaused()`, which read atomics updated by **`ApuCore`** when `MUSIC_PLAY` / `MUSIC_STOP` / end-of-non-looping-track / pause / resume are processed.
- **While paused**, `isPlaying()` returns **false** (playback is suspended).
- **Right after `play()`**, before the audio thread has dequeued `MUSIC_PLAY`, `isPlaying()` may still return **true** briefly so game code does not see a spurious “stopped” window (command in flight).
- **Non-looping** tracks: when the last note finishes, `ApuCore` clears the music-playing flag; `isPlaying()` becomes **false** without an explicit `stop()`.

For raw transport without the MusicPlayer wrapper, call `engine.getAudioEngine().isMusicPlaying()` / `isMusicPaused()`.

### BPM API

Besides `setTempoFactor` / `getTempoFactor`, you can drive absolute tempo with **`setBPM`** / **`getBPM`** (default 150 BPM, 4 ticks per beat in the sequencer).

### Master bitcrush and post-mix hook

For global lo-fi degradation or analysis, use **`AudioEngine::setMasterBitcrush`** / **`getMasterBitcrush`** (0–15; 0 = off). For custom processing on the final mono buffer (after bitcrush), configure **`AudioConfig::postMixMono`** / **`postMixUser`** when constructing the engine—see [API_AUDIO.md](api/API_AUDIO.md).

### Related API (sweeps and extra waves)

One-shot **frequency sweeps** on `AudioEvent` (`sweepEndHz`, `sweepDurationSec`) apply to **`PULSE`** and **`TRIANGLE`** (and to **`SINE`** / **`SAW`** when extra waves are enabled). **`NOISE`** ignores sweep fields. Full detail: [API_AUDIO.md](api/API_AUDIO.md).

---

## Troubleshooting

### Music Not Playing

```cpp
// Check if music player is working
auto& musicPlayer = engine.getMusicPlayer();
if (!musicPlayer.isPlaying()) {
    Serial.println("Music player not playing");
    
    // Check track definition
    if (track.count == 0) {
        Serial.println("Track has no notes");
    }
    
    // Check audio engine
    if (!engine.getAudioEngine().isInitialized()) {
        Serial.println("Audio engine not initialized");
    }
}
```

### Tempo Issues

```cpp
// Verify tempo factor
float tempo = musicPlayer.getTempoFactor();
Serial.print("Current tempo factor: ");
Serial.println(tempo);

// Reset to normal if needed
if (tempo < 0.1f || tempo > 3.0f) {
    musicPlayer.setTempoFactor(1.0f);
}
```

### Memory Issues on ESP32

```cpp
// Monitor free memory
Serial.print("Free heap: ");
Serial.println(ESP.getFreeHeap());

// Check track size
Serial.print("Track size: ");
Serial.println(sizeof(MY_TRACK) / sizeof(MusicNote));
```

---

## Complete Example: Game with Multiple Music Tracks

```cpp
#include "audio/MusicPlayer.h"
#include "audio/AudioMusicTypes.h"
#include "audio/AudioTypes.h"

using namespace pixelroot32::audio;

class GameScene : public Scene {
private:
    // Menu music - calm and inviting
    static const MusicNote MENU_NOTES[] = {
        makeNote(INSTR_PULSE_LEAD, Note::C, 4, 0.5f),
        makeNote(INSTR_PULSE_LEAD, Note::E, 4, 0.5f),
        makeNote(INSTR_PULSE_LEAD, Note::G, 4, 0.5f),
        makeNote(INSTR_PULSE_LEAD, Note::C, 5, 1.0f),
    };
    
    static const MusicTrack MENU_TRACK = {
        MENU_NOTES, sizeof(MENU_NOTES)/sizeof(MusicNote), 
        true, WaveType::PULSE, 0.5f
    };
    
    // Game music - upbeat and energetic
    static const MusicNote GAME_NOTES[] = {
        makeNote(INSTR_PULSE_LEAD, Note::C, 4, 0.25f),
        makeNote(INSTR_PULSE_LEAD, Note::D, 4, 0.25f),
        makeNote(INSTR_PULSE_LEAD, Note::E, 4, 0.25f),
        makeNote(INSTR_PULSE_LEAD, Note::F, 4, 0.25f),
        makeNote(INSTR_PULSE_LEAD, Note::G, 4, 0.25f),
        makeNote(INSTR_PULSE_LEAD, Note::A, 4, 0.25f),
        makeNote(INSTR_PULSE_LEAD, Note::B, 4, 0.25f),
        makeNote(INSTR_PULSE_LEAD, Note::C, 5, 0.5f),
    };
    
    static const MusicTrack GAME_TRACK = {
        GAME_NOTES, sizeof(GAME_NOTES)/sizeof(MusicNote),
        true, WaveType::PULSE, 0.5f
    };
    
    // Victory music - triumphant
    static const MusicNote VICTORY_NOTES[] = {
        makeNote(INSTR_PULSE_LEAD, Note::C, 5, 0.125f),
        makeNote(INSTR_PULSE_LEAD, Note::E, 5, 0.125f),
        makeNote(INSTR_PULSE_LEAD, Note::G, 5, 0.125f),
        makeNote(INSTR_PULSE_LEAD, Note::C, 6, 0.5f),
    };
    
    static const MusicTrack VICTORY_TRACK = {
        VICTORY_NOTES, sizeof(VICTORY_NOTES)/sizeof(MusicNote),
        false, WaveType::PULSE, 0.75f
    };
    
    enum class GameState {
        MENU,
        PLAYING,
        VICTORY
    };
    
    GameState currentState = GameState::MENU;
    
public:
    void init() override {
        // Start with menu music
        engine.getMusicPlayer().play(MENU_TRACK);
        engine.getMusicPlayer().setTempoFactor(1.0f);
    }
    
    void update(unsigned long deltaTime) override {
        // Handle state transitions
        if (playerWon && currentState != GameState::VICTORY) {
            changeState(GameState::VICTORY);
        } else if (gameStarted && currentState == GameState::MENU) {
            changeState(GameState::PLAYING);
        }
        
        // Update game logic...
    }
    
private:
    void changeState(GameState newState) {
        auto& musicPlayer = engine.getMusicPlayer();
        
        switch (newState) {
            case GameState::MENU:
                musicPlayer.stop();
                musicPlayer.setTempoFactor(1.0f);
                musicPlayer.play(MENU_TRACK);
                break;
                
            case GameState::PLAYING:
                musicPlayer.stop();
                musicPlayer.setTempoFactor(1.1f); // Slightly faster
                musicPlayer.play(GAME_TRACK);
                break;
                
            case GameState::VICTORY:
                musicPlayer.stop();
                musicPlayer.setTempoFactor(1.0f);
                musicPlayer.play(VICTORY_TRACK);
                break;
        }
        
        currentState = newState;
    }
};
```

---

## References

- **Audio types & presets:** `include/audio/AudioMusicTypes.h`
- **Wave types, `AudioEvent`, `AudioCommand`:** `include/audio/AudioTypes.h`
- **MusicPlayer API:** `include/audio/MusicPlayer.h` (`play`, `stop`, tempo/BPM, …)
- **Audio facade & music transport:** `include/audio/AudioEngine.h` (`isMusicPlaying`, `isMusicPaused`, `setMasterBitcrush`, …)
- **Engine config & post-mix:** `include/audio/AudioConfig.h`
- **Shared synthesis & sequencer:** `include/audio/ApuCore.h`
- **API reference (sweep, bitcrush, SINE/SAW, hooks):** [API_AUDIO.md](api/API_AUDIO.md)
- **Examples:** See game samples under `examples/` for real-world usage.

---

**Note:** Music timing is sample-accurate inside `ApuCore` and is independent of render frame rate, so melody tempo does not slow down when the main loop stalls (within the limits of the audio backend’s buffer).
