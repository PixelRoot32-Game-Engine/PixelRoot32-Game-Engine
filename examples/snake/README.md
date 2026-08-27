# Snake Game Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

Classic **Snake** on a grid: discrete movement (no physics engine), **pre-allocated segment pool** to avoid runtime allocations, food spawning, wall/self collision, score, and **procedural audio** through the engine **`AudioEngine`**.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_AUDIO=1`** — set in [`lib/platformio.ini`](lib/platformio.ini) `base` template so all environments inherit it.
- **`PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=1`** — required, not optional. `GameConstants.h` declares `kSnakeGrid` as a `gameplay::GridSpec`, and the whole `GridSpace.h` header lives behind this flag (default `0`), so the example does not compile without it.

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
- Cell/world conversion via **`gameplay::GridSpace`**: segment positions, hit boxes, and food spawn points are all placed through `kSnakeGrid` (`GameConstants.h`) instead of hand-rolled `* CELL_SIZE` / `/ CELL_SIZE` arithmetic. The food-eaten check compares cells (`worldToCellX/Y`), not pixel-aligned equality, so it also works if food is ever spawned off the cell grid.

## Documentation links

- [Audio API](../../docs/api/audio.md)
- [Core API](../../docs/api/core.md)
- [Input API](../../docs/api/input.md)
- [Memory system — gameplay flags and byte budgets](../../docs/architecture/memory-system.md) — RAM cost of `PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE`
- [`gameplay/GridSpace.h`](../../include/gameplay/GridSpace.h) — the full grid conversion API

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
