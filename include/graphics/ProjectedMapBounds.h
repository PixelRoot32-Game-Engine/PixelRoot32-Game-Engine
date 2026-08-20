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
 * should be allowed to scroll across.
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

}  // namespace pixelroot32::graphics

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION
