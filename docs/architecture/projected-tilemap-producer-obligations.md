# Projected Tilemap: Producer Obligations

> **Engine-facing only.** This documents what code that *builds* a `TileMapGeneric<T>` for `Renderer::drawTileMap`'s projected overloads must emit for the geometry, culling and colour path in `src/graphics/Renderer.cpp` to render correctly.
>
> **This is still not the editor's export *format*, but it now has a reference implementation.** `examples/iso_dungeon` has since been reshaped into the three-file shape the Tilemap Editor emits, so the obligations below can be read against concrete exported data rather than against a hypothetical one — see [The reference export](#the-reference-export) and the example's own README. The generator itself is unchanged and still cannot emit an isometric map; turning that reference into generator work is phase-2.
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

## The reference export

`examples/iso_dungeon/src/assets/IsoDungeonRoomTileMap.{h,cpp}` and
`IsoDungeonRoomTileMapPalette.h` are a hand-written export in the shape the
generator emits for an orthogonal map
(`examples/metroidvania/src/assets/MetroidvaniaSceneOneTileMap.*`). The delta
between the two IS the specification for isometric export support:

| Needed | Orthogonal export today | Why the orthogonal form cannot serve |
|---|---|---|
| Rectangular cell stride | one `TILE_SIZE` written into both `tileWidth` and `tileHeight` (`cpp_code_generator.cpp:1035-1036`) | a 32×16 cell whose `WALL` bitmap is 40 px tall is inexpressible |
| `TILESET_FOOT_Y` | absent | obligation 3 above has nothing to read; every tile anchors top-left |
| `ISO_PROJECTION` | absent | obligation 6 has no spec to `static_assert` against |
| Geometry outside the 4bpp guard | whole file is guarded | gameplay places actors at cells where it draws no tile; a 4bpp-off build must still compile |
| `inline constexpr` dimensions | `static const` | these cross translation units and initialise the consumer's own constants, which an internal-linkage constant cannot legally do |

Two further properties the reference export demonstrates, both of which the
orthogonal generator already satisfies and must keep satisfying:

- **Resolved values, never rules.** The reference room's floor checkerboard was
  a runtime `(x + y) & 1` before the reshape. An exporter cannot emit a rule; it
  emits the resolved index array. Anything a producer computes per-cell at draw
  time is, by definition, not exportable.
- **A zero-extent slot 0, not an all-zero tile.** The orthogonal export emits an
  all-zero tile of full size at index 0. That is safe there but wrong as an
  isometric convention, for the reason obligation 2 gives: a full-size empty
  tile would still render correctly if the index-0 skip regressed.

## Free-standing props are not tiles, and layers do not change that

A producer converting an isometric scene will reach for a second tile layer to hold props -- the way an orthogonal map layers background, platforms and stairs. It does not work, and the reason is worth stating because the failure is intermittent rather than obvious.

**Every tile layer is drawn before every entity.** `examples/metroidvania` draws its three layers and only then calls `Scene::draw`. That is sufficient for an orthogonal game, where a mover is in front of a whole plane or behind a whole plane.

Under a projection, depth is per **cell**, not per layer. In `examples/iso_dungeon`'s room 1, screen depth is `8 * (x + y)`, and the shipped pillars at `(2, 2)` and `(4, 4)` have depths 32 and 64 with the hero at `(3, 3)` sitting at 48 -- strictly between them. The hero must paint after one pillar and before the other, and both would be in the same layer. A `drawTileMap` call is atomic, so no layer arrangement expresses it.

**Obligation: art a mover can pass behind must be drawn as a depth-sorted entity, not as a tile.** The test is not size. `iso_dungeon`'s 40 px `WALL` is legitimately a tile because both walls hug the back edges and nothing reachable is ever behind them; its 30 px altar is not, because the hero walks past both sides of it.

This constrains *rendering* only. Whether such props should nonetheless be authored and exported as map data is a separate question with a different answer, and one this page does not settle.

## Open: the behaviour layer is not covered

The editor also emits per-tile behaviour flags — `TILE_BEHAVIOR_LAYER_<NAME>[]`,
`behavior_layers[]` and `NUM_BEHAVIOR_LAYERS` (`cpp_code_generator.cpp:409-461`)
— against `physics::TileBehaviorLayer`. The reference export does **not** use
them: `examples/iso_dungeon` still derives walkability from its layout chars
through `RoomCatalog.h`'s `isSolidTile()`.

That is a known gap with a concrete engine-side cause, not an oversight.
`isSolidTile()` is `constexpr`, and roughly twenty `static_assert`s in
`RoomCatalog.h` use it to prove door and spawn invariants at compile time —
that no door is walkable straight through, that the spawn tile is not solid, and
so on. `physics::getTileFlags()` (`include/physics/TileAttributes.h`) is
`inline`, not `constexpr`, so a behaviour layer cannot answer those questions at
compile time. Converting today would trade every one of those proofs for a
runtime lookup.

Making `getTileFlags()` `constexpr` looks mechanical — it is a bounds check and
one array index — but it is an engine change with its own review, and it is
listed here rather than done as part of an example's reshape.

