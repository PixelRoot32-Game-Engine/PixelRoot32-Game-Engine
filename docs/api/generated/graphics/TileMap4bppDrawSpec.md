# TileMap4bppDrawSpec

<Badge type="info" text="Struct" />

**Source:** `StaticTilemapLayerCache.h`

## Description

One drawable 4bpp tilemap layer with an origin in logical coordinates.

Entries with map == nullptr are skipped. Use any number of static layers
(snapshotted together) and dynamic layers (redrawn every frame after restore).
