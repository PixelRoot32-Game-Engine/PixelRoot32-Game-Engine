# Metroidvania-Style Example

A compact **platformer** sample with **4bpp tilemap layers**, **`StaticTilemapLayerCache`** for the ESP32 fast path when available, and a **`KinematicActor`**-based player with gravity, climbing, and jump rules tailored to the sample map.

**Requires `PIXELROOT32_ENABLE_4BPP_SPRITES`** — the scene is guarded in [`src/MetroidvaniaScene.h`](src/MetroidvaniaScene.h).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_4BPP_SPRITES`**
- **`PIXELROOT32_ENABLE_2BPP_SPRITES`** (enabled alongside 4bpp in this example's `platformio.ini`)
- **`PIXELROOT32_ENABLE_SCENE_ARENA`**
- **`PIXELROOT32_ENABLE_DIRTY_REGIONS`**

See **`platformio.ini`** for **`native`** and **`esp32dev`** presets (no `esp32cyd` environment in this project).

**`extern pixelroot32::core::Engine engine`** is wired in the platform headers under `src/platforms/`.

## Platforms

| Environment | Display |
|-------------|---------|
| **`native`** | SDL2, 240x240 |
| **`esp32dev`** | **ST7789** 240x240 |

## Controls

Uses **`GameConstants.h`** button IDs: **Up / Down / Left / Right** and **Jump** (`BTN_UP` ... `BTN_JUMP`). Map these to your `InputManager` / GPIO / keyboard mapping for the platform file you use.

## Player Actor

The **`PlayerActor`** extends **`KinematicActor`** and implements:

- **Gravity + horizontal movement** with configurable `PLAYER_GRAVITY`, `PLAYER_MOVE_SPEED`, `PLAYER_JUMP_VELOCITY`
- **Ladder climbing** via `setStairs()` / `buildStairsCache()` — a bitmask RAM cache of climbable tiles
- **State machine**: `IDLE`, `RUN`, `JUMP`, `CLIMBING` with sprite animation per state
- **Collision layers**: `PLAYER`, `PLATFORM`, `GROUND`, `ENEMY` (for future extension)

## How this scene uses the tilemap cache

Drawing goes through **`StaticTilemapLayerCache`**: allocate for the renderer when layers are ready, draw static groups with camera offsets, and **`invalidate()`** when static tile data or relevant animators change. See [Animated Tilemap README](../animated_tilemap/README.md) for the detailed invalidation table and [Architecture — static tilemap cache](../../docs/architecture/architecture-index.md#static-tilemap-layer-cache-engine--scenes).

## Dirty Regions

This example enables **`PIXELROOT32_ENABLE_DIRTY_REGIONS`** for targeted clearing on ESP32. The rendering pipeline uses a dirty-grid approach to selectively clear only the regions that changed between frames, reducing SPI transfer overhead on the display.

## Features

- **4bpp tilemaps** and layered level data (background, platforms, stairs)
- **`StaticTilemapLayerCache`** snapshot path when the driver exposes a logical framebuffer
- **Dirty Regions** — targeted clearing via dirty-grid pipeline on ESP32
- **Player actor** with gravity, stairs/climb behavior, and manual tilemap collision
- **Stairs mask cache** — bitmask RAM cache built once from tile indices for fast overlap checks
- **State-driven sprite animation** — `IDLE`, `RUN`, `JUMP`, `CLIMBING` states
- **Scene arena** + owned layer entities

## Documentation links

- [Graphics — tilemaps & `StaticTilemapLayerCache`](../../docs/api/graphics.md#multi-layer-4bpp-tilemap-framebuffer-snapshot-statictilemaplayercache)
- [Architecture — static tilemap layer cache](../../docs/architecture/architecture-index.md#static-tilemap-layer-cache-engine--scenes)
- [Architecture — ESP32 rendering / tilemap caching](../../docs/architecture/architecture-index.md#esp32-rendering-pipeline-and-tilemap-caching)
- [Physics API](../../docs/api/physics.md)
- [Core API](../../docs/api/core.md)

## Build

From **`examples/metroidvania`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
