# TileMap4bppDrawSpec

<Badge type="info" text="Struct" />

**Source:** `StaticTilemapLayerCache.h`

## Description

One drawable 4bpp tilemap layer with an origin in logical coordinates.

Entries with map == nullptr are skipped. Use any number of static layers
(snapshotted together) and dynamic layers (redrawn every frame after restore).

TileMap4bppDrawSpec::projection
Optional isometric/oblique basis, only present when
PIXELROOT32_ENABLE_TILEMAP_PROJECTION is on. nullptr — the default, and
therefore what every existing three-element aggregate initialiser
({&map, 0, 0}) keeps meaning — selects the axis-aligned
Renderer::drawTileMap overload, byte for byte as before. A non-null spec
selects the projected overload instead, which places, culls and marks cells
through that basis (see math/Projection.h).

The member is deliberately last and default-initialised so adding it did not
touch a single call site, and so the struct's layout is unchanged when the
flag is off.
