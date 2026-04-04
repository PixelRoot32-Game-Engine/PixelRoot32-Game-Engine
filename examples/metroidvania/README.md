# Metroidvania-Style Example

A compact **platformer** sample with **4bpp tilemap layers** (background, platforms, decorative tiles), **`StaticTilemapLayerCache`** for the ESP32 fast path when available, and a **`KinematicActor`**-based player with climbing and jump rules tailored to the sample map.

**Requires `PIXELROOT32_ENABLE_4BPP_SPRITES`** — the scene is guarded in [`src/MetroidvaniaScene.h`](src/MetroidvaniaScene.h).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_4BPP_SPRITES`**
- **`PIXELROOT32_ENABLE_2BPP_SPRITES`** (enabled alongside 4bpp in this example’s `platformio.ini`)
- **`PIXELROOT32_ENABLE_SCENE_ARENA`**

See **`platformio.ini`** for **`native`** and **`esp32dev`** presets (no `esp32cyd` environment in this project).

**`extern pixelroot32::core::Engine engine`** is wired in the platform headers under `src/platforms/`.

## Platforms

| Environment | Display |
|-------------|---------|
| **`native`** | SDL2, 240×240 |
| **`esp32dev`** | **ST7789** 240×240 |

## Controls

Uses **`GameConstants.h`** button IDs: **Up / Down / Left / Right** and **Jump** (`BTN_UP` … `BTN_JUMP`). Map these to your `InputManager` / GPIO / keyboard mapping for the platform file you use.

## How this scene uses the tilemap cache

Like the animated tilemap sample, drawing goes through **`StaticTilemapLayerCache`**: allocate for the renderer when layers are ready, draw static groups with camera offsets, and **`invalidate()`** when static tile data or relevant animators change. See [Animated Tilemap README](../animated_tilemap/README.md) for the detailed invalidation table and [Architecture — static tilemap cache](../../docs/ARCHITECTURE.md).

## Features

- **4bpp tilemaps** and layered level data
- **`StaticTilemapLayerCache`** snapshot path when the driver exposes a logical framebuffer
- **Player actor** with gravity, stairs/climb behavior, and map collision
- **Scene arena** + owned layer entities

## Documentation links

- [Graphics — tilemaps & `StaticTilemapLayerCache`](../../docs/api/API_GRAPHICS.md#multi-layer-4bpp-tilemap-framebuffer-snapshot-statictilemaplayercache)
- [Architecture — ESP32 rendering / tilemap caching](../../docs/ARCHITECTURE.md#esp32-rendering-pipeline-and-tilemap-caching)
- [Physics API](../../docs/api/API_PHYSICS.md)
- [Core API](../../docs/api/API_CORE.md)

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
