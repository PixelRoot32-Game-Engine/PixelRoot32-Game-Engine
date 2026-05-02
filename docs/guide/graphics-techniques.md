# Graphics Techniques - PixelRoot32

> **Note:** This document combines `tilemaps.md` and `multi-palette.md` for unified reference.

---

## Tilemaps

Tilemaps are efficient for backgrounds and level geometry: the engine stores compact tile indices, applies palettes, and can skip off-screen regions.

### Features

- Generic `TileMap` templates for different BPP modes (see [TileMap API](../api/graphics.md#tilemaps)).
- Viewport culling so only visible tiles hit the draw surface.
- Optional **tile animations** (water, lava, etc.) with O(1) frame lookup when enabled — see [Tile animation architecture](../architecture/tile-animation.md).
- Optional static layer cache on ESP32 for heavy 4bpp multi-layer scenes (described in the graphics API).

### Compile-time Flags

`PIXELROOT32_ENABLE_TILE_ANIMATIONS`, `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE`, and related switches are documented in [Configuration flags](../api/config.md) and the [API overview](../api/index.md).

### Examples in Engine Repo

- `examples/animated_tilemap` — animated tiles
- `examples/metroidvania` — larger scrollable maps
- `examples/snake` — minimal grid usage

---

## Multi-Palette and Indexed Color

Sprites and tile layers use **indexed** pixel data plus **palette** tables. You can switch palettes per draw call or layer to get more visual variety without wider BPP.

### Color Modes

| Mode | Footprint | Colors | Enable Flag |
|------|-----------|--------|-------------|
| **1bpp** | 1 bit/pixel | 2 colors | (always on) |
| **2bpp** | 2 bits/pixel | 4 colors | `PIXELROOT32_ENABLE_2BPP_SPRITES` |
| **4bpp** | 4 bits/pixel | 16 colors | `PIXELROOT32_ENABLE_4BPP_SPRITES` |

Built-in palette presets (PR32, NES, Game Boy, PICO-8, etc.) are described in the [Color / graphics API](../api/graphics.md#color-and-palettes).

### Workflow

1. Store art as indices (and optional per-tile attributes).
2. Assign a `PaletteType` or custom colors when drawing sprites or tile layers.
3. Use multi-palette tile attributes where supported (see tile attribute section in [TileMap API](../api/graphics.md#tilemaps)).

### Example Project

The `examples/dual_palette` sample in the engine repository demonstrates switching palettes in a real scene.

---

## Related Documentation

| Document | Topic |
|----------|-------|
| [Rendering](./rendering.md) | Core rendering pipeline |
| [Graphics Guidelines](./graphics-guidelines.md) | Best practices |
| [Tile Animation](../architecture/tile-animation.md) | Animation system deep dive |
| [Physics tile collision](../api/physics.md#collision-system-the-flat-solver) | Tilemap collision |