# Architecture Document - PixelRoot32 Game Engine

## Quick Navigation

The architecture documentation is organized into **layers** (hardware to game code) and **subsystem deep dives**.

### Layer Architecture

| Layer | Document | Description |
|-------|----------|-------------|
| **Overview** | [Architecture Overview](architecture/ARCH_OVERVIEW.md) | Executive summary, design philosophy, layer diagram |
| **Layer 0** | [Hardware Layer](architecture/ARCH_LAYER_HARDWARE.md) | ESP32, displays, audio hardware, PC simulation |
| **Layer 1** | [Driver Layer](architecture/ARCH_LAYER_DRIVERS.md) | TFT_eSPI, U8G2, SDL2, AudioBackends |
| **Layer 2** | [Abstraction Layer](architecture/ARCH_LAYER_ABSTRACTION.md) | DrawSurface, PlatformMemory, Logging, Math |
| **Layer 3** | [System Layer](architecture/ARCH_LAYER_SYSTEMS.md) | Renderer, Audio, Physics, UI subsystems |
| **Layer 4** | [Scene Layer](architecture/ARCH_LAYER_SCENE.md) | Engine, SceneManager, Entity, Actor hierarchy |

### Subsystem Deep Dives

| Subsystem | Document | Description |
|-----------|----------|-------------|
| **Audio NES** | [Audio Subsystem](architecture/ARCH_AUDIO_SUBSYSTEM.md) | 4-channel NES-style audio (ex-AUDIO_NES_*) |
| **Physics** | [Physics Subsystem](architecture/ARCH_PHYSICS_SUBSYSTEM.md) | Flat Solver, collisions, CCD (ex-PHYSICS_*) |
| **Memory** | [Memory System](architecture/ARCH_MEMORY_SYSTEM.md) | Smart pointers, RAII, ESP32 DRAM (ex-MEMORY_*) |
| **Resolution Scaling** | [Resolution Scaling](architecture/ARCH_RESOLUTION_SCALING.md) | Logical vs physical resolution (ex-RESOLUTION_*) |
| **Tile Animation** | [Tile Animation](architecture/ARCH_TILE_ANIMATION.md) | Lookup tables, O(1) resolve; see also static layer cache in [ESP32 rendering](#esp32-rendering-pipeline-and-tilemap-caching) (ex-TILE_ANIMATION_*) |
| **Touch Input** | [Touch Input](architecture/ARCH_TOUCH_INPUT.md) | Pipeline, XPT2046, calibration (ex-TOUCH_INPUT) |
| **Extensibility** | [Extending PixelRoot32](EXTENDING_PIXELROOT32.md) | Custom drivers, configuration |

### API Reference

For class-level API documentation, see `docs/api/`:

| Module | Document |
|--------|----------|
| Configuration | [API_CONFIG.md](api/API_CONFIG.md) |
| Math | [API_MATH.md](api/API_MATH.md) |
| Core | [API_CORE.md](api/API_CORE.md) |
| Physics | [API_PHYSICS.md](api/API_PHYSICS.md) |
| Graphics | [API_GRAPHICS.md](api/API_GRAPHICS.md) |
| UI | [API_UI.md](api/API_UI.md) |
| Audio | [API_AUDIO.md](api/API_AUDIO.md) |
| Input | [API_INPUT.md](api/API_INPUT.md) |
| Platform | [API_PLATFORM.md](api/API_PLATFORM.md) |

---

## Executive Summary (Brief)

PixelRoot32 is a lightweight, modular 2D game engine written in C++17, designed primarily for ESP32 microcontrollers, with a native simulation layer for PC (SDL2).

**Key Features**:
- Scene-based architecture inspired by Godot Engine
- Modular compilation with `PIXELROOT32_ENABLE_*` flags
- Logical/physical resolution independence
- NES-style 4-channel audio
- "Flat Solver" physics with specialized Actor types
- Multi-platform support through driver abstraction

**Design Philosophy**:
- **Modularity**: Subsystems can be used independently
- **Selective Compilation**: Exclude unused subsystems to save RAM/flash
- **Portability**: Same code for ESP32 and PC
- **Performance**: Optimized for resource-constrained hardware

---

## Layer Hierarchy (Simplified)

```
┌─────────────────────────────────────────────────────────────┐
│  LAYER 5: Game Layer (User Code)                            │
│  └─ Your game scenes and actors                             │
├─────────────────────────────────────────────────────────────┤
│  LAYER 4: Scene Layer                                       │
│  ├─ Engine, SceneManager                                    │
│  ├─ Scene, Entity, Actor hierarchy                          │
│  └─ CollisionSystem integration                             │
├─────────────────────────────────────────────────────────────┤
│  LAYER 3: System Layer                                      │
│  ├─ Renderer, Camera2D                                      │
│  ├─ AudioEngine, MusicPlayer                                │
│  ├─ CollisionSystem (Flat Solver)                           │
│  ├─ UI System                                               │
│  ├─ InputManager                                            │
│  └─ Particle System                                         │
├─────────────────────────────────────────────────────────────┤
│  LAYER 2: Abstraction Layer                                 │
│  ├─ DrawSurface (Bridge Pattern)                            │
│  ├─ AudioScheduler (Strategy Pattern)                       │
│  ├─ PlatformMemory, PlatformCapabilities                    │
│  ├─ Math System (Scalar abstraction)                        │
│  └─ Unified Logging                                         │
├─────────────────────────────────────────────────────────────┤
│  LAYER 1: Driver Layer                                      │
│  ├─ TFT_eSPI_Drawer, U8G2_Drawer (ESP32)                    │
│  ├─ ESP32_I2S_AudioBackend, ESP32_DAC_AudioBackend          │
│  └─ SDL2_Drawer, SDL2_AudioBackend (PC)                     │
├─────────────────────────────────────────────────────────────┤
│  LAYER 0: Hardware Layer                                    │
│  ├─ ESP32/ESP32-S3/ESP32-C3 microcontrollers                │
│  ├─ Displays (ST7789, SSD1306, etc.)                        │
│  ├─ Audio (I2S, DAC, amplifiers)                            │
│  ├─ Input (buttons, touch controllers)                      │
│  └─ PC/Native (SDL2 simulation)                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Subsystem Modular Compilation

| Subsystem | Enable Flag | Default |
|-----------|-------------|---------|
| Audio | `PIXELROOT32_ENABLE_AUDIO` | Enabled |
| Physics | `PIXELROOT32_ENABLE_PHYSICS` | Enabled |
| UI System | `PIXELROOT32_ENABLE_UI_SYSTEM` | Enabled |
| Particles | `PIXELROOT32_ENABLE_PARTICLES` | Enabled |
| Touch Input | `PIXELROOT32_ENABLE_TOUCH` | Disabled |
| Tile Animations | `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | Enabled |
| Static tilemap FB snapshot (4bpp) | `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` | Enabled (`PlatformDefaults.h`) |
| Tilemap Optimization | `PIXELROOT32_ENABLE_TILEMAP_OPTIMIZATION` | Enabled |
| Debug Overlay | `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Disabled |

---

## ESP32 rendering pipeline and tilemap caching

On ESP32 with **TFT_eSPI** (`TFT_eSPI_Drawer`), the logical framebuffer is typically an **8-bit color-depth sprite** (`TFT_eSprite`). Each frame:

1. **`Renderer::beginFrame()`** obtains a pointer to that buffer via **`DrawSurface::getSpriteBuffer()`** (when the driver supports it), clears the buffer, then draws the scene.
2. **2bpp / 4bpp tilemaps and sprites** can write **directly into that buffer** (matching TFT_eSPI’s 8bpp packing for RGB565), avoiding a virtual `drawPixel` per pixel where possible.
3. **`present()` / `sendBuffer()`** converts logical 8bpp rows to **RGB565** using a LUT and pushes pixels to the panel via **DMA** (see [Driver Layer](architecture/ARCH_LAYER_DRIVERS.md), [System Layer / Renderer](architecture/ARCH_LAYER_SYSTEMS.md)).

### Static tilemap layer cache (engine + scenes)

The engine provides **`pixelroot32::graphics::StaticTilemapLayerCache`** (`include/graphics/StaticTilemapLayerCache.h`): a **4bpp tilemap** helper that, when **`getSpriteBuffer()`** is non-null, can snapshot the logical framebuffer after drawing a **static** group of **`TileMap4bppDrawSpec`** entries, then on subsequent frames **`memcpy`** that snapshot back and redraw only the **dynamic** group until the **camera sample** (`-getXOffset()`, `-getYOffset()`) changes or **`invalidate()`** runs.

- **Allocation:** **`allocateForLogicalSize`** / **`allocateForRenderer`** in **`Scene::init()`** (about **W×H** bytes via `std::malloc`; not in `draw`/`update`).
- **Opt-out:** build flag **`PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE=0`**, or **`setFramebufferCacheEnabled(false)`**.
- **Reflection in config:** **`pixelroot32::platforms::config::EnableStaticTilemapFbCache`** (`EngineConfig.h`).

**Example:** **`examples/animated_tilemap`** — **`AnimatedTilemapScene`** owns a **`StaticTilemapLayerCache`**, registers **background + ground** as **static** and **details** as **dynamic** (any split is possible via spec lists).

**Game / scene developer contract**

- Call **`invalidate()`** (or a scene wrapper like **`invalidateStaticLayerCache()`**) when something inside the **static** group changes visually, e.g. after **`TileAnimationManager::step()`** on a layer in that group, or when mutating **indices** / **palettes** / **`runtimeMask`** on those maps.
- Layers in the **dynamic** group are drawn every frame on the fast path—no invalidation needed for **`step()`** on **dynamic-only** animators.
- **Scroll:** cache rebuilds when the camera sample changes; no extra invalidation solely for scroll.
- **`getSpriteBuffer() == nullptr`:** full redraw of all groups every frame; no snapshot used.

For animation data flow and linking managers to tilemaps, see [Tile Animation](architecture/ARCH_TILE_ANIMATION.md). API surface: [API Reference — ESP32 graphics / tilemap cache](api/API_GRAPHICS.md#multi-layer-4bpp-tilemap-framebuffer-snapshot-statictilemaplayercache).

---

## Related Documentation

| Document | Description |
|----------|-------------|
| [API Reference](API_REFERENCE.md) | Complete API documentation index |
| [Getting Started](GETTING_STARTED.md) | First steps with the engine |
| [Style Guide](STYLE_GUIDE.md) | Coding conventions |
| [Platform Compatibility](PLATFORM_COMPATIBILITY.md) | Supported hardware matrix |
| [Testing Guide](TESTING_GUIDE.md) | Unit and integration testing |
| [Migration Guides](MIGRATION_v1.0.0.md) | Version upgrade guides |
