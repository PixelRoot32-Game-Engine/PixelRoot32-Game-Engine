# Snake Game Example

Classic **Snake** on a grid: discrete movement (no physics engine), **pre-allocated segment pool** to avoid runtime allocations, food spawning, wall/self collision, score, and **procedural audio** through the engine **`AudioEngine`**.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_AUDIO=1`** — set in [`lib/platformio.ini`](lib/platformio.ini) `base` template so all environments inherit it.

Display size is **240×240** in the project `platformio.ini` (see **`PHYSICAL_DISPLAY_*`**).

## Platforms

| Environment | Display | Audio backend |
|-------------|---------|----------------|
| **`native`** | SDL2, 240×240 | **`SDL2_AudioBackend`** in [`src/platforms/native.h`](src/platforms/native.h) |
| **`esp32dev`** | **ST7789** 240×240 | Default: **`ESP32_I2S_AudioBackend`** (comment in `esp32_dev.h` documents optional internal **DAC** backend instead) |

Pin choices for I2S / DAC are in **`src/platforms/esp32_dev.h`** (edit there if your wiring differs).

## Controls

- **Arrow keys** (or GPIO D-pad mapped in your platform input config) to steer.
- **180° reverse** on the same frame is blocked via `nextDir` (see [`SnakeScene.h`](src/SnakeScene.h)).
- Eat food to grow and add score; hitting walls or yourself ends the run.

## How audio is triggered

[`SnakeScene.cpp`](src/SnakeScene.cpp) builds **`pixelroot32::audio::AudioEvent`** values (move, eat, crash) and calls **`engine.getAudioEngine().playEvent(...)`**. Wave types (triangle, pulse, noise) are lightweight beeps suited for embedded output.

## Features

- **Scene** + **Entity** background + pooled **`SnakeSegmentActor`**
- **Grid logic** and timers (`moveInterval`, `lastMoveTime`)
- **Audio** subsystem integration (`AudioEngine`, platform backends)

## Documentation links

- [Audio API](../../docs/api/audio.md)
- [Core API](../../docs/api/core.md)
- [Input API](../../docs/api/input.md)

## Build

From **`examples/snake`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
