# API Reference: Configuration

This document covers global configuration options, build flags, and compile-time constants for the PixelRoot32 Game Engine.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

---

## Platform Macros (Build Flags)

| Macro | Description | Default (ESP32) |
|-------|-------------|-----------------|
| `PR32_DEFAULT_AUDIO_CORE` | CPU core assigned to audio tasks. | `0` |
| `PR32_DEFAULT_MAIN_CORE` | CPU core assigned to the main game loop. | `1` |
| `PIXELROOT32_NO_DAC_AUDIO` | Disable Internal DAC support on classic ESP32. | Enabled |
| `PIXELROOT32_NO_I2S_AUDIO` | Disable I2S audio support. | Enabled |
| `PIXELROOT32_USE_U8G2_DRIVER` | Enable U8G2 display driver support for monochromatic OLEDs. | Disabled |
| `PIXELROOT32_NO_TFT_ESPI` | Disable default TFT_eSPI driver support. | Enabled |

---

## Modular Compilation Flags

| Macro | Description | Default |
|-------|-------------|---------|
| `PIXELROOT32_ENABLE_AUDIO` | Enable audio subsystem (AudioEngine + MusicPlayer). | `1` |
| `PIXELROOT32_ENABLE_PHYSICS` | Enable physics system (CollisionSystem). | `1` |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | Enable UI system (UIButton, UILabel, etc.). | `1` |
| `PIXELROOT32_ENABLE_PARTICLES` | Enable particle system. | `1` |
| `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Enable FPS/RAM/CPU debug overlay. | Disabled |
| `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | Enable tile animation system. | `1` |
| `PIXELROOT32_ENABLE_2BPP_SPRITES` | Enable 2bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_4BPP_SPRITES` | Enable 4bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_SCENE_ARENA` | Enable scene memory arena. | Disabled |
| `PIXELROOT32_ENABLE_PROFILING` | Enable profiling hooks in physics pipeline. | Disabled |
| `PIXELROOT32_ENABLE_TOUCH` | Enable automatic touch processing in Engine (mouse-to-touch on Native, touch point injection on ESP32). | `0` (disabled) |
| `PIXELROOT32_ENABLE_TILEMAP_OPTIMIZATION` | Enable tilemap optimizations (TileCache, ChunkManager, DirtyTileTracker). | `1` |
| `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` | Enable **`StaticTilemapLayerCache`** (4bpp direct logical framebuffer snapshot). Set `0` to save ~W×H RAM or force full redraw. | `1` (`PlatformDefaults.h`) |
| `PIXELROOT32_TILE_CACHE_SIZE` | LRU cache size for pre-rendered tiles. | `16` |
| `PIXELROOT32_DIRTY_TRACKER_SIZE` | Number of tiles to track for animation changes. | `256` |
| `PIXELROOT32_CHUNK_SIZE` | Chunk size for viewport culling (tiles per chunk). | `8` |
| `PIXELROOT32_DEBUG_MODE` | Enable unified logging system. | Disabled |
| `PIXELROOT32_ENABLE_PHYSICS_FIXED_TIMESTEP` | Enable PhysicsScheduler for consistent physics across variable frame rates. | `1` (profile_full/arcade) |
| `PIXELROOT32_VELOCITY_DAMPING` | Per-frame velocity damping factor (0.0-1.0). | `0.999` |
| `PIXELROOT32_MAX_VELOCITY` | Maximum velocity cap in units/s. | `500` |
| `PIXELROOT32_HAS_FAST_RSQRT` | Enable fast reciprocal square root (1/sqrt). | `1` (profile_full/arcade) |

---

## Memory Savings by Subsystem

| Subsystem Disabled | RAM Savings | Flash Savings |
|-------------------|-------------|--------------|
| `PIXELROOT32_ENABLE_AUDIO=0` | ~8 KB | ~15 KB |
| `PIXELROOT32_ENABLE_PHYSICS=0` | ~12 KB | ~25 KB |
| `PIXELROOT32_ENABLE_UI_SYSTEM=0` | ~4 KB | ~20 KB |
| `PIXELROOT32_ENABLE_PARTICLES=0` | ~6 KB | ~10 KB |
| `PIXELROOT32_ENABLE_TOUCH=0` | ~200 bytes | ~2 KB |

---

## Build Profiles (platformio.ini)

```ini
[profile_full]         ; All features enabled
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=1
    -D PIXELROOT32_ENABLE_PARTICLES=1
    -D PIXELROOT32_ENABLE_UI_SYSTEM=1

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

---

## Recommended Profiles by Game Type

| Game Type | Recommended Profile | Rationale |
|-----------|-------------------|-----------|
| Arcade (shooters, platformers) | `arcade` or `full` | Physics + particles + optional UI |
| Puzzle / Casual | `puzzle` | UI for menus, simple collision logic |
| Retro / Minimal | `retro` | Minimal footprint, custom collision |
| Educational / Tool | `puzzle` or custom | UI for menus |

---

## Constants

- **`DISPLAY_WIDTH`**
    The width of the display in pixels. Default is `240`.

- **`DISPLAY_HEIGHT`**
    The height of the display in pixels. Default is `240`.

- **`int xOffset`**
    The horizontal offset for the display alignment. Default is `0`.

- **`int yOffset`**
    The vertical offset for the display alignment. Default is `0`.

- **`PHYSICS_MAX_PAIRS`**
    Maximum number of collision pairs considered in broadphase. Default is `128`.

- **`PHYSICS_MAX_CONTACTS`**
    Maximum number of simultaneous contacts in the solver (fixed pool, no heap per frame). Default is `128`. When exceeded, additional contacts are dropped.

- **`VELOCITY_ITERATIONS`**
    Number of impulse solver passes per frame. Higher values improve stacking stability but increase CPU load. Default is `2`.

- **`SPATIAL_GRID_CELL_SIZE`**
    Size of each cell in the broadphase grid (in pixels). Default is `32`.

- **`SPATIAL_GRID_MAX_ENTITIES_PER_CELL`**
    Legacy: maximum entities per cell when using a single grid. Default is `24`.

- **`SPATIAL_GRID_MAX_STATIC_PER_CELL`**
    Maximum static (immovable) actors per grid cell. Default is `12`. Used by the static layer of the spatial grid.

- **`SPATIAL_GRID_MAX_DYNAMIC_PER_CELL`**
    Maximum dynamic (RIGID/KINEMATIC) actors per grid cell. Default is `12`. Used by the dynamic layer of the spatial grid.

---

## Custom Scene Limits

The engine defines default limits in `platforms/EngineConfig.h`: `MAX_LAYERS` (default 3) and `MAX_ENTITIES` (default 32). These are guarded with `#ifndef`, so you can override them from your project without modifying the engine.

**Compiler flags (recommended)**

In your project (e.g. in `platformio.ini`), add the defines to `build_flags`:

```ini
build_flags =
    -DMAX_LAYERS=5
    -DMAX_ENTITIES=64
```

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [Platform Compatibility Guide](../PLATFORM_COMPATIBILITY.md)
- [Extending PixelRoot32](../EXTENDING_PIXELROOT32.md)