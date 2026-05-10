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
| `PIXELROOT32_NO_DAC_AUDIO` | Disable Internal DAC support on classic ESP32. | Enabled |
| `PIXELROOT32_NO_I2S_AUDIO` | Disable I2S audio support. | Enabled |
| `PIXELROOT32_USE_U8G2_DRIVER` | Enable U8G2 display driver support for monochromatic OLEDs. | Disabled |
| `PIXELROOT32_NO_TFT_ESPI` | Disable default TFT_eSPI driver support. | Enabled |

## Modular Compilation Flags

| Macro | Description | Default |
|-------|-------------|---------|
| `PIXELROOT32_ENABLE_AUDIO` | Enable audio subsystem. | `1` |
| `PIXELROOT32_ENABLE_PHYSICS` | Enable physics system. | `1` |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | Enable UI system. | `1` |
| `PIXELROOT32_ENABLE_PARTICLES` | Enable particle system. | `1` |
| `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Enable FPS/RAM/CPU debug overlay. | Disabled |
| `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | Enable tile animation system. | `1` |
| `PIXELROOT32_ENABLE_2BPP_SPRITES` | Enable 2bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_4BPP_SPRITES` | Enable 4bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_SCENE_ARENA` | Enable scene memory arena. | Disabled |
| `PIXELROOT32_ENABLE_PROFILING` | Enable profiling hooks in physics pipeline. | Disabled |
| `PIXELROOT32_ENABLE_TOUCH` | Enable automatic touch processing. | `0` (disabled) |
| `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` | Enable **`StaticTilemapLayerCache`** (4bpp FB snapshot). | `1` |
| `PIXELROOT32_ENABLE_DIRTY_REGIONS` | Enable dirty-cell selective framebuffer clear (`DirtyGrid`). Requires 64–226 B RAM. | `0` |
| `PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING` | Enable dirty region profiling metrics. | `0` |
| `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK` | TFT_eSPI DMA line batch size. | `60` |
| `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK` | Fallback DMA batch size if memory fails. | `30` |
| `PIXELROOT32_DEBUG_MODE` | Enable unified logging system. | Disabled |
| `PIXELROOT32_ENABLE_PHYSICS_FIXED_TIMESTEP` | Enable PhysicsScheduler for consistent physics. | `1` |
| `PIXELROOT32_VELOCITY_DAMPING` | Per-frame velocity damping factor (0.0-1.0). | `0.999` |
| `PIXELROOT32_MAX_VELOCITY` | Maximum velocity cap in units/s. | `500` |
| `PIXELROOT32_HAS_FAST_RSQRT` | Enable fast reciprocal square root. | `1` |

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
| `DISPLAY_WIDTH` | `240` | The logical width of the display in pixels. |
| `DISPLAY_HEIGHT` | `240` | The logical height of the display in pixels. |
| `xOffset` / `yOffset` | `0` | Coordinate offsets for hardware alignment. |
| `PHYSICS_MAX_PAIRS` | `128` | Maximum collision pairs considered in broadphase. |
| `PHYSICS_MAX_CONTACTS` | `128` | Maximum simultaneous contacts in the physics solver. |
| `VELOCITY_ITERATIONS` | `2` | Number of impulse solver passes per frame. |
| `SPATIAL_GRID_CELL_SIZE` | `32` | Size of each cell in the broadphase grid (pixels). |
| `SPATIAL_GRID_MAX_ENTITIES_PER_CELL` | `24` | (Legacy) max entities per cell. |
| `SPATIAL_GRID_MAX_STATIC_PER_CELL` | `12` | Max static actors per grid cell. |
| `SPATIAL_GRID_MAX_DYNAMIC_PER_CELL` | `12` | Max dynamic actors per grid cell. |

## Custom Scene Limits

The engine defines default limits in `platforms/EngineConfig.h`: `MAX_LAYERS` (default 3) and `MAX_ENTITIES` (default 32). These are guarded with `#ifndef`, so you can override them from your project without modifying the engine.

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