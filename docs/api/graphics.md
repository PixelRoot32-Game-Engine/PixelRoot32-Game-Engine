# API Reference: Graphics Module

> **Source of truth:**
> - `include/graphics/Renderer.h`
> - `include/graphics/Camera2D.h`
> - `include/graphics/Color.h`
> - `include/graphics/Font.h`, `include/graphics/FontManager.h`
> - `include/graphics/StaticTilemapLayerCache.h`
> - `include/graphics/DirtyGrid.h`
> - `include/graphics/TileAnimation.h`
> - `include/graphics/DrawSurface.h`, `include/graphics/BaseDrawSurface.h`
> - `include/graphics/particles/*.h`

## Overview

This document covers the rendering system, sprites, tilemaps, colors, fonts, and particle system in PixelRoot32. The `Renderer` provides a unified high-level API for drawing shapes, text, and images, abstracting the underlying hardware implementation (such as `DrawSurface`).

## Platform Optimizations (ESP32)

The engine includes several low-level optimizations for the ESP32 platform to maximize performance:

- **DMA Support**: Buffer transfers to the display are handled via DMA (`pushImageDMA`), allowing the CPU to process the next frame concurrently.
- **IRAM Execution**: Critical rendering functions (`drawPixel`, `drawSpriteInternal`, `resolveColor`, `drawTileMap`) run from internal RAM (`IRAM_ATTR`).
- **Palette Caching**: Tilemaps cache the resolved RGB565 LUT per tile.
- **Viewport Culling**: All tilemap rendering functions automatically skip tiles outside the screen boundaries.
- **Dirty Region Tracking**: Selective framebuffer clear via `DirtyGrid` (double-buffer prev/curr with 8×8 cells), reducing memset overhead.
- **Direct logical framebuffer**: The `TFT_eSPI` driver exposes an 8bpp sprite memory buffer, enabling `Renderer` to write packed 2bpp/4bpp pixels directly without virtual function overhead.

### Multi-layer 4bpp tilemap framebuffer snapshot (`StaticTilemapLayerCache`)

Avoids redrawing "static" **4bpp** tilemaps every frame. It caches the static group of tiles into an internal buffer and restores it via `memcpy` each frame, so only dynamic elements need redrawing until the camera moves.

> **Dirty Regions Interaction:** When both the static cache and Dirty Regions are enabled, the cache advises `beginFrame()` to skip its selective or full clear if a cache `memcpy` will entirely overwrite the framebuffer anyway.

## Key Concepts

### Camera2D

Manages the viewport and scrolling of the game world. Handles coordinate transformations and target following with configurable dead zones.

### Color and Palettes

The engine supports indexed colors via the `Color` enumeration.
- **PaletteType**: `PR32`, `NES`, `GB`, `GBC`, `PICO8`.
- Supports single palette mode or dual palette mode (separate background and sprite palettes).

### Font System

Uses a native bitmap font system via 1bpp sprites (`struct Font`).
- **`FONT_5X7`**: A built-in 5x7 pixel bitmap font (ASCII 32-126).
- `FontManager` manages the active default font and text width calculations.

### Sprite Structures

- **`Sprite`**: Compact 1bpp monochrome bitmap descriptor.
- **`Sprite2bpp` / `Sprite4bpp`**: Packed multi-color sprites with a local palette (requires compile flags).
- **`SpriteLayer` / `MultiSprite`**: Layered multi-color sprites built from monochrome layers.

### TileMaps

Generic descriptors for tile-based backgrounds, instantiated as `TileMap` (1bpp), `TileMap2bpp`, or `TileMap4bpp`. 
- **Tilemap rendering notes**: `drawTileMap` always applies viewport culling. For static 4bpp reuse, see `StaticTilemapLayerCache`.

### Tile Animation System

Enables frame-based tile animations (water, lava, fire) while maintaining static tilemap data. Pacing uses high-resolution wall time so animations stay correct regardless of frame skipping. 
- A `TileAnimationManager` computes the `VisualSignature` to inform the scene if a redraw is necessary.

### Dirty Region System

The Dirty Region System provides selective framebuffer clearing to reduce unnecessary `memset` operations when most of the screen stays unchanged.

- **`DirtyGrid`** (requires `PIXELROOT32_ENABLE_DIRTY_REGIONS=1`): Double-buffer design with `prev` and `curr` bit-packed grids of 8×8 pixel cells. Each cell tracks whether it was drawn to in the current frame.

- **`LayerType`** (in `include/graphics/Renderer.h`): Classifies tilemaps to enable selective tracking:
  - `LayerType::Static`: Background layers that rarely change. The system tracks dirty cells, but the layer itself doesn't mark cells as dirty.
  - `LayerType::Dynamic`: Sprites, particles, and UI that move every frame. These mark their occupied cells as dirty.

- **`forceFullRedraw()`**: Call this when a full framebuffer clear is required (scene transitions, pause menus, camera jumps). Resets the dirty grid state.

- **Debug overlay** (`setDebugDirtyCellOverlay`): Visualizes dirty cells on screen for debugging. Requires `PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING=1`.

- **Compile flag**: `PIXELROOT32_ENABLE_DIRTY_REGIONS` (default: disabled). RAM cost: 64–226 bytes depending on resolution.

> **Tip:** Use `LayerType::Static` for backgrounds that don't change—avoids unnecessary dirty cell marking.

### Tile Attribute System

Provides runtime access to custom metadata attached to tiles. Attributes are stored in Flash memory on the ESP32.

> [!IMPORTANT]
> Since attributes are stored in Flash memory on ESP32, you must use **`PIXELROOT32_STRCMP_P`** or **`PIXELROOT32_MEMCPY_P`** to compare or copy the returned values.

### Particle System

*(Requires `PIXELROOT32_ENABLE_PARTICLES=1`)*
Provides lightweight visual effects using a fixed-size pool of `Particle` objects. Managed by a `ParticleEmitter` entity.
- **ParticlePresets**: Predefined configs (`Fire`, `Explosion`, `Sparks`, `Smoke`, `Dust`).

### SpriteAnimation

Lightweight, step-based animation controller for advancing through an array of `SpriteAnimationFrame`.

### DrawSurface / BaseDrawSurface

Abstract interfaces for platform-specific drawing operations (e.g., `SDL2_Drawer`, `TFT_eSPI_Drawer`).

## Related Types

- `Renderer` → `include/graphics/Renderer.h`
- `Camera2D` → `include/graphics/Camera2D.h`
- `Color`, `PaletteType` → `include/graphics/Color.h`
- `Font`, `FontManager` → `include/graphics/FontManager.h`
- `Sprite`, `TileMap` → `include/graphics/Renderer.h`
- `DirtyGrid` → `include/graphics/DirtyGrid.h`
- `LayerType` → `include/graphics/Renderer.h`
- `ParticleEmitter`, `ParticleConfig` → `include/graphics/particles/ParticleEmitter.h`
- `TileAnimationManager` → `include/graphics/TileAnimation.h`

## Related Documentation

- [API Reference](index.md) - Main index
- [Core Module](core.md) - Engine, Entity, Scene
- [Physics Module](physics.md) - Collision system
- [UI Module](ui.md) - User interface system