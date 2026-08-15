/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Per-pixel tile collision helper.
 *
 * Answers "is this pixel inside this 4bpp tile opaque (solid)?" by decoding the
 * same LSB-first nibble packing the Renderer uses (see Renderer.cpp:640-642 in
 * drawSpriteInternal). Combines that bitmap check with the existing
 * TileBehaviorLayer flag lookup via a convenience wrapper, so game logic can
 * refine whole-tile collision to per-pixel granularity without baking solidity
 * into the tilemap exporter.
 *
 * Both functions are read-only, allocation-free, and O(1) per call. They never
 * throw (matching the engine's -fno-exceptions contract) and are safe to call
 * from hot paths.
 */
#pragma once

#include <cstdint>
#include "physics/TileAttributes.h"

namespace pixelroot32::graphics {
    struct Sprite4bpp;
    template<typename T> struct TileMapGeneric;
    using TileMap4bpp = TileMapGeneric<Sprite4bpp>;
}

namespace pixelroot32::physics {

/**
 * @brief Check if a pixel inside a 4bpp tile is opaque (solid).
 *
 * Decodes the 4bpp bitmap using the same LSB-first, 2-pixels-per-byte packing
 * the Renderer uses: the low nibble of each byte holds the even column (left)
 * and the high nibble holds the odd column (right). A pixel is solid when its
 * palette index is non-zero; palette index 0 means transparent (walkable).
 *
 * @param tile    Pointer to Sprite4bpp (nullptr -> returns false).
 * @param px      X coordinate within the tile, in [0, tile->width).
 * @param py      Y coordinate within the tile, in [0, tile->height).
 * @param erodePx Morphological erosion radius (default 0 = off). When > 0, the
 *                pixel is solid only if the whole (2*erodePx+1)^2 square centred
 *                on it is opaque. A single transparent neighbour — or a
 *                neighbour beyond the tile edge — voids the pixel. This shrinks
 *                the *object's* collision silhouette instead of the moving body,
 *                so 1px branches, dangling protrusions and stray opaque pixels
 *                stop blocking passage while the object's core stays solid.
 * @return true if the (eroded) pixel is opaque; false if transparent,
 *         out-of-bounds, or tile/tile->data is null.
 *
 * @note O(1) cost with erodePx = 0: 1 bounds check, 1 multiply (row stride),
 *       1 shift+mask, 1 compare. With erodePx = k the cost is (2k+1)^2 lookups.
 *       No allocation, no mutable state, thread-safe (pure function).
 * @note Erosion is per-tile: it does not read neighbouring tiles, so a solid
 *       wall made of several tiles erodes 1px at each tile seam. At 16px tile
 *       size this gap is far smaller than any body, so it is harmless.
 *
 * @see Renderer::drawSpriteInternal (src/graphics/Renderer.cpp:640-642) for the
 *      authoritative LSB-first nibble packing this helper mirrors. If the
 *      renderer's bit layout ever changes, this function must change with it.
 */
bool isTilePixelSolid(const graphics::Sprite4bpp* tile, int px, int py, int erodePx = 0) noexcept;

/**
 * @brief Check if a world-pixel is solid: behavior flag set AND bitmap pixel opaque.
 *
 * Combines the TileBehaviorLayer flag lookup with the per-pixel bitmap check.
 * Returns true only when all of these hold:
 *   1. (tileX, tileY) is in-bounds in the behavior layer and its flags satisfy
 *      (flags & requiredFlags) != 0,
 *   2. (tileX, tileY) is in-bounds in the tilemap,
 *   3. the tile index at that cell is not the empty-tile sentinel (0) and not
 *      beyond tileCount,
 *   4. the referenced 4bpp bitmap has an opaque pixel at the local (px, py).
 *
 * @param tilemap       Pointer to TileMap4bpp (nullptr -> returns false).
 * @param layer         Behavior layer for flag lookup.
 * @param tileX         Tile X coordinate in the layer/tilemap.
 * @param tileY         Tile Y coordinate in the layer/tilemap.
 * @param px            X coordinate within the tile, in [0, tile->width).
 * @param py            Y coordinate within the tile, in [0, tile->height).
 * @param requiredFlags TileFlags bitmask that must be set (default: TILE_SOLID).
 * @param erodePx       Erosion radius forwarded to isTilePixelSolid (default 0).
 * @return true iff the behavior flag is set AND the (eroded) bitmap pixel is opaque.
 *
 * @note Short-circuits on TILE_NONE (no behavior) before any bitmap read.
 * @note O(1) cost: flag lookup + tile index + isTilePixelSolid.
 *
 * @see isTilePixelSolid for the underlying per-pixel check and the erodePx
 *      semantics.
 * @see Renderer::drawSpriteInternal (src/graphics/Renderer.cpp:640-642) for
 *      the LSB-first nibble packing this wrapper relies on.
 */
bool isWorldPixelSolid(
    const graphics::TileMap4bpp* tilemap,
    const TileBehaviorLayer& layer,
    int tileX, int tileY,
    int px, int py,
    uint8_t requiredFlags = TILE_SOLID,
    int erodePx = 0) noexcept;

} // namespace pixelroot32::physics
