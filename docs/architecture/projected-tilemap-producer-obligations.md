# Projected Tilemap: Producer Obligations


> **Where these projects live.** `iso_dungeon` and `metroidvania` were moved out
> of this repository's `examples/` into
> [PixelRoot32-Demo-Projects](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main) — `graphics/iso_dungeon` and
> `gameplay/metroidvania` respectively. The measurements below were taken on
> that code and remain valid; only its address changed. Paths are written
> relative to that repository from here on.

> **Engine-facing only.** This documents what code that *builds* a `TileMapGeneric<T>` for `Renderer::drawTileMap`'s projected overloads must emit for the geometry, culling and colour path in `src/graphics/Renderer.cpp` to render correctly.
>
> **This is not the editor's export *format*, but the editor's export satisfies it.** `graphics/iso_dungeon`'s room map is now produced by the Tilemap Editor from a versioned project, so the obligations below can be read against concrete exported data — see [The reference export](#the-reference-export) and the example's own README. The isometric export emits a 32×16 cell stride in `TILE_WIDTH`/`TILE_HEIGHT`, an `ISO_PROJECTION` with its `static_assert`s, a `TILESET_FOOT_Y` table wired into `tileFootY`, a zero-extent slot 0, the depth-guard split, precomputed span tables, `inline constexpr` dimensions and one map per room. The format itself is documented in the [Tilemap Editor Technical Reference](/tools/tilemap-editor/technical-reference#isometric-projection).
>
> Source: `graphics/iso_dungeon`'s conversion of `RoomRenderer::drawTiles` from hand-rolled `drawSprite` calls to one projected `drawTileMap` call — the first place in this repo the projected path actually *executes*, not merely links. See [Memory System — Cell-to-screen projection](./memory-system.md) for the measured cost.

## Obligations

1. **Tile ids are 1-based.** `drawTileMap` skips `index == 0` in every format overload and in `drawTileMapProjectedImpl`. A 0-based export silently loses every cell of its first tile type.
2. **Slot 0 must exist and be readable, even though it is never drawn.** The projected path's cull-window padding scans `tiles[0 .. tileCount)` for the tileset's worst-case sprite extent before the draw loop runs, so it *reads* `tiles[0]` unconditionally even though the draw loop never blits it. A zero-extent sentinel (e.g. `{nullptr, nullptr, 0, 0, 0}` for `Sprite4bpp`) is correct there; a null or absent slot 0 is not. Do not duplicate a real tile into slot 0 to "fill the gap" — that hides an index-0-skip regression instead of exposing it.
3. **`tileFootY` is parallel to `tiles[]`, `tileCount` entries, and `nullptr` means top-left anchoring** (see `TileMapGeneric::footYFor()`, `include/graphics/Renderer.h`). It is per-tile, not per-map, because one tileset can legitimately mix heights over one cell footprint: `graphics/iso_dungeon`'s room layer uses `8` for floor tiles and `32` for walls and doors in the same 7-slot tileset.
4. **The projection is a draw parameter, never map data.** The projected `drawTileMap` overloads take `const math::ProjectionSpec&` as a call argument, not a `TileMapGeneric` field. A per-map or per-layer copy of the basis is a per-map chance for two layers to disagree — which renders two different scenes, not one consistent one.
5. **`tileWidth`/`tileHeight` are the cell stride, not the art extent.** In a 2:1 isometric export they are `32`×`16`, the reference export's `TILE_WIDTH`/`TILE_HEIGHT`, and the same values go into a `gameplay::RoomLayer`. They must not grow to fit tall art: `graphics/iso_dungeon`'s 40 px `WALL` still sits on a 16 px cell height, and its height is carried by the tile's own `Sprite4bpp::height` and `tileFootY`. They still feed `computeTilemapDirtyTracking`'s cell-grid bookkeeping; the projected draw itself places cells through the `ProjectionSpec`, not through these fields.
6. **Row-major cell iteration must be a valid painter's-algorithm order for the chosen basis.** `math::rowMajorIsPainterOrder(spec)` (`include/math/Projection.h`) is the check; enforcement is caller-side, via a `static_assert` at the spec's declaration site, the same shape `projectionSpecIsValid` uses. It is **sufficient, not necessary**: `false` means the order is unproven, not wrong. It is `false` for both the Orthogonal and Oblique rows of `ProjectionSpec`'s own doc table, and both render correctly because their art fills its cell exactly.

## The palette-bank obligation

A tile layer drawn via `drawSprite` resolves colour through `getSpritePaletteSlot`; the same layer drawn via `drawTileMap` resolves through `getBackgroundPaletteSlot` — `src/graphics/Color.cpp` keeps `spritePaletteSlots[]` and `backgroundPaletteSlots[]` as separate static arrays. **Converting a layer from sprite-per-cell to `drawTileMap` therefore changes which palette bank it reads, even though nothing about the tile art changed.**

`graphics/iso_dungeon`'s conversion found no visible colour change only because, at the time, its scene installed the same palette pointer into both banks via `setDualCustomPalette(PAL, PAL)`. (It now installs the exported tilemap palette into the background bank and its own sprite palette into the sprite bank — two palettes chosen per bank on purpose.) A game that installs *different* palettes into the sprite bank and the background bank — a legitimate, supported configuration — will see its converted layer's colours change on conversion, silently.

**Obligation: before converting a sprite-per-cell layer to `drawTileMap`, confirm its sprite-bank and background-bank palettes already agree — otherwise the conversion is also a visual change, not a pure refactor.**

## The reference export

`graphics/iso_dungeon/src/assets/tilemap/iso_dungeon_rooms.{h,cpp}` is the
Tilemap Editor's isometric export of `editor/iso_dungeon.pr32scene.bin`. It
began as a hand-written export in the shape the generator emits for an
orthogonal map (`gameplay/metroidvania/src/assets/MetroidvaniaSceneOneTileMap.*`);
the delta between the two was the specification for isometric export support,
and every row below is now emitted for isometric scenes only:

| Needed | Orthogonal export | Why the orthogonal form cannot serve |
|---|---|---|
| Rectangular cell stride | one tile size written into both `tileWidth` and `tileHeight` | a 32×16 cell whose `WALL` bitmap is 40 px tall is inexpressible |
| `TILESET_FOOT_Y` | absent | obligation 3 above has nothing to read; every tile anchors top-left |
| `ISO_PROJECTION` | absent | obligation 6 has no spec to `static_assert` against |
| Geometry outside the 4bpp guard | whole file is guarded | gameplay places actors at cells where it draws no tile; a 4bpp-off build must still compile |
| `inline constexpr` dimensions | `static const` | these cross translation units and initialise the consumer's own constants, which an internal-linkage constant cannot legally do |
| Zero-extent slot 0 | full-size all-zero tile | see below and obligation 2 |
| One map per room, `ROOM_LAYERS[]` | rooms are rects over one map | a room drawn alone needs its own map with local cells; `roomProjection` places it |

Two further properties the reference export demonstrates:

- **Resolved values, never rules.** The reference room's floor checkerboard was
  a runtime `(x + y) & 1` before the reshape. An exporter cannot emit a rule; it
  emits the resolved index array. Anything a producer computes per-cell at draw
  time is, by definition, not exportable.
- **A zero-extent slot 0, not an all-zero tile.** The orthogonal export still
  emits an all-zero tile of full size at index 0. That is safe there but wrong
  as an isometric convention, for the reason obligation 2 gives: a full-size
  empty tile would still render correctly if the index-0 skip regressed. The
  isometric export emits `{ nullptr, PALETTE, 0, 0, … }` with a foot of 0.

## Free-standing props are not tiles, and layers do not change that

A producer converting an isometric scene will reach for a second tile layer to hold props -- the way an orthogonal map layers background, platforms and stairs. It does not work, and the reason is worth stating because the failure is intermittent rather than obvious.

**Every tile layer is drawn before every entity.** `gameplay/metroidvania` draws its three layers and only then calls `Scene::draw`. That is sufficient for an orthogonal game, where a mover is in front of a whole plane or behind a whole plane.

Under a projection, depth is per **cell**, not per layer. In `graphics/iso_dungeon`'s room 1, screen depth is `8 * (x + y)`, and the shipped pillars at `(2, 2)` and `(4, 4)` have depths 32 and 64 with the hero at `(3, 3)` sitting at 48 -- strictly between them. The hero must paint after one pillar and before the other, and both would be in the same layer. A `drawTileMap` call is atomic, so no layer arrangement expresses it.

**Obligation: art a mover can pass behind must be drawn as a depth-sorted entity, not as a tile.** The test is not size. `iso_dungeon`'s 40 px `WALL` is legitimately a tile because both walls hug the back edges and nothing reachable is ever behind them; its 30 px altar is not, because the hero walks past both sides of it.

This constrains *rendering* only. Whether such props should nonetheless be authored and exported as map data is a separate question with a different answer, and one this page does not settle.

## Camera bounds: consuming the box, not just producing it

`graphics::expandProjectedMapBounds` and `graphics::cameraRangeFor` (`include/graphics/ProjectedMapBounds.h`) are the consumer-side counterpart to the obligations above: a producer emits geometry that satisfies them, and a game turns that geometry into scroll bounds through these two functions. Getting the flow or the two return types wrong is a silent bug, not a compile error, so it earns its own section.

**The end-to-end flow.** Seed one `ScreenBounds{}` (its default `valid = false` is deliberate — see the struct's own doc), call `expandProjectedMapBounds` once per layer at scene init to union every layer's extent into it, then convert once with `cameraRangeFor(world, viewWidth, viewHeight)`. Feed the result straight into `Camera2D::setBounds` / `setVerticalBounds`, call `setPosition` (or drive the camera with `followTarget`) and then `apply(renderer)` (`src/graphics/Camera2D.cpp:96-101`). `apply()` writes the camera's negated position into `Renderer::setDisplayOffset` (`include/graphics/Renderer.h:1045`), so the subsequent `drawTileMap(map, 0, 0, layerType, projection)` call (`include/graphics/Renderer.h:1270-1273`) must pass a ZERO origin — the offset is already live in the renderer. Passing a camera-negated origin here too would double-count the camera position on top of what `apply()` already pushed.

**`ScreenBounds` and `CameraBounds` are not interchangeable, on purpose.** `ScreenBounds` is half-open — `right`/`bottom` are one PAST the last covered pixel, matching a tile blit's own `[drawX, drawX + width)` convention. `CameraBounds`, returned by `cameraRangeFor`, is closed — every one of `minX`/`maxX`/`minY`/`maxY` is a camera position the camera may actually occupy. Feeding a `ScreenBounds`-shaped value where a `CameraBounds` is expected (or vice versa) is an off-by-one that a short play session will not surface: the boundary pixel only differs from the one-past-it pixel once the camera actually scrolls to the map's edge.

**The centre-collapse is required behaviour, not a nicety.** `Camera2D::setPosition` (`src/graphics/Camera2D.cpp:35-42`) applies the min clamp and then the max clamp, unconditionally, in that order. If a caller ever handed it an inverted range (`minX > maxX` — the raw, un-collapsed result whenever the world is narrower than the viewport on that axis), the max clamp fires last and wins every time, jamming the camera against `maxX` with all the slack piled onto one side instead of the world sitting centred in the viewport. `cameraRangeFor` exists specifically so no caller can construct that failure: when an axis would invert, it collapses `minX`/`maxX` (or `minY`/`maxY`) to a single centred value before `Camera2D` ever sees the range. Each axis collapses independently — a room narrower than the viewport only on Y does not affect the X range.

**The rounding contract is deliberate.** The collapse midpoint is plain C++ integer division, `(lo + hi) / 2`, which TRUNCATES TOWARD ZERO. `math::detail::projectionFloorDiv` (`include/math/Projection.h:130-133`), used elsewhere in this same projection subsystem, floors instead — but that is a correctness requirement for cell picking, not a style default to match. `cameraRangeFor`'s truncation matches the reference `IsoCamera::clampOrCentre` pixel-for-pixel, and negative `left`/`top` coordinates are the NORMAL case here, not an edge case: `axisYx` is negative under an isometric basis (see `ProjectionSpec`'s own doc table), so a world's `left`/`top` routinely land below zero. Do not "fix" this to floor division to match `projectionFloorDiv` — that would silently shift the camera by a pixel and break parity with the reference implementation this capability was built to match.

## Rooms under a projection

Closed: `gameplay/RoomLayout.h` used to build only orthogonal camera rects (`cells * tile size`), so `graphics/iso_dungeon` derived its room layer by hand and passed `camera = nullptr` to `enterRoom`. With `PIXELROOT32_ENABLE_GAMEPLAY_ROOM` and `PIXELROOT32_ENABLE_TILEMAP_PROJECTION` both on, two additions cover it:

- **`buildRoomGraph(layer, graph, proj, perRoom = nullptr)`** makes each room's camera rect the screen box of its projected cell block: the four corners of `[originCol, originCol + cols] × [originRow, originRow + rows]` through `cellToScreenX/Y`. `perRoom`, an optional array of one `graphics::ScreenBounds` per room (typically `expandProjectedMapBounds` over that room's map), widens the box by the art's overhang, which the cells alone cannot know. Validation keeps the orthogonal shape: any bad room rejects the whole layer, and a degenerate projection (`projectionDet < 1`) is one more reason to.
- **`roomProjection(proj, room)`** returns `proj` with its origin moved to the screen position of cell `(originCol, originRow)`. A map holding only that room, with local cells from `(0, 0)`, drawn through it lands where those cells sit in the scene.

`RoomData` and `RoomLayer` did not change shape: rooms stay in scene cells and `tileWidth`/`tileHeight` stay the cell stride (obligation 5). Negative coordinates work as they are: an isometric basis puts rooms on the `+Y` side at negative screen X (`{120, 88, 16, 8, -16, 8}` puts a 7×7 room at cell `(0, 7)` at `x = -104`), `Room` stores its rect as signed `Scalar`, and `Camera2D` clamps to it as-is. `kMaxRoomWorldCoord` applies to the projected, widened box in both directions, `[-32767, 32767]` per axis, because a `Fixed16` `Scalar` wraps past either end. The orthogonal overload is unchanged; `camera = nullptr` remains valid for a game whose rooms are one screen each.

## Open: the behaviour layer is not covered

The editor also emits per-tile behaviour flags — `TILE_BEHAVIOR_LAYER_<NAME>[]`,
`behavior_layers[]` and `NUM_BEHAVIOR_LAYERS` — against
`physics::TileBehaviorLayer`, in scene cells even when the scene has room maps.
The reference export carries an empty one, and the game does **not** use it: `graphics/iso_dungeon` still derives walkability from its layout chars
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

