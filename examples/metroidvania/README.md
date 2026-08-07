# Metroidvania-Style Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

A compact **platformer** sample with **4bpp tilemap layers**, **`StaticTilemapLayerCache`** for the ESP32 fast path when available, and a **`KinematicActor`**-based player with gravity, climbing, and jump rules tailored to the sample map.

**Requires `PIXELROOT32_ENABLE_4BPP_SPRITES`** — the scene is guarded in [`src/MetroidvaniaScene.h`](src/MetroidvaniaScene.h).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_4BPP_SPRITES`**
- **`PIXELROOT32_ENABLE_2BPP_SPRITES`** (enabled alongside 4bpp in this example's `platformio.ini`)
- **`PIXELROOT32_ENABLE_SCENE_ARENA`**
- **`PIXELROOT32_ENABLE_DIRTY_REGIONS`**
- **`PIXELROOT32_ENABLE_CAMERA_EFFECTS`** — camera shake when player falls into void
- **`PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE`** — required, not optional. `PlayerActor` holds a `gameplay::StateMachine` member and the whole class lives behind this flag (default `0`), so the actor does not compile without it.

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
- **State machine**: `IDLE`, `RUN`, `JUMP`, `CLIMBING` with sprite animation per state, dispatched through the engine's **`gameplay::StateMachine`** rather than a hand-written `switch`. The state graph is declared in a `static const` table on `PlayerActor`; `IDLE`/`RUN`/`JUMP` carry `onUpdate` callbacks that request the next transition. `CLIMBING`'s callback is deliberately null — that state is entered and left by ladder presence and vertical input, from outside the machine.

  Animation timing keeps an explicit accumulator reset from `onEnter`, rather than deriving frames from `getTimeInState()`. That is not an oversight: `StateMachine::update()` adds the frame delta *before* dispatching `onUpdate`, and `requestState()` then resets time-in-state to `0`, so a transition made from inside `onUpdate` would lose one frame per transition. See the `@warning` on `update()` in [`gameplay/StateMachine.h`](../../include/gameplay/StateMachine.h).
- **Collision layers**: `PLAYER`, `PLATFORM`, `GROUND`, `ENEMY` (for future extension)

## How this scene uses the tilemap cache

Drawing goes through **`StaticTilemapLayerCache`**: allocate for the renderer when layers are ready, draw static groups with camera offsets, and **`invalidate()`** when static tile data or relevant animators change. See [Animated Tilemap README](../animated_tilemap/README.md) for the detailed invalidation table and [Architecture — static tilemap cache](../../docs/architecture/architecture-index.md#static-tilemap-layer-cache-engine--scenes).

**Note**: During camera shake, the `StaticTilemapLayerCache` auto-invalidates each frame because the renderer offset changes. This ensures tilemaps shake along with entities. After the shake ends, the cache resumes its fast memcpy path.

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
- **Camera shake on void fall** — when the player falls below the world boundary, the entire screen (tilemaps + entities) shakes for 200ms before respawning

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
