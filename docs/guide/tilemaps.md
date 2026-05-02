# Tilemaps

Tilemaps are efficient for backgrounds and level geometry: the engine stores compact tile indices, applies palettes, and can skip off-screen regions.

## Features

- Generic `TileMap` templates for different BPP modes (see [TileMap API](../api/graphics.md#tilemaps)).
- Viewport culling so only visible tiles hit the draw surface.
- Optional **tile animations** (water, lava, etc.) with O(1) frame lookup when enabled — see [Tile animation architecture](../architecture/tile-animation.md).
- Optional static layer cache on ESP32 for heavy 4bpp multi-layer scenes (described in the graphics API).

## Compile-time flags

`PIXELROOT32_ENABLE_TILE_ANIMATIONS`, `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE`, and related switches are documented in [Configuration flags](../api/config.md) and the [API overview](../api/index.md).

## Samples in the engine repo

- `examples/animated_tilemap` — animated tiles
- `examples/metroidvania` — larger scrollable maps
- `examples/snake` — minimal grid usage

## Related

- [Rendering](./rendering.md)
- [Physics tile collision](../api/physics.md#collision-system-the-flat-solver)
