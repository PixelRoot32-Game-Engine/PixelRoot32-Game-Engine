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
- **Manage tilesets** from PNG imports (JPG/BMP loadable via the file picker's "All Files" filter; PNG recommended)
- **Multiple scenes** in one project with shared tilesets
- **Isometric or orthogonal** projection, chosen **per scene**
- **Onion skinning** — overlay scenes for alignment
- **Layers** — up to **8 layers** per scene, **palette slots P0-P7** for multi-palette
- **Tile animations** with live preview
- **Tile attributes** — collision metadata and export rules
- **Player spawn** — optional per-scene start tile
- **Rooms** — optional per-scene room layer with connections between rooms
- **Export** — C++ code ready for ESP32

## Key Features (Summary)

| Area | Highlights |
|------|------------|
| Tools | Brush, eraser, rectangle, pipette, attribute, animation, live preview |
| Scenes | Multiple maps, onion skin, per-scene size, per-scene projection, optional player spawn and rooms |
| Layers | Up to 8, visibility, reorder, palette slot per layer |
| Tilesets | Multi-tileset, zoom, per-tileset tile size |
| Projection | Orthogonal or isometric per scene, cell stride, fit-cell-to-art |
| Export | Scene `.h`/`.cpp`, animations, `setBackgroundCustomPaletteSlot()`, isometric `ISO_PROJECTION` + foot table |

## Documentation Structure

The documentation is organized in **5 guide levels**:

| Guide | Level | Description |
|-------|-------|-------------|
| [Quick Start](/tools/tilemap-editor/quick-start) | ⭐ Beginner | 5-minute quick start |
| [Usage Guide](/tools/tilemap-editor/usage-guide) | ⭐/⭐⭐ Basic/Intermediate | Essential features |
| [Advanced Guide](/tools/tilemap-editor/advanced-guide) | ⭐⭐/⭐⭐⭐ Advanced | Multi-palette, animations, attributes |
| [Isometric Guide](/tools/tilemap-editor/isometric-guide) | ⭐⭐⭐ Advanced | Projection, cell stride, fit to art, isometric export |
| [Technical Reference](/tools/tilemap-editor/technical-reference) | ⭐⭐⭐ Advanced | API, limits, data formats |

## Data Formats

### Project (`.pr32scene.bin`)

- **Single binary format** (v9) — compact and fast to load
- The **"Use Binary Format"** preference only changes the saved extension (`.pr32scene.bin` vs `.pr32scene`); the content is always binary
- No human-readable JSON project writer

### Exported C++

- Scene pair: `scene_name.h` / `scene_name.cpp`
- Shared palette header: `{namespace}_tilemap_palette.h`
- Tile animations are embedded in the scene header, not emitted as separate files
- Multi-palette uses per-layer slot setup

## Getting started

1. **New project** — tile size, map dimensions, target resolution
2. **Import tilesets** — use the TILESET panel
3. **Add layers** — background, collision, detail, etc.
4. **Paint** on the canvas
5. **Export to C++** — link generated files to engine

For detailed walkthrough, see the [Quick Start Guide](/tools/tilemap-editor/quick-start).

## Known limitations & Roadmap

- **i18n**: The `language` setting in `~/.pixelroot32/settings.json` is reserved but not yet active. The UI is currently English-only.
- **Theme toggle**: Dark and Classic themes are available but switching requires editing `settings.json` directly (`"theme": "classic"` or `"dark"`). A UI toggle is planned.
- **Dock layout reset**: When a new layout schema version ships, the first launch after upgrade resets your panel arrangement silently. Make a note of repositioning in future release notes.
- **Auto-update**: The app checks for updates at startup only when a valid license is active. Manual check via **"Check for Updates"** in the launcher footer works without a license. **Skip-this-version** is not persisted between sessions.
- **Music Editor**: Planned pattern-based tracker with multi-channel support for the PixelRoot32 APU (upcoming — not yet available in the Tool Suite).

## Next steps

- [Installation](/tools/tilemap-editor/installation)
- [Quick Start](/tools/tilemap-editor/quick-start) - First map in 5 minutes
- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features

## See also

- [Tools overview](/tools/)
- [Graphics Techniques](/guide/graphics-techniques.md)
- [Tile animation](../../architecture/tile-animation.md)
