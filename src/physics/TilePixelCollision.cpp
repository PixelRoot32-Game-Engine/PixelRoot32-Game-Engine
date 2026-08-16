/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Per-pixel tile collision helper implementation.
 *
 * The bitmap decode here mirrors Renderer::drawSpriteInternal
 * (src/graphics/Renderer.cpp:640-642) so the helper and the renderer agree on
 * what "opaque" means: palette index 0 == transparent, anything else == solid,
 * with 2 pixels packed LSB-first into each byte.
 */
#include "physics/TilePixelCollision.h"
#include "graphics/Renderer.h"

namespace pixelroot32::physics {

namespace {

/**
 * @brief Decode one 4bpp pixel and report whether it is opaque.
 *
 * @param data           Pointer to the tile's packed 4bpp bytes.
 * @param rowStrideBytes Byte stride of one pixel row (computed once by the
 *                       caller — see isTilePixelSolid).
 * @param px             X coordinate (already bounds-checked by the caller).
 * @param py             Y coordinate (already bounds-checked by the caller).
 */
bool pixelOpaque(const uint8_t* data, int rowStrideBytes, int px, int py) noexcept {
    const uint8_t byte = data[py * rowStrideBytes + (px >> 1)];
    const uint8_t nibble = (px & 1) ? ((byte >> 4) & 0x0F) : (byte & 0x0F);
    return nibble != 0;
}

} // namespace

bool isTilePixelSolid(const graphics::Sprite4bpp* tile, int px, int py, int erodePx) noexcept {
    if (tile == nullptr || tile->data == nullptr) {
        return false;
    }
    if (px < 0 || px >= tile->width || py < 0 || py >= tile->height) {
        return false;
    }
    if (erodePx < 0) {
        erodePx = 0;
    }

    // The /8 is a real integer division (tile->width is a runtime field), so
    // hoist it out of the erosion loop: with erodePx = k it would otherwise be
    // recomputed (2k+1)^2 times per call.
    const int rowStrideBytes = (tile->width * 4 + 7) / 8;

    if (erodePx == 0) {
        return pixelOpaque(tile->data, rowStrideBytes, px, py);
    }

    // Morphological erosion: a pixel survives only if every pixel in the
    // (2*erodePx+1)^2 square centred on it is opaque. A transparent neighbour,
    // or a neighbour beyond the tile edge (treated as transparent), voids the
    // centre pixel. This is the inverse of shrinking the moving body: the
    // object's silhouette shrinks by erodePx, so thin branches, 1px
    // protrusions and stray opaque pixels no longer block passage.
    const int minX = px - erodePx;
    const int maxX = px + erodePx;
    const int minY = py - erodePx;
    const int maxY = py + erodePx;
    for (int ny = minY; ny <= maxY; ++ny) {
        for (int nx = minX; nx <= maxX; ++nx) {
            if (nx < 0 || nx >= tile->width || ny < 0 || ny >= tile->height) {
                return false;
            }
            if (!pixelOpaque(tile->data, rowStrideBytes, nx, ny)) {
                return false;
            }
        }
    }
    return true;
}

bool isWorldPixelSolid(
    const graphics::TileMap4bpp* tilemap,
    const TileBehaviorLayer& layer,
    int tileX, int tileY,
    int px, int py,
    uint8_t requiredFlags,
    int erodePx) noexcept {
    if (tilemap == nullptr) {
        return false;
    }

    // Flag short-circuit. getTileFlags is bounds-checked against the layer and
    // returns TILE_NONE (0) for out-of-bounds tile coords, so OOB layer coords
    // short-circuit here too -- before any bitmap read.
    const uint8_t flags = getTileFlags(layer, tileX, tileY);
    if ((flags & requiredFlags) == 0) {
        return false;
    }

    // Tilemap cell bounds (indices array is tilemap->width * tilemap->height).
    if (tileX < 0 || tileX >= tilemap->width || tileY < 0 || tileY >= tilemap->height) {
        return false;
    }

    const uint8_t idx = tilemap->indices[tileY * tilemap->width + tileX];
    // Index 0 is the empty-tile sentinel and any index at/above tileCount is
    // invalid -- both match Renderer::drawTileMap's skip condition.
    if (idx == 0 || idx >= tilemap->tileCount) {
        return false;
    }

    return isTilePixelSolid(&tilemap->tiles[idx], px, py, erodePx);
}

} // namespace pixelroot32::physics
