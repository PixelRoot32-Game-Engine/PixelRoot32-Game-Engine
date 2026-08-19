#pragma once

#include <cstdint>

#include "graphics/Renderer.h"

#include "RoomCatalog.h"
#include "assets/DungeonTiles.h"

/**
 * @file RoomTileMap.h
 * @brief The tile layer's 7-slot tileset, foot table and the runtime index
 *        bake that turns a RoomSpec's layout into a TileMap4bpp.
 *
 * Six real tiles, not eight: `RoomRenderer::drawTiles` switches only on `'W'`,
 * `'D'`, `'E'` and `default` (floor). `'A'` (altar) and `'P'` (pillar) fall
 * into `default`, where they resolve to accent/plain floor exactly like their
 * neighbours -- ALTAR_SPRITE and PILLAR_SPRITE are PropEntity sprites drawn
 * on layer 1 and never appear on this tile layer.
 */

namespace iso_dungeon {

namespace gfx = pixelroot32::graphics;

/// Ids for the STATIC TILE LAYER only -- see the file comment for why altar
/// and pillar are absent. 1-based: index 0 is the reserved empty sentinel
/// `drawTileMap` always skips, so no layout cell may ever bake to it.
enum TileId : uint8_t {
    TILE_EMPTY = 0,
    TILE_FLOOR_A,
    TILE_FLOOR_B,
    TILE_FLOOR_ACCENT,
    TILE_WALL,
    TILE_DOOR_NE,
    TILE_DOOR_NW,
};

/// Sentinel + 6 real tiles.
inline constexpr uint16_t kTileLayerTileCount = 7;

/// Present so index 0 means "empty" and the projected path's cull-padding
/// scan (drawTileMapProjectedImpl, which reads tiles[0] for the tileset's
/// worst-case extent even though it never blits it) stays in bounds. Never
/// drawn. Deliberately NOT a copy of FLOOR_A_SPRITE: a duplicate would still
/// render correctly if the index-0 skip ever regressed, hiding the
/// regression instead of catching it.
static const gfx::Sprite4bpp EMPTY_TILE_SPRITE = {nullptr, nullptr, 0, 0, 0};

/// Indexed by TileId. Flash-resident (static const), matching the shipped
/// convention in assets/DungeonTiles.h.
static const gfx::Sprite4bpp kRoomTileset[kTileLayerTileCount] = {
    EMPTY_TILE_SPRITE,
    FLOOR_A_SPRITE,
    FLOOR_B_SPRITE,
    FLOOR_ACCENT_SPRITE,
    WALL_SPRITE,
    DOOR_NE_SPRITE,
    DOOR_NW_SPRITE,
};

/// Parallel to kRoomTileset, per TileMapGeneric::tileFootY's contract.
static const uint8_t kRoomTileFootY[kTileLayerTileCount] = {
    0,
    FLOOR_A_FOOT_Y,
    FLOOR_B_FOOT_Y,
    FLOOR_ACCENT_FOOT_Y,
    WALL_FOOT_Y,
    DOOR_NE_FOOT_Y,
    DOOR_NW_FOOT_Y,
};

/**
 * @brief Bakes a room's layout into a runtime uint8_t[kRoomTiles * kRoomTiles]
 *        tile-layer index grid.
 *
 * Reproduces RoomRenderer::drawTiles' pre-conversion switch verbatim,
 * including the position-dependent checkerboard rule: the same '.' char maps
 * to a different id at different cells ((x + y) & 1), which is why this bakes
 * into a concrete runtime buffer rather than living as a constexpr per-room
 * table -- TileMapGeneric::indices is uint8_t*, not const uint8_t*.
 *
 * @param room Room whose kRoomTiles x kRoomTiles layout to bake.
 * @param out  Destination buffer, kRoomTiles * kRoomTiles entries.
 */
inline void buildRoomTileIndices(const RoomSpec& room, uint8_t out[kRoomTiles * kRoomTiles]) {
    for (int y = 0; y < kRoomTiles; ++y) {
        for (int x = 0; x < kRoomTiles; ++x) {
            const char cell = room.layout[y][x];
            uint8_t id;
            switch (cell) {
                case 'W':
                    id = TILE_WALL;
                    break;
                case 'D':
                    id = TILE_DOOR_NE;
                    break;
                case 'E':
                    id = TILE_DOOR_NW;
                    break;
                default: {
                    // Floor. Position-dependent, not char-dependent: the same
                    // '.' bakes to a different id at different cells, which
                    // is why this loop bakes into a concrete runtime buffer
                    // rather than a constexpr table (see the file comment).
                    const bool accent = (cell == 'a' || cell == 'A');
                    id = accent            ? TILE_FLOOR_ACCENT
                       : ((x + y) & 1)     ? TILE_FLOOR_B
                                           : TILE_FLOOR_A;
                    break;
                }
            }
            out[y * kRoomTiles + x] = id;
        }
    }
}

}  // namespace iso_dungeon
