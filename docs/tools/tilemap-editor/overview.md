# Tilemap Editor Overview

> **Quick Index**
>
> - [Features](#key-features-summary)
> - [Documentation Structure](#documentation-structure)
> - [Getting Started](#getting-started)
> - [Next Steps](#next-steps)

The **PixelRoot32 Tilemap Editor** is a visual tool for creating tile-based maps with multi-layer support, optimized for ESP32 hardware and the PixelRoot32 renderer.

::: tip Premium module

The Tilemap Editor is part of the **Tool Suite**. Licensing and downloads: [pixelroot32.com](https://pixelroot32.com).

:::

## What it does

- **Paint** tiles on canvas with layers
- **Manage tilesets** from PNG/JPG/BMP imports
- **Multiple scenes** in one project with shared tilesets
- **Onion skinning** — overlay scenes for alignment
- **Layers** — up to **8 layers** per scene, **palette slots P0-P7** for multi-palette
- **Tile animations** with live preview
- **Tile attributes** — collision metadata and export rules
- **Export** — C++ code ready for ESP32

## Key Features (Summary)

| Area | Highlights |
|------|------------|
| Tools | Brush, eraser, rectangle, pipette, attribute, animation eyedropper, live preview |
| Scenes | Multiple maps, onion skin, per-scene size |
| Layers | Up to 8, visibility, reorder, palette slot per layer |
| Tilesets | Multi-tileset, zoom, auto tile size |
| Export | Scene `.h`/`.cpp`, animations, `setBackgroundCustomPaletteSlot()` |

## Documentation Structure

The documentation is organized in **4 guide levels**:

| Guide | Level | Description |
|-------|-------|-------------|
| [Quick Start](/tools/tilemap-editor/quick-start) | ⭐ Beginner | 5-minute quick start |
| [Usage Guide](/tools/tilemap-editor/usage-guide) | ⭐/⭐⭐ Basic/Intermediate | Essential features |
| [Advanced Guide](/tools/tilemap-editor/advanced-guide) | ⭐⭐/⭐⭐⭐ Advanced | Multi-palette, animations, attributes |
| [Technical Reference](/tools/tilemap-editor/technical-reference) | ⭐⭐⭐ Advanced | API, limits, data formats |

## Data Formats

### Project (`.pr32scene` / `.pr32scene.bin`)

- **JSON** — human-readable, git-friendly
- **Binary** — up to 335× smaller, 10× faster

### Exported C++

- Scene pair: `scene_name.h` / `scene_name.cpp`
- Optional: `scene_name_animations.h` / `.cpp`
- Multi-palette uses per-layer slot setup

## Getting started

1. **New project** — tile size, map dimensions, target resolution
2. **Import tilesets** — use menu or drag into panel
3. **Add layers** — background, collision, detail, etc.
4. **Paint** on the canvas
5. **Export to C++** — link generated files to engine

For detailed walkthrough, see the [Quick Start Guide](/tools/tilemap-editor/quick-start).

## Next steps

- [Installation](/tools/tilemap-editor/installation)
- [Quick Start](/tools/tilemap-editor/quick-start) - First map in 5 minutes
- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features

## See also

- [Tools overview](/tools/)
- [Graphics Techniques](/guide/graphics-techniques.md)
- [Tile animation](../../architecture/tile-animation.md)
