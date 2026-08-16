#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM

#include "gameplay/RoomLayout.h"
#include "GameConstants.h"

/**
 * @file DungeonRooms.h
 * @brief The dungeon's exported room layer: four rooms, wired 2x2.
 *
 *     +----------------+----------------+
 *     | 0  north-west  | 1  north-east  |
 *     +----------------+----------------+
 *     | 2  ENTRANCE    | 3  south-east  |
 *     +----------------+----------------+
 *
 * Same shape and same wiring as the overworld's layer — see OverworldRooms.h
 * for the contract. The player arrives in room 2, beside the stairs.
 *
 * The connection table and the character map in DungeonMap.cpp have to agree.
 * A connection with a wall behind it is dead data; a doorway with no connection
 * behind it drops the player off the edge of the room.
 */

namespace legend_of_clone {

// --- Room Metadata ---
static const pixelroot32::gameplay::RoomData DUNGEON_ROOMS[] = {
    // originCol, originRow, cols, rows, { Up, Down, Left, Right }
    {  0,  0, kRoomCols, kRoomRows, { kWall,     2, kWall,     1 } },  // 0 north-west
    { 15,  0, kRoomCols, kRoomRows, { kWall,     3,     0, kWall } },  // 1 north-east
    {  0, 11, kRoomCols, kRoomRows, {     0, kWall, kWall,     3 } },  // 2 entrance
    { 15, 11, kRoomCols, kRoomRows, {     1, kWall,     2, kWall } },  // 3 south-east
};

static const pixelroot32::gameplay::RoomLayer DUNGEON_ROOM_LAYER = {
    DUNGEON_ROOMS,  // rooms
    4,              // roomCount
    kTileSize,      // tileWidth
    kTileSize       // tileHeight
};

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_GAMEPLAY_ROOM
