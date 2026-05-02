# API Reference: Core Module

> **Source of truth:**
> - `include/core/Engine.h`
> - `include/core/Entity.h`
> - `include/core/Scene.h`
> - `include/core/SceneManager.h`
> - `include/platforms/PlatformCapabilities.h`
> - `include/graphics/DisplayConfig.h`

## Overview

This document covers the core engine classes, entity system, and scene management in PixelRoot32. The `Engine` acts as the central hub, initializing and managing the Renderer, InputManager, and SceneManager. It runs the main game loop, handling timing (delta time), updating the current scene, and rendering frames.

## Key Concepts

### Engine

The main engine class that manages the game loop and core subsystems. Each iteration calls **`update()`**; **`draw()`** and **`present()`** run only when **`SceneManager::aggregateShouldRedrawFramebuffer()`** is `true` (any stacked scene may request a pass). 

### Entity

Abstract base class for all game objects. Entities are the fundamental building blocks of the scene. They have a position, size, and lifecycle methods (`update`, `draw`).

**Properties Summary:**
- Position (`x`, `y`) in world space.
- Dimensions (`width`, `height`).
- Visibility and enabled state.
- `renderLayer`: Logical render layer (0 = background, 1 = gameplay, 2 = UI).

**Modular Compilation Notes:**
Specialized subclasses may be affected by compilation flags:
- **UI Elements**: Requires `PIXELROOT32_ENABLE_UI_SYSTEM=1`
- **Physics Actors**: Requires `PIXELROOT32_ENABLE_PHYSICS=1`
- **Particle Systems**: Requires `PIXELROOT32_ENABLE_PARTICLES=1`

### Scene

Represents a game level or screen containing entities. A Scene manages a collection of Entities, an optional **PhysicsScheduler**, and a **CollisionSystem**. It is responsible for updating and drawing all entities it contains.

**Overriding Scene Limits:**
The engine defines default limits in `platforms/EngineConfig.h`: `MAX_LAYERS` (default 3) and `MAX_ENTITIES` (default 32). In your project (e.g. `platformio.ini`), add defines to `build_flags` to override:
```ini
build_flags =
    -DMAX_LAYERS=5
    -DMAX_ENTITIES=64
```

### SceneManager

Manages the stack of active scenes. Allows for scene transitions (replacing) and stacking (push/pop), useful for pausing or menus. `aggregateShouldRedrawFramebuffer()` ensures that menus don't suppress background scenes that still need drawing.

### SceneArena (Memory Management)

An optional memory arena for zero-allocation scenes (enabled via `PIXELROOT32_ENABLE_SCENE_ARENA`). Pre-allocates a fixed memory block for temporary data or entity storage, avoiding heap fragmentation on embedded devices.
- **Benefits**: Predictable memory usage, no `new`/`delete` in the scene, reduced fragmentation.
- **Costs**: Fixed buffer size; freed only when the arena is reset or the scene ends.

## Configuration & Structures

### PlatformCapabilities

Detected hardware capabilities, used to optimize task pinning and threading.

| Field | Description |
|-------|-------------|
| `hasDualCore` | True if the hardware has more than one CPU core. |
| `coreCount` | Total number of CPU cores detected. |
| `audioCoreId` | Recommended CPU core for audio tasks. |
| `mainCoreId` | Recommended CPU core for the main game loop. |
| `audioPriority` | Recommended priority for audio tasks. |

### DisplayConfig

Configuration settings for display initialization and scaling.

| Field | Description |
|-------|-------------|
| `type` | Display type (ST7789, ST7735, OLED, NONE, CUSTOM). |
| `rotation` | Display rotation (0-3). |
| `physicalWidth` / `physicalHeight` | Actual hardware resolution. |
| `logicalWidth` / `logicalHeight` | Virtual rendering resolution. |
| `xOffset` / `yOffset` | Offsets for hardware alignment. |
| **Pins** | `clockPin`, `dataPin`, `csPin`, `dcPin`, `resetPin`, `useHardwareI2C`. |

## Optional: Debug Statistics Overlay

When the engine is built with the preprocessor define **`PIXELROOT32_ENABLE_DEBUG_OVERLAY`**, it draws a technical overlay with real-time metrics.

- **FPS**: Frames per second (green).
- **RAM**: Memory used in KB (cyan). ESP32 specific.
- **CPU**: Estimated processor load percentage based on frame processing time (yellow).

The metrics are drawn in the top-right area of the screen, fixed and independent of the camera. Values are recalculated and formatted only every **16 frames** (`DEBUG_UPDATE_INTERVAL`); the cached strings are drawn every frame. This ensures minimal overhead while providing useful development data.

## Related Types

- `Engine` → `include/core/Engine.h`
- `Entity` → `include/core/Entity.h`
- `Scene` → `include/core/Scene.h`
- `SceneManager` → `include/core/SceneManager.h`
- `SceneArena` → `include/core/Scene.h`
- `PlatformCapabilities` → `include/platforms/PlatformCapabilities.h`
- `DisplayConfig` → `include/graphics/DisplayConfig.h`

## Related Documentation

- [API Reference](index.md) - Main index
- [Physics Module](physics.md) - Physics actors and collision system
- [Graphics Module](graphics.md) - Rendering and sprites
- [UI Module](ui.md) - User interface system