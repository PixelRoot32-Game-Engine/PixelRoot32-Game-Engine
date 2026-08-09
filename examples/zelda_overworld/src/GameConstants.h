#pragma once
#include <cstdint>

namespace zelda_overworld {

// ---------------------------------------------------------------------------
// Screen layout
// ---------------------------------------------------------------------------
// The NES splits its 256x240 output into a 256x176 playfield and a 64px status
// bar. This example keeps that split on the engine's reference 240x240 panel:
// a 240x176 playfield with the status bar moved to the bottom.
//
// Putting the bar at the bottom is not cosmetic. With the playfield anchored at
// screen y = 0, world y maps straight to screen y and the camera offset is the
// only transform in play. A top bar would push a constant +64 into every world
// coordinate, tilemap origin and hit test in the example.

/// Display size in pixels (square 240x240, same panel as every other example).
inline constexpr int kDisplayWidth  = 240;
inline constexpr int kDisplayHeight = 240;

/// Tile size in pixels. NES overworld tiles are 16x16.
inline constexpr int kTileSize = 16;

/// One room is one screen: 15x11 tiles = 240x176 px.
inline constexpr int kRoomCols = 15;
inline constexpr int kRoomRows = 11;

/// Playfield in pixels — also the camera viewport.
inline constexpr int kPlayfieldWidth  = kRoomCols * kTileSize;  // 240
inline constexpr int kPlayfieldHeight = kRoomRows * kTileSize;  // 176

/// Status bar strip below the playfield. Reserved, not populated yet.
inline constexpr int kStatusBarY      = kPlayfieldHeight;        // 176
inline constexpr int kStatusBarHeight = kDisplayHeight - kPlayfieldHeight;  // 64

// ---------------------------------------------------------------------------
// World layout
// ---------------------------------------------------------------------------
// Iteration 1 defines a 2x2 block of adjacent overworld screens.

/// Rooms across and down in the world grid.
inline constexpr int kWorldRoomsX = 2;
inline constexpr int kWorldRoomsY = 2;

/// World size in tiles.
inline constexpr int kWorldCols = kRoomCols * kWorldRoomsX;  // 30
inline constexpr int kWorldRows = kRoomRows * kWorldRoomsY;  // 22

/// Total world cells — the length of every layer index array.
inline constexpr int kWorldCells = kWorldCols * kWorldRows;  // 660

/// Room indices, in the order OVERWORLD_ROOMS declares them.
inline constexpr uint16_t kRoomNorthWest = 0;
inline constexpr uint16_t kRoomNorthEast = 1;
inline constexpr uint16_t kRoomSouthWest = 2;  ///< Start screen (cave mouth).
inline constexpr uint16_t kRoomSouthEast = 3;

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

/// Player sprite and hitbox size in pixels.
inline constexpr int kPlayerSize = 16;

/**
 * Walking speed in pixels per second.
 *
 * The NES moves Link 1.5 px per frame at 60 Hz. 90 px/s is that rate expressed
 * in wall-clock time, so the feel survives a variable frame budget.
 */
inline constexpr int kPlayerSpeedPxPerSec = 90;

/// Player start cell, in world tile coordinates (start screen, below the cave).
inline constexpr int kPlayerStartCol = 7;
inline constexpr int kPlayerStartRow = 18;

// ---------------------------------------------------------------------------
// Screen transition
// ---------------------------------------------------------------------------

/**
 * Duration of the scrolling screen change, in milliseconds.
 *
 * The NES slides one full screen in a little under half a second while Link
 * walks the last few pixels across the seam. Input is ignored throughout.
 */
inline constexpr unsigned long kTransitionDurationMs = 420;

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

/// Button indices matching the InputConfig order (Up, Down, Left, Right, A, B).
inline constexpr std::uint8_t BTN_UP    = 0;
inline constexpr std::uint8_t BTN_DOWN  = 1;
inline constexpr std::uint8_t BTN_LEFT  = 2;
inline constexpr std::uint8_t BTN_RIGHT = 3;

} // namespace zelda_overworld
