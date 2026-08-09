#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM

#include "gameplay/RoomLayout.h"
#include "GameConstants.h"

/**
 * @file OverworldRooms.h
 * @brief The exported room layer: four adjacent overworld screens, wired 2x2.
 *
 * Hand-written to stand in for the Tilemap Editor's room-metadata emitter. The
 * shape is the contract, not the provenance — see
 * docs/tools/tilemap-editor/technical-reference.md#room-metadata.
 *
 *     +----------------+----------------+
 *     | 0  north-west  | 1  north-east  |
 *     +----------------+----------------+
 *     | 2  START       | 3  south-east  |
 *     +----------------+----------------+
 *
 * Each room is one screen: 15x11 tiles at 16 px, so its camera rect is exactly
 * the 240x176 playfield. Coordinates here are in TILES; buildRoomGraph()
 * multiplies by the layer's tile size.
 *
 * The connection table and the character map in OverworldMap.cpp have to agree.
 * A connection with a wall behind it is dead data — the player never reaches
 * the seam — and an opening with no connection dumps the player off the edge of
 * the room with nowhere to go.
 */

namespace zelda_overworld {

namespace gameplay = pixelroot32::gameplay;

/// Shorthand for an unconnected direction, so the table below stays readable.
inline constexpr uint16_t kWall = gameplay::kNoRoomConnection;

// --- Room Metadata ---
static const gameplay::RoomData OVERWORLD_ROOMS[] = {
    // originCol, originRow, cols, rows, { Up, Down, Left, Right }
    {  0,  0, kRoomCols, kRoomRows, { kWall,     2, kWall,     1 } },  // 0 north-west
    { 15,  0, kRoomCols, kRoomRows, { kWall,     3,     0, kWall } },  // 1 north-east
    {  0, 11, kRoomCols, kRoomRows, {     0, kWall, kWall,     3 } },  // 2 start screen
    { 15, 11, kRoomCols, kRoomRows, {     1, kWall,     2, kWall } },  // 3 south-east
};

static const gameplay::RoomLayer OVERWORLD_ROOM_LAYER = {
    OVERWORLD_ROOMS,  // rooms
    4,                // roomCount
    kTileSize,        // tileWidth
    kTileSize         // tileHeight
};

} // namespace zelda_overworld

#endif // PIXELROOT32_ENABLE_GAMEPLAY_ROOM
