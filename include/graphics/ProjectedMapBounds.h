/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION

#include "graphics/Renderer.h"
#include "math/Projection.h"

/**
 * @file ProjectedMapBounds.h
 * @brief Screen-space bounding-box accumulator for projected tilemaps.
 *
 * `expandProjectedMapBounds` derives how much screen space a projected
 * `TileMapGeneric` layer actually covers -- the same placement geometry
 * `Renderer::drawTileMap`'s projected overloads use
 * (`src/graphics/Renderer.cpp:1146-1220`, `drawTileMapProjectedImpl`) --
 * and unions it into a caller-owned `ScreenBounds`. A game calls it once per
 * layer at scene init, never per frame, to obtain the world extent a camera
 * should be allowed to scroll across. `cameraRangeFor` then converts that
 * extent into a `CameraBounds` -- a camera-position range, closed rather
 * than half-open (see `CameraBounds`'s own doc for why the two interval
 * kinds must not be conflated).
 *
 * Two things this header deliberately does NOT do:
 * - It does not read `TileMapGeneric::tileWidth` / `tileHeight` for
 *   overhang. Those are the map's NOMINAL CELL size; each `tiles[]` entry
 *   carries its own `width` / `height`, and isometric art routinely
 *   overhangs its cell (a 32x16 cell with 32x32 wall art anchored by
 *   `tileFootY` is the normal case, not an edge case).
 * - It does not consult `runtimeMask` or `animManager`. Bounds cover the
 *   map's static scene geometry over the whole cell rectangle; a layer's
 *   runtime holes do not shrink the box the camera is allowed to explore.
 */

namespace pixelroot32::graphics {

/**
 * @struct ScreenBounds
 * @brief Half-open screen-space bounding box accumulated across one or more
 *        `expandProjectedMapBounds` calls.
 *
 * `right` and `bottom` are one PAST the last covered pixel -- the same
 * convention a tile blit uses (`[drawX, drawX + width)`), so
 * `right - left` and `bottom - top` are exact pixel widths/heights with no
 * off-by-one at the edge.
 *
 * `valid` distinguishes "never seeded" from the representable real box
 * `{0, 0, 0, 0}` (e.g. a single-cell map at the origin with a zero-reach
 * tileset). `false` means the next `expandProjectedMapBounds` call seeds the
 * box directly from its own extent; `true` means it widens the existing box
 * to the union. A default-constructed `ScreenBounds{}` is always the correct
 * value to seed a fresh accumulation with.
 */
struct ScreenBounds {
    int left = 0;    ///< Leftmost covered screen X, in pixels.
    int top = 0;     ///< Topmost covered screen Y, in pixels.
    int right = 0;   ///< One past the rightmost covered screen X, in pixels.
    int bottom = 0;  ///< One past the bottommost covered screen Y, in pixels.
    bool valid = false;  ///< `false` until the first successful expand call.
};

/**
 * @brief Unions a projected 1bpp tilemap layer's screen extent into `bounds`.
 *
 * Derives the extent from the four corners of the map's cell-index
 * rectangle -- cell anchors at `(0,0)`, `(width-1,0)`, `(0,height-1)` and
 * `(width-1,height-1)` -- transformed through `projection`, then widens that
 * box by the tileset's own worst-case per-tile overhang (left/right from
 * `tiles[i].width`, up/down from `tiles[i].height` and `footYFor(i)`,
 * mirroring `drawTileMapProjectedImpl`'s cull-padding scan at
 * `src/graphics/Renderer.cpp:1146-1170`). Tile index 0 is EXCLUDED from that
 * scan -- it is the exporter's empty-tile sentinel `drawTileMap` never
 * blits, so counting it would let the camera scroll into space nothing
 * occupies. This deliberately diverges from the renderer's own worst-case
 * scan, which starts at index 0 because over-padding a CULL rect is
 * harmless; do not "harmonize" the two loops.
 *
 * A no-op (leaves `bounds` byte-for-byte unmodified) when `map.indices`,
 * `map.tiles` is `nullptr`, or `map.width`, `map.height`, `map.tileWidth`,
 * `map.tileHeight`, or `map.tileCount` is `0` -- byte-identical to
 * `drawTileMapProjectedImpl`'s own degenerate-input guard
 * (`src/graphics/Renderer.cpp:1131-1136`), so the bounds box's validity
 * tracks exactly what the renderer would draw.
 *
 * Runs in `O(tileCount + 4)`, allocates no heap memory, and throws no
 * exceptions. Intended for scene init, not per-frame use.
 *
 * @param bounds Accumulator widened (or seeded, if `bounds.valid == false`)
 *               by this layer's extent.
 * @param map The projected 1bpp tilemap layer to measure.
 * @param projection The basis this layer is drawn through.
 * @pre `pixelroot32::math::projectionSpecIsValid(projection, map.width, map.height)`
 *      holds for `map`; an invalid basis is undefined for this function the
 *      same way it is for `Renderer::drawTileMap`.
 */
void expandProjectedMapBounds(ScreenBounds& bounds, const TileMap& map,
                               const pixelroot32::math::ProjectionSpec& projection);

/**
 * @brief Unions a projected 2bpp tilemap layer's screen extent into `bounds`.
 * @see expandProjectedMapBounds(ScreenBounds&, const TileMap&, const pixelroot32::math::ProjectionSpec&)
 *      for the full contract -- identical behavior, only the tile format
 *      differs.
 * @param bounds Accumulator widened (or seeded) by this layer's extent.
 * @param map The projected 2bpp tilemap layer to measure.
 * @param projection The basis this layer is drawn through.
 * @pre `pixelroot32::math::projectionSpecIsValid(projection, map.width, map.height)`
 */
void expandProjectedMapBounds(ScreenBounds& bounds, const TileMap2bpp& map,
                               const pixelroot32::math::ProjectionSpec& projection);

/**
 * @brief Unions a projected 4bpp tilemap layer's screen extent into `bounds`.
 * @see expandProjectedMapBounds(ScreenBounds&, const TileMap&, const pixelroot32::math::ProjectionSpec&)
 *      for the full contract -- identical behavior, only the tile format
 *      differs.
 * @param bounds Accumulator widened (or seeded) by this layer's extent.
 * @param map The projected 4bpp tilemap layer to measure.
 * @param projection The basis this layer is drawn through.
 * @pre `pixelroot32::math::projectionSpecIsValid(projection, map.width, map.height)`
 */
void expandProjectedMapBounds(ScreenBounds& bounds, const TileMap4bpp& map,
                               const pixelroot32::math::ProjectionSpec& projection);

/**
 * @struct CameraBounds
 * @brief Closed-interval camera-position range produced by `cameraRangeFor`.
 *
 * **Deliberately NOT `ScreenBounds`.** Every field here -- `minX`, `maxX`,
 * `minY`, `maxY` -- is a camera position the camera may actually occupy, so
 * the interval is CLOSED: `[minX, maxX]`, both ends inclusive. `ScreenBounds`
 * is HALF-OPEN (`right`/`bottom` are one past the last covered pixel)
 * because it measures a run of covered pixels, not a set of legal camera
 * positions. Reusing `ScreenBounds` as this return type -- feeding a
 * `right` that means "one past the edge" into code that reads it as "the
 * maximum legal position" -- is an off-by-one that stays invisible until the
 * camera scrolls all the way to a map edge, which is exactly the kind of bug
 * a short play session will not surface. The two structs share a field
 * count and nothing else; do not conflate them.
 *
 * `valid` mirrors `ScreenBounds::valid`: `false` when the source
 * `ScreenBounds` passed to `cameraRangeFor` was itself never seeded (see
 * `expandProjectedMapBounds`'s union-across-calls contract), in which case
 * no camera range exists and `minX`/`maxX`/`minY`/`maxY` are left at their
 * defaults rather than computed from garbage input.
 */
struct CameraBounds {
    int minX = 0;    ///< Minimum camera X position the camera may occupy.
    int maxX = 0;    ///< Maximum camera X position the camera may occupy.
    int minY = 0;    ///< Minimum camera Y position the camera may occupy.
    int maxY = 0;    ///< Maximum camera Y position the camera may occupy.
    bool valid = false;  ///< `false` when `world.valid` was `false`; see above.
};

/**
 * @brief Converts an accumulated `ScreenBounds` world extent into a
 *        `CameraBounds` camera-position range, one axis at a time.
 *
 * For each axis the naive range is `[world.left, world.right - viewWidth]`
 * (Y analogously): `world.right` is one past the last covered pixel, so
 * `world.right - viewWidth` is exactly the maximum camera position that
 * still keeps the viewport's right edge within the world -- no separate
 * `-1` needed, because the half-open convention already accounts for it.
 *
 * **Centre-collapse (CRITICAL).** When the world is SMALLER than the
 * viewport on an axis -- `world.right - viewWidth < world.left` (Y:
 * `world.bottom - viewHeight < world.top`) -- that naive range is INVERTED
 * (`lo > hi`), and this function does not return it inverted. Instead the
 * axis collapses to a single point: `minX == maxX == (world.left +
 * (world.right - viewWidth)) / 2` (Y analogously). This is not optional
 * polish; it is required because `Camera2D::setPosition`
 * (`src/graphics/Camera2D.cpp:35-42`) applies the min clamp and THEN the
 * max clamp, unconditionally, in that order. Fed an inverted range, the max
 * clamp always fires last and wins, jamming the camera against `maxX`
 * (`maxY`) with every pixel of slack piled on the opposite side instead of
 * the world being centred in the viewport -- silently, with no error and no
 * assertion, because nothing about `Camera2D` itself is broken. Each axis
 * collapses independently: a world narrower than the viewport on X only
 * collapses `minX`/`maxX` and leaves `minY`/`maxY` at the normal range.
 *
 * **Rounding contract (deliberate, pin this if you touch it).** The
 * collapse midpoint uses plain C++ integer division, `(lo + hi) / 2`, which
 * TRUNCATES TOWARD ZERO -- copied verbatim from the reference
 * `IsoCamera::clampOrCentre` this capability matches pixel-for-pixel. This
 * is a deliberate DIVERGENCE from `math::detail::projectionFloorDiv`'s
 * floor rule used elsewhere in this same subsystem: floor is a correctness
 * requirement there (cell picking), whereas here both truncation and floor
 * land inside `[hi, lo]` and the choice is only a <=1px cosmetic bias whose
 * direction flips with the sign of `lo + hi`. Negative `left`/`top` are the
 * NORMAL case under an isometric basis (`axisYx` is negative), not an edge
 * case -- do not "fix" this to floor division; that would silently shift
 * the camera by a pixel and break parity with the reference implementation.
 *
 * @param world Accumulated world extent, normally the union of every
 *              layer's `expandProjectedMapBounds` call for the scene.
 * @param viewWidth Viewport width, in pixels.
 * @param viewHeight Viewport height, in pixels.
 * @return A `CameraBounds` with `valid == world.valid`. When `world.valid`
 *         is `false` the returned range is default-constructed (all zero)
 *         and must not be applied to a camera.
 * @pre `viewWidth > 0 && viewHeight > 0`.
 */
[[nodiscard]] CameraBounds cameraRangeFor(const ScreenBounds& world, int viewWidth, int viewHeight);

}  // namespace pixelroot32::graphics

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION
