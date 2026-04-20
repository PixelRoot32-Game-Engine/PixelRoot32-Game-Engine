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
| **Audio NES** | [Audio Subsystem](architecture/ARCH_AUDIO_SUBSYSTEM.md) | 4-channel NES-style: shared **`ApuCore`**, `AudioScheduler`, backends — see [MusicPlayer Guide](MUSIC_PLAYER_GUIDE.md), [API Audio](api/API_AUDIO.md) |
| **Physics** | [Physics Subsystem](architecture/ARCH_PHYSICS_SUBSYSTEM.md) | Flat Solver, collisions, CCD (ex-PHYSICS_*) |
| **Memory** | [Memory System](architecture/ARCH_MEMORY_SYSTEM.md) | Smart pointers, RAII, ESP32 DRAM (ex-MEMORY_*) |
| **Resolution Scaling** | [Resolution Scaling](architecture/ARCH_RESOLUTION_SCALING.md) | Logical vs physical resolution (ex-RESOLUTION_*) |
| **Tile Animation** | [Tile Animation](architecture/ARCH_TILE_ANIMATION.md) | Lookup tables, O(1) resolve; see also static layer cache in [ESP32 rendering](#esp32-rendering-pipeline-and-tilemap-caching) (ex-TILE_ANIMATION_*) |
| **Touch Input** | [Touch Input](architecture/ARCH_TOUCH_INPUT.md) | Pipeline, XPT2046, calibration (ex-TOUCH_INPUT) |
| **Display Bottleneck Optimization** | [Display Optimization](#display-bottleneck-optimization-subsystem) | Dirty rect tracking, partial updates, color depth config (v1.3.0+) |
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

**Audio (lectura recomendada):** [Arquitectura audio NES](architecture/ARCH_AUDIO_SUBSYSTEM.md) (diseño e implementación) → [API Audio](api/API_AUDIO.md) (clases y tipos) → [MusicPlayer Guide](MUSIC_PLAYER_GUIDE.md) (melodías y multi-pista).

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
│  ├─ Particle System                                         │
│  └─ Display Optimization (DirtyRect, PartialUpdate)         │
├─────────────────────────────────────────────────────────────┤
│  LAYER 2: Abstraction Layer                                 │
│  ├─ DrawSurface (Bridge Pattern)                            │
│  ├─ AudioScheduler → ApuCore (Strategy + núcleo compartido) │
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
| Display Bottleneck Optimization | `ENABLE_PARTIAL_UPDATES` | Enabled (v1.3.0) |
| Debug Overlay | `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Disabled |
| Debug Dirty Regions | `PIXELROOT32_DEBUG_DIRTY_REGIONS` | Disabled |

---

## ESP32 rendering pipeline and tilemap caching

On ESP32 with **TFT_eSPI** (`TFT_eSPI_Drawer`), the logical framebuffer is typically an **8-bit color-depth sprite** (`TFT_eSprite`). Each frame:

1. **`Renderer::beginFrame()`** obtains a pointer to that buffer via **`DrawSurface::getSpriteBuffer()`** (when the driver supports it), clears the buffer, then draws the scene.
2. **2bpp / 4bpp tilemaps and sprites** can write **directly into that buffer** (matching TFT_eSPI's 8bpp packing for RGB565), avoiding a virtual `drawPixel` per pixel where possible.
3. **`present()` / `sendBuffer()`** converts logical 8bpp rows to **RGB565** using a LUT and pushes pixels to the panel via **DMA** (see [Driver Layer](architecture/ARCH_LAYER_DRIVERS.md), [System Layer / Renderer](architecture/ARCH_LAYER_SYSTEMS.md)).

### Display Bottleneck Optimization Subsystem (v1.3.0+)

Version 1.3.0 introduces **display bottleneck optimization** to reduce SPI transfer time on slow displays. The system consists of three coordinated components:

#### Architecture Overview

```
┌────────────────────────────────────────────────────────┐
│         Display Bottleneck Optimization                │
├────────────────────────────────────────────────────────┤
│                                                        │
│  ┌───────────────────┐   ┌──────────────────────────┐  │
│  │  DirtyRectTracker │   │  PartialUpdateController │  │
│  │                   │   │                          │  │
│  │  - Bitmap-based   │──▶│  - Threshold decision    │  │
│  │  - 8x8 blocks     │   │  - Full vs Partial mode  │  │
│  │  - O(1) marking   │   │  - Region combining      │  │
│  └───────────────────┘   └───────────┬──────────────┘  │
│                                      │                 │
│                                      ▼                 │
│  ┌──────────────────┐   ┌──────────────────────────┐   │
│  │ ColorDepthManager│◀──│  TFT_eSPI_Drawer         │   │
│  │                  │   │                          │   │
│  │  - 24/16/8-bit   │   │  - sendBufferScaled()    │   │
│  │  - Palette mgmt  │   │  - PartialUpdate path    │   │
│  └──────────────────┘   └──────────────────────────┘   │
└────────────────────────────────────────────────────────┘
```

#### 5-Step Rendering Pipeline (v1.3.0+)

With display bottleneck optimization enabled, the rendering pipeline follows these steps:

```
Step 1: Dirty Marking
        ├── drawTileDirect() → markDirty(x,y,w,h)
        ├── drawSprite() → markDirty(bounding box)
        └── Auto-mark: markDirty() called automatically after each draw operation

Step 2: Region Batching
        ├── DirtyRectTracker accumulates all markDirty() calls
        └── Uses 8x8 block bitmap (150 bytes for 320x240)

Step 3: Threshold Decision (PartialUpdateController)
        ├── Calculate dirty ratio = dirtyPixels / totalPixels
        ├── If dirty% > MAX_DIRTY_RATIO_PERCENT (default 70%): use Full mode
        └── Otherwise: use Partial mode

Step 4: Region Combining (optional)
        ├── ENABLED: Merge adjacent 8x8 blocks into larger regions
        └── DISABLED: Each 8x8 block sent separately

Step 5: Partial vs Full Update
        ├── Full: send entire framebuffer via DMA
        └── Partial: send only combined regions via setAddrWindow + pushPixelsDMA
```

#### Components

##### DirtyRectTracker

**File**: `include/graphics/DirtyRectTracker.h`, `src/graphics/DirtyRectTracker.cpp`

Tracks which regions of the screen have been modified using a bitmap-based approach. The grid and bitmap size adjust automatically to the logical resolution during engine initialization.

| Property | Value |
|----------|-------|
| Grid granularity | 8x8 pixels |
| Grid dimensions | Dynamic (e.g. 40x30 for 320x240) |
| Bitmap size | Dynamic (e.g. 150 bytes for 320x240) |
| Allocation | Heap (once in `configure()` / `init()`) |
| Mark operation | O(1) |
| Merge operation | O(grid size) |

**Key Methods**:
- `configure(w, h)`: Re-allocate bitmap for new resolution
- `markDirty(x, y, w, h)`: Mark region as dirty
- `combineRegions()`: Merge adjacent blocks
- `hasDirtyRegions()`: Check if any regions dirty
- `getRegions()`: Get merged dirty rectangles

##### PartialUpdateController

**File**: `include/graphics/PartialUpdateController.h`, `src/graphics/PartialUpdateController.cpp`

Coordinates dirty region tracking and decides update strategy.

| Property | Default | Range |
|----------|---------|-------|
| Mode | Partial | Full/Partial |
| Min region pixels | 256 | 64-4096 |
| Dirty ratio threshold | 70% | 0-100% |

**Key Methods**:
- `beginFrame()`: Prepare tracking for frame
- `endFrame(width, height)`: Finalize dirty regions
- `shouldUsePartial()`: Check if partial is beneficial
- `getRegions()`: Get dirty regions to send
- `setMinRegionPixels(n)`: Tune minimum region size

**Benchmark APIs**:
- `getLastRegionCount()`: Regions sent in last frame
- `getLastTotalSentPixels()`: Pixels sent in last frame

##### ColorDepthManager

**File**: `include/graphics/ColorDepthManager.h`, `src/graphics/ColorDepthManager.cpp`

Manages color depth for display output to reduce SPI transfer.

| Depth | Format | Bytes/Pixel | Transfer Reduction |
|-------|--------|-------------|--------------------|
| 24 | RGB888 | 3 | 0% (baseline) |
| 16 | RGB565 | 2 | 33% |
| 8 | Indexed | 1 | 66% |
| 4 | Indexed | 0.5 | 83% (Not Implemented) |

**Key Methods**:
- `setDepth(bits)`: Set color depth (24/16/8). Returns `false` for 4-bit.
- `needsPaletteConversion()`: Check if palette needed
- `setCustomPalette()`: Set custom 256-color palette
- `getTransferRatio()`: Get reduction vs 24-bit

#### Configuration Options

| Flag | Default | Valid Range | Description |
|------|---------|-------------|-------------|
| `ENABLE_PARTIAL_UPDATES` | 1 | 0-1 | Enable partial screen updates |
| `DISPLAY_COLOR_DEPTH` | 16 | 24, 16, 8 | Color depth in bits |
| `MAX_DIRTY_RATIO_PERCENT` | 70 | 0-100 | Threshold for full update |
| `ENABLE_DIRTY_RECT_COMBINE` | 1 | 0-1 | Enable region combining |
| `PIXELROOT32_DEBUG_DIRTY_REGIONS` | 0 | 0-1 | Show dirty region borders |

#### Performance Impact

| Scenario | Without Optimization | With Optimization | Savings |
|----------|---------------------|------------------|---------|
| Static scene (tiles only) | ~115,200 bytes/frame | ~5,000 bytes/frame | ~95% |
| UI button press | ~115,200 bytes/frame | ~2,500 bytes/frame | ~98% |
| Player movement | ~57,600 bytes/frame | ~30,000 bytes/frame | ~48% |
| Full screen change | ~115,200 bytes/frame | ~115,200 bytes/frame | 0% |

**Memory Overhead**: ~300 bytes (DirtyRectTracker bitmap + partial controller state)

#### BaseDrawSurface Partial Update API

The partial update API is defined in `BaseDrawSurface.h` with default no-op implementations:

```cpp
// Mark region as dirty (auto-called by default)
virtual void markDirty(int x, int y, int width, int height);

// Clear for next frame
virtual void clearDirtyFlags();

// Check if partial updates beneficial
virtual bool hasDirtyRegions() const;

// Enable/disable partial updates
virtual void setPartialUpdateEnabled(bool enabled);
virtual bool isPartialUpdateEnabled() const;

// Frame lifecycle
virtual void beginFrame();
virtual void endFrame();

// Color depth control
virtual void setColorDepth(int depth);

// Auto-mark dirty control
virtual void setAutoMarkDirty(bool enabled);
virtual bool isAutoMarkDirty() const;

// Debug overlay
virtual void setDebugDirtyRegions(bool enabled);
virtual bool isDebugDirtyRegions() const;
```

**Backward Compatibility**: All methods have default no-op implementations in `BaseDrawSurface`. Existing games work without modifications. Only drivers that support partial updates (e.g., `TFT_eSPI_Drawer`) override these methods.

### Static tilemap layer cache (engine + scenes)

The engine provides **`pixelroot32::graphics::StaticTilemapLayerCache`** (`include/graphics/StaticTilemapLayerCache.h`): a **4bpp tilemap** helper that, when **`getSpriteBuffer()`** is non-null, can snapshot the logical framebuffer after drawing a **static** group of **`TileMap4bppDrawSpec`** entries, then on subsequent frames **`memcpy`** that snapshot back and redraw only the **dynamic** group until the **camera sample** (`-getXOffset()`, `-getYOffset()`) changes or **`invalidate()`** runs.

- **Allocation:** **`allocateForLogicalSize`** / **`allocateForRenderer`** in **`Scene::init()`** (about **W×H** bytes via `std::malloc`; not in `draw`/`update`).
- **Opt-out:** build flag **`PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE=0`**, or **`setFramebufferCacheEnabled(false)`**.
- **Reflection in config:** **`pixelroot32::platforms::config::EnableStaticTilemapFbCache`** (`EngineConfig.h`).

**Example:** **`examples/animated_tilemap`** — **`AnimatedTilemapScene`** owns a **`StaticTilemapLayerCache`**, registers **background** as **static** and **ground + details** as **dynamic** (any split is possible via spec lists).

### Present-path savings (optional)

- **Opción A (implementada):** `Scene::shouldRedrawFramebuffer()` — el **`Engine`** omite **`draw()`** + **`present()`** cuando la escena devuelve `false`. **`AnimatedTilemapScene`** usa firmas de **`TileAnimationManager::getVisualSignature()`** y muestras de cámara para detectar frames sin cambio visual (p. ej. entre avances de frame de animación). Con **`PIXELROOT32_ENABLE_DEBUG_OVERLAY`** el motor **siempre** redibuja para mantener el overlay coherente.
- **Opción B (implemented v1.3.0):** **dirty region / partial updates** via **`TFT_eSPI_Drawer::sendBufferScaled`**: uses dirty bitmap tracking and sends only modified regions via `setAddrWindow` + `pushPixelsDMA`. Saves SPI when a fraction of the panel changes. Enabled by default in v1.3.0 via `ENABLE_PARTIAL_UPDATES=1`. See [Driver Layer](architecture/ARCH_LAYER_DRIVERS.md).

**Game / scene developer contract**

- Call **`invalidate()`** (or a scene wrapper like **`invalidateStaticLayerCache()`**) when something inside the **static** group changes visually, e.g. after **`TileAnimationManager::step(deltaTime)`** on a layer in that group, or when mutating **indices** / **palettes** / **`runtimeMask`** on those maps.
- Layers in the **dynamic** group are drawn every frame on the fast path—no invalidation needed for **`step()`** on **dynamic-only** animators.
- **Scroll:** cache rebuilds when the camera sample changes; no extra invalidation solely for scroll.
- **`getSpriteBuffer() == nullptr`:** full redraw of all groups every frame; no snapshot used.
- **Dirty rect tracking (v1.3.0+):** The partial update system automatically tracks dirty regions. For custom rendering (drawing directly to framebuffer), call **`renderer.markDirty(x, y, w, h)`** to mark modified regions. Auto-mark dirty is enabled by default.

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
| [MusicPlayer Guide](MUSIC_PLAYER_GUIDE.md) | Música de fondo, multi-pista, tempo/BPM |
