# API Reference: Configuration

> **Source of truth:**
> - `include/platforms/EngineConfig.h`
> - `include/platforms/PlatformDefaults.h`

## Overview

This document covers global configuration options, build flags, and compile-time constants for the PixelRoot32 Game Engine. Most of these configurations can be overridden in your `platformio.ini` file without modifying the engine source code.

## Platform Macros (Build Flags)

| Macro | Description | Default (ESP32) |
|-------|-------------|-----------------|
| `PR32_DEFAULT_AUDIO_CORE` | CPU core assigned to audio tasks. | `0` |
| `PR32_DEFAULT_MAIN_CORE` | CPU core assigned to the main game loop. | `1` |
| `PIXELROOT32_NO_DAC_AUDIO` | Disable Internal DAC support on classic ESP32. | Not defined (opt-in) |
| `PIXELROOT32_NO_I2S_AUDIO` | Disable I2S audio support. | Not defined (opt-in) |
| `PIXELROOT32_USE_U8G2_DRIVER` | Enable U8G2 display driver support for monochromatic OLEDs. | Disabled |
| `PIXELROOT32_NO_TFT_ESPI` | Disable default TFT_eSPI driver support. | Not defined (opt-in) |

## Modular Compilation Flags

| Macro | Description | Default |
|-------|-------------|---------|
| `PIXELROOT32_ENABLE_AUDIO` | Enable audio subsystem. | `1` |
| `PIXELROOT32_ENABLE_PHYSICS` | Enable physics system. | `1` |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | Enable UI system. | `1` |
| `PIXELROOT32_ENABLE_PARTICLES` | Enable particle system. | `1` |
| `PIXELROOT32_ENABLE_CAMERA_EFFECTS` | Enable camera effects (shake, punch, offset). | `1` |
| `PIXELROOT32_ENABLE_SCENE_TRANSITIONS` | Enable scene transition effects (fade, iris). | `1` |
| `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Enable FPS/RAM/CPU debug overlay. | Disabled |
| `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | Enable tile animation system. | `0` |
| `PIXELROOT32_ENABLE_2BPP_SPRITES` | Enable 2bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_4BPP_SPRITES` | Enable 4bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_SCENE_ARENA` | Enable scene memory arena. | Disabled |
| `PIXELROOT32_ENABLE_PROFILING` | Enable profiling hooks in physics pipeline. | Disabled |
| `PIXELROOT32_ENABLE_TOUCH` | Enable automatic touch processing. | `0` (disabled) |
| `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` | Enable **`StaticTilemapLayerCache`** (4bpp FB snapshot). | `1` |
| `PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT` | Enable **`StaticLayerSnapshot`** (FB snapshot of game-drawn static layers). Costs one logical framebuffer of heap per allocating scene (~57 KB at 240×240). | `0` |
| `PIXELROOT32_ENABLE_DIRTY_REGIONS` | Enable dirty-cell selective framebuffer clear (`DirtyGrid`). Requires 64–226 B RAM. | `0` |
| `PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING` | Enable dirty region profiling metrics. | `0` |
| `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK` | TFT_eSPI DMA line batch size. | `60` |
| `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK` | Fallback DMA batch size if memory fails. | `30` |
| `PIXELROOT32_TFT_12BIT_COLOR` | Send frames as 12-bit RGB444 (2 pixels per 3 bytes) instead of RGB565. Experimental. | `0` |
| `PIXELROOT32_DEBUG_MODE` | Enable unified logging system. | Disabled |
| `PIXELROOT32_VELOCITY_DAMPING` | Per-frame velocity damping factor (0.0-1.0). | `0.999` |
| `PIXELROOT32_MAX_VELOCITY` | Maximum velocity cap in units/s. | `500` |

### TFT_eSPI Display Flags

> **Note:** `PIXELROOT32_TFT_12BIT_COLOR=1` is **experimental and not yet verified on hardware**. It cuts 25% of the SPI bus time per frame and shrinks each DMA line buffer by 25%, and the driver silently keeps RGB565 when `PHYSICAL_DISPLAY_WIDTH` is not a multiple of 4. See [12-bit Color on the Wire (RGB444)](../guide/performance/esp32-performance.md#12-bit-color-on-the-wire-rgb444) for the bandwidth math, the width constraint and the memory trade-off.

> **Note:** The two framebuffer caches solve the same problem from opposite ends, and a given static layer wants one or the other, never both. `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` covers layers the engine can redraw itself — it owns the `TileMap4bpp` and repaints it when the cache goes cold. `PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT` covers layers **game code** draws, caching the resulting framebuffer instead of any source; that is what an isometric or oblique floor needs in the default build, since `drawTileMap` assumes axis-aligned cells and has no diamond to redraw unless given a projection (`PIXELROOT32_ENABLE_TILEMAP_PROJECTION`, default `0`). Pair the snapshot with `PIXELROOT32_ENABLE_DIRTY_REGIONS` to restore only the cells last frame's movers touched rather than the whole buffer.

> **Note:** `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK=60` only became reachable in `d6dc9ae`. Earlier builds hit a buffer-selection bug in `buildScaleLUTs()` that always downgraded to `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK`, so the documented `60` default never applied. The fallback still applies when DMA-capable internal RAM is tight.

## Memory Savings by Subsystem

| Subsystem Disabled | RAM Savings | Flash Savings |
|-------------------|-------------|--------------|
| `PIXELROOT32_ENABLE_AUDIO=0` | ~8 KB | ~15 KB |
| `PIXELROOT32_ENABLE_PHYSICS=0` | ~12 KB | ~25 KB |
| `PIXELROOT32_ENABLE_UI_SYSTEM=0` | ~4 KB | ~20 KB |
| `PIXELROOT32_ENABLE_PARTICLES=0` | ~6 KB | ~10 KB |
| `PIXELROOT32_ENABLE_TOUCH=0` | ~200 bytes | ~2 KB |
| `PIXELROOT32_ENABLE_DIRTY_REGIONS=0` | -64 to -226 bytes | ~1 KB |

## Build Profiles (platformio.ini)

> **Note:** `PIXELROOT32_ENABLE_DIRTY_REGIONS` can be added to any profile for selective framebuffer clearing. It is not enabled by default in any profile.

```ini
[profile_full]         ; All features enabled
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=1
    -D PIXELROOT32_ENABLE_PARTICLES=1
    -D PIXELROOT32_ENABLE_UI_SYSTEM=1
    -D PIXELROOT32_ENABLE_DIRTY_REGIONS=1

[profile_arcade]       ; Audio + Physics + Particles, no UI
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=1
    -D PIXELROOT32_ENABLE_PARTICLES=1
    -D PIXELROOT32_ENABLE_UI_SYSTEM=0

[profile_puzzle]       ; Audio + UI only, no physics/particles
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=0
    -D PIXELROOT32_ENABLE_PARTICLES=0
    -D PIXELROOT32_ENABLE_UI_SYSTEM=1

[profile_retro]        ; Minimal: no subsystems
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=0
    -D PIXELROOT32_ENABLE_PHYSICS=0
    -D PIXELROOT32_ENABLE_PARTICLES=0
    -D PIXELROOT32_ENABLE_UI_SYSTEM=0
```

### Recommended Profiles by Game Type

| Game Type | Recommended Profile | Rationale |
|-----------|-------------------|-----------|
| Arcade (shooters, platformers) | `arcade` or `full` | Physics + particles + optional UI |
| Puzzle / Casual | `puzzle` | UI for menus, simple collision logic |
| Retro / Minimal | `retro` | Minimal footprint, custom collision |
| Educational / Tool | `puzzle` or custom | UI for menus |

## Core Constants

| Constant | Default | Description |
|----------|---------|-------------|
| `DISPLAY_WIDTH` | `LOGICAL_WIDTH` | Deprecated alias for `LOGICAL_WIDTH`. |
| `DISPLAY_HEIGHT` | `LOGICAL_HEIGHT` | Deprecated alias for `LOGICAL_HEIGHT`. |
| `PHYSICAL_DISPLAY_WIDTH` | `240` | Actual hardware display width in pixels. |
| `PHYSICAL_DISPLAY_HEIGHT` | `240` | Actual hardware display height in pixels. |
| `LOGICAL_WIDTH` | `PHYSICAL_DISPLAY_WIDTH` | Internal rendering width. Engine scales to physical. |
| `LOGICAL_HEIGHT` | `PHYSICAL_DISPLAY_HEIGHT` | Internal rendering height. Engine scales to physical. |
| `DISPLAY_ROTATION` | `0` | Display rotation (0, 90, 180, 270). |
| `X_OFF_SET` | `0` | Horizontal coordinate offset for hardware alignment. |
| `Y_OFF_SET` | `0` | Vertical coordinate offset for hardware alignment. |
| `MAX_SCENES` | `8` | Maximum number of scenes in the scene stack. |
| `MAX_LAYERS` | `4` | Maximum number of layers per scene. |
| `MAX_ENTITIES` | `64` | Maximum entities per scene. |
| `MAX_TILESET_SIZE` | `256` | Maximum number of tiles in a tileset. |
| `MAX_BACKGROUND_PALETTE_SLOTS` | `8` | Background palette slots for multi-palette tilemaps (2bpp/4bpp). |
| `MAX_SPRITE_PALETTE_SLOTS` | `8` | Sprite palette slots for multi-palette sprites (2bpp/4bpp). |
| `PHYSICS_MAX_ENTITIES` | `64` | Maximum entities in the physics system. |
| `PHYSICS_MAX_PAIRS` | `128` | Maximum collision pairs considered in broadphase. |
| `PHYSICS_MAX_CONTACTS` | `128` | Maximum simultaneous contacts in the physics solver. |
| `PIXELROOT32_VELOCITY_ITERATIONS` | `2` | Number of impulse solver passes per frame. |
| `SPATIAL_GRID_CELL_SIZE` | `32` | Size of each cell in the broadphase grid (pixels). |
| `SPATIAL_GRID_MAX_ENTITIES_PER_CELL` | `24` | (Legacy) max entities per cell. |
| `SPATIAL_GRID_MAX_STATIC_PER_CELL` | `12` | Max static actors per grid cell. |
| `SPATIAL_GRID_MAX_DYNAMIC_PER_CELL` | `12` | Max dynamic actors per grid cell. |
| `PR32_HAS_FPU_MACRO` | Auto-detected | Set to `1` if target has hardware FPU (ESP32, ESP32-S3, ESP32-P4, native). Undefined for C-series. |
| `PR32_FORCE_FIXED` | Undefined | User override: undefines `PR32_HAS_FPU_MACRO` to force Fixed16 math. |

## Custom Scene Limits

The engine defines default limits in `platforms/EngineConfig.h`: `MAX_LAYERS` (default 4) and `MAX_ENTITIES` (default 64). These are guarded with `#ifndef`, so you can override them from your project without modifying the engine.

**Compiler flags (recommended)**

In your project (e.g. in `platformio.ini`), add the defines to `build_flags`:

```ini
build_flags =
    -DMAX_LAYERS=5
    -DMAX_ENTITIES=64
```

## Related Documentation

- [API Reference](index.md) - Main index
- [Platform Compatibility Guide](../guide/platform-compatibility.md)
- [Extending PixelRoot32](../guide/extending-pixelroot32.md)