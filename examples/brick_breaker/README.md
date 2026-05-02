# Brick Breaker Example

Classic **Breakout** arcade game: paddle with ball physics, destructible brick grid, particle effects on destruction, multiple lives, progressive levels, and **procedural audio** through the engine **`AudioEngine`** and **`MusicPlayer`**.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_AUDIO=1`** — set in [`lib/platformio.ini`](lib/platformio.ini) `base` template so all environments inherit it.
- **`PIXELROOT32_ENABLE_PHYSICS=1`** — required for ball collision detection.
- **`PIXELROOT32_ENABLE_PARTICLES=1`** — required for brick destruction effects.
- **`PIXELROOT32_ENABLE_UI_SYSTEM=1`** — required for score/lives display.

Display size is **240×240** in the project `platformio.ini` (see **`PHYSICAL_DISPLAY_*`**).

## Platforms

| Environment | Display | Audio backend |
|-------------|---------|---------------|
| **`native`** | SDL2, 240×240 | **`SDL2_AudioBackend`** in [`src/platforms/native.h`](src/platforms/native.h) |
| **`esp32dev`** | **ST7789** 240×240 | Default: **`ESP32_I2S_AudioBackend`** (comment in `esp32_dev.h` documents optional internal **DAC** backend instead) |

Pin choices for I2S / DAC are in **`src/platforms/esp32_dev.h`** (edit there if your wiring differs).

## Controls

- **Arrow keys** (or GPIO D-pad mapped in your platform input config) to move paddle left/right.
- **Start/Enter** to launch ball (from attached position) and start game.
- **Start/Enter** to restart after game over.

## How audio is triggered

- **SFX**: [`GameConstants.h`](src/GameConstants.h) defines `sfx::` namespace with `AudioEvent` values (paddle hit, wall hit, brick hit, life lost, start game) using pulse wave tones.
- **BGM**: [`BrickBreakerScene.cpp`](src/BrickBreakerScene.cpp) sets up an Atari-style 4-note melody loop using **`MusicPlayer`** with triangle wave.

## Features

- **Scene** with pooled **`PaddleActor`**, **`BallActor`**, **`BrickActor`**
- **Ball attaches to paddle** until player launches (Breakout-style)
- **3 lives** system with visual indicators
- **Progressive levels** — more rows and HP per brick as level increases
- **Particle effects** via `ParticleEmitter` (explosion preset)
- **Audio** — SFX synthesis + BGM loop via `MusicPlayer`
- **Score** system displayed on screen

## Documentation links

- [Audio API](../../docs/api/audio.md)
- [Core API](../../docs/api/core.md)
- [Input API](../../docs/api/input.md)
- [Particles API](../../docs/api/particles.md)

## Build

From **`examples/brick_breaker`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
