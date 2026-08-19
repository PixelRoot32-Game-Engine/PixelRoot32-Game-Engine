# Projected Tilemap: Producer Obligations

> **Engine-facing only.** This documents what code that *builds* a `TileMapGeneric<T>` for `Renderer::drawTileMap`'s projected overloads must emit for the geometry, culling and colour path in `src/graphics/Renderer.cpp` to render correctly.
>
> **This is not an export-format contract for the PixelRoot32 Tilemap Editor.** The editor has no isometric concept today — `rg -ci "isometric|diamond" docs/tools/` returns zero matches. Freezing an export format now would freeze the very decisions this document exists to test. A later phase-2 unit turns these obligations into an editor export contract; do not read this page as that contract.
>
> Source: `examples/iso_dungeon`'s conversion of `RoomRenderer::drawTiles` from hand-rolled `drawSprite` calls to one projected `drawTileMap` call — the first place in this repo the projected path actually *executes*, not merely links. See [Memory System — Cell-to-screen projection](./memory-system.md) for the measured cost.

## Obligations

1. **Tile ids are 1-based.** `drawTileMap` skips `index == 0` in every format overload and in `drawTileMapProjectedImpl`. A 0-based export silently loses every cell of its first tile type.
2. **Slot 0 must exist and be readable, even though it is never drawn.** The projected path's cull-window padding scans `tiles[0 .. tileCount)` for the tileset's worst-case sprite extent before the draw loop runs, so it *reads* `tiles[0]` unconditionally even though the draw loop never blits it. A zero-extent sentinel (e.g. `{nullptr, nullptr, 0, 0, 0}` for `Sprite4bpp`) is correct there; a null or absent slot 0 is not. Do not duplicate a real tile into slot 0 to "fill the gap" — that hides an index-0-skip regression instead of exposing it.
3. **`tileFootY` is parallel to `tiles[]`, `tileCount` entries, and `nullptr` means top-left anchoring** (see `TileMapGeneric::footYFor()`, `include/graphics/Renderer.h`). It is per-tile, not per-map, because one tileset can legitimately mix heights over one cell footprint: `examples/iso_dungeon`'s room layer uses `8` for floor tiles and `32` for walls and doors in the same 7-slot tileset.
4. **The projection is a draw parameter, never map data.** The projected `drawTileMap` overloads take `const math::ProjectionSpec&` as a call argument, not a `TileMapGeneric` field. A per-map or per-layer copy of the basis is a per-map chance for two layers to disagree — which renders two different scenes, not one consistent one.
5. **`tileWidth`/`tileHeight` keep their orthogonal meaning.** They still feed `computeTilemapDirtyTracking`'s cell-grid bookkeeping. They are not the projected diamond's on-screen dimensions — do not set them to the diamond's footprint.
6. **Row-major cell iteration must be a valid painter's-algorithm order for the chosen basis.** `math::rowMajorIsPainterOrder(spec)` (`include/math/Projection.h`) is the check; enforcement is caller-side, via a `static_assert` at the spec's declaration site, the same shape `projectionSpecIsValid` uses. It is **sufficient, not necessary**: `false` means the order is unproven, not wrong. It is `false` for both the Orthogonal and Oblique rows of `ProjectionSpec`'s own doc table, and both render correctly because their art fills its cell exactly.

## The palette-bank obligation

A tile layer drawn via `drawSprite` resolves colour through `getSpritePaletteSlot`; the same layer drawn via `drawTileMap` resolves through `getBackgroundPaletteSlot` — `src/graphics/Color.cpp` keeps `spritePaletteSlots[]` and `backgroundPaletteSlots[]` as separate static arrays. **Converting a layer from sprite-per-cell to `drawTileMap` therefore changes which palette bank it reads, even though nothing about the tile art changed.**

`examples/iso_dungeon`'s conversion found no visible colour change only because `IsoDungeonScene.cpp:22` installs the same palette pointer into both banks via `setDualCustomPalette(PAL, PAL)`. A game that installs *different* palettes into the sprite bank and the background bank — a legitimate, supported configuration — will see its converted layer's colours change on conversion, silently.

**Obligation: before converting a sprite-per-cell layer to `drawTileMap`, confirm its sprite-bank and background-bank palettes already agree — otherwise the conversion is also a visual change, not a pure refactor.**
