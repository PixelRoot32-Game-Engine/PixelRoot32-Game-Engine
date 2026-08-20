/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ProjectedMapBounds.h"

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION

namespace pixelroot32::graphics {

namespace {

/// Worst-case per-tile overhang reach, in pixels, taken across a scanned
/// tileset. All four fields are independent maxima -- a tall narrow tile and
/// a short wide tile in the same tileset each contribute only their own
/// dimension to the box, never both from one tile.
struct TilesetReach {
    uint8_t left = 0;
    uint8_t right = 0;
    uint8_t up = 0;
    uint8_t down = 0;
};

/**
 * Scans `map.tiles[1 .. map.tileCount)` -- the ONLY part of this capability
 * that touches `TileT`, kept minimal so each of the three public overloads
 * instantiates a small loop rather than duplicating the corner/union math
 * below. Index 0 is skipped on purpose: it is the exporter's empty-tile
 * sentinel `drawTileMap` never blits (see `TileMapGeneric::footYFor`'s own
 * note), and counting it would let a bounds box -- unlike a cull rect --
 * grow to cover space nothing actually occupies. The renderer's own
 * worst-case scan (`Renderer.cpp:1156`, `drawTileMapProjectedImpl`) starts
 * at index 0 for the opposite reason: over-padding a CULL rect is harmless.
 * Do not "harmonize" the two loops -- the divergence is deliberate.
 */
template <typename TileT>
TilesetReach scanTilesetReach(const TileMapGeneric<TileT>& map) {
    TilesetReach reach;
    for (uint16_t i = 1; i < map.tileCount; ++i) {
        const TileT& tile = map.tiles[i];
        const uint8_t footY = map.footYFor(i);
        const uint8_t left = static_cast<uint8_t>(tile.width / 2);
        const uint8_t right = static_cast<uint8_t>(tile.width - tile.width / 2);
        const uint8_t down =
            (tile.height > footY) ? static_cast<uint8_t>(tile.height - footY) : uint8_t{0};

        if (left > reach.left) reach.left = left;
        if (right > reach.right) reach.right = right;
        if (footY > reach.up) reach.up = footY;
        if (down > reach.down) reach.down = down;
    }
    return reach;
}

/**
 * Non-template: the four-corner transform plus seed/union logic, shared by
 * all three public overloads regardless of tile format (D2's flash-bound
 * split -- only `scanTilesetReach` above is instantiated per `TileT`).
 *
 * The extent is derived from the four corners of the CELL-INDEX rectangle:
 * the actual anchor positions of the extreme drawn cells at `(0, 0)`,
 * `(cols-1, 0)`, `(0, rows-1)` and `(cols-1, rows-1)` -- not the geometric
 * rectangle vertices `(0,0)`-`(cols,rows)`, which would overshoot by one
 * full cell step under a non-orthogonal basis. That box is then widened
 * uniformly by `reach` (the tileset's worst-case overhang around any cell
 * anchor), and unioned into `bounds` per its `valid` flag: seed directly
 * when `false`, widen to the min/max when `true`.
 */
void expandFromCellRect(ScreenBounds& bounds, int cols, int rows,
                         const pixelroot32::math::ProjectionSpec& projection,
                         TilesetReach reach) {
    const int lastCol = cols - 1;
    const int lastRow = rows - 1;

    const int cornerX[4] = {
        pixelroot32::math::cellToScreenX(0, 0, projection),
        pixelroot32::math::cellToScreenX(lastCol, 0, projection),
        pixelroot32::math::cellToScreenX(0, lastRow, projection),
        pixelroot32::math::cellToScreenX(lastCol, lastRow, projection),
    };
    const int cornerY[4] = {
        pixelroot32::math::cellToScreenY(0, 0, projection),
        pixelroot32::math::cellToScreenY(lastCol, 0, projection),
        pixelroot32::math::cellToScreenY(0, lastRow, projection),
        pixelroot32::math::cellToScreenY(lastCol, lastRow, projection),
    };

    int minX = cornerX[0];
    int maxX = cornerX[0];
    int minY = cornerY[0];
    int maxY = cornerY[0];
    for (int i = 1; i < 4; ++i) {
        if (cornerX[i] < minX) minX = cornerX[i];
        if (cornerX[i] > maxX) maxX = cornerX[i];
        if (cornerY[i] < minY) minY = cornerY[i];
        if (cornerY[i] > maxY) maxY = cornerY[i];
    }

    const int left = minX - static_cast<int>(reach.left);
    const int right = maxX + static_cast<int>(reach.right);
    const int top = minY - static_cast<int>(reach.up);
    const int bottom = maxY + static_cast<int>(reach.down);

    if (!bounds.valid) {
        bounds.left = left;
        bounds.top = top;
        bounds.right = right;
        bounds.bottom = bottom;
        bounds.valid = true;
    } else {
        if (left < bounds.left) bounds.left = left;
        if (top < bounds.top) bounds.top = top;
        if (right > bounds.right) bounds.right = right;
        if (bottom > bounds.bottom) bounds.bottom = bottom;
    }
}

/// Shared body for all three public overloads: the degenerate-input guard
/// (byte-identical to drawTileMapProjectedImpl's, Renderer.cpp:1131-1136,
/// so the bounds box's validity tracks exactly what the renderer would
/// draw), then the scan + corner/union expansion.
template <typename TileT>
void expandProjectedMapBoundsImpl(ScreenBounds& bounds, const TileMapGeneric<TileT>& map,
                                   const pixelroot32::math::ProjectionSpec& projection) {
    if (map.indices == nullptr || map.tiles == nullptr ||
        map.width == 0 || map.height == 0 ||
        map.tileWidth == 0 || map.tileHeight == 0 ||
        map.tileCount == 0) {
        return;
    }

    const TilesetReach reach = scanTilesetReach(map);
    expandFromCellRect(bounds, map.width, map.height, projection, reach);
}

}  // namespace

void expandProjectedMapBounds(ScreenBounds& bounds, const TileMap& map,
                               const pixelroot32::math::ProjectionSpec& projection) {
    expandProjectedMapBoundsImpl<Sprite>(bounds, map, projection);
}

void expandProjectedMapBounds(ScreenBounds& bounds, const TileMap2bpp& map,
                               const pixelroot32::math::ProjectionSpec& projection) {
    expandProjectedMapBoundsImpl<Sprite2bpp>(bounds, map, projection);
}

void expandProjectedMapBounds(ScreenBounds& bounds, const TileMap4bpp& map,
                               const pixelroot32::math::ProjectionSpec& projection) {
    expandProjectedMapBoundsImpl<Sprite4bpp>(bounds, map, projection);
}

}  // namespace pixelroot32::graphics

#endif  // PIXELROOT32_ENABLE_TILEMAP_PROJECTION
