#pragma once
#include <cstdint>

namespace legend_of_clone {

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

/**
 * @brief Shorthand for an unconnected direction in a room table.
 *
 * Spelled as a literal here so both room tables can use it without either one
 * having to include the other. `OverworldRooms.h` static_asserts that it still
 * matches the engine's own sentinel, so a change on either side breaks the
 * build instead of silently mis-wiring rooms.
 */
inline constexpr uint16_t kWall = 0xFFFF;

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

/**
 * @brief Collision strategy used against the map (see PlayerActor::canOccupy).
 *
 * WholeTile:      a tile the export flags solid blocks its whole 16x16 cell;
 *                 the tile's bitmap is ignored. Cheapest, and correct for
 *                 rectangular masonry like TILE_ROCK.
 * PerPixel:       the cell must be flagged solid AND the tile's bitmap must be
 *                 opaque at that pixel (palette index 0 is walkable). Note
 *                 that none of THIS map's terrain tiles have an index-0 pixel
 *                 -- a bush is drawn over its own background, not on
 *                 transparency -- so on this art PerPixel is indistinguishable
 *                 from WholeTile. It is here for maps whose tiles do carry
 *                 real transparency. See the README for the measured counts.
 * PerPixelEroded: per-pixel with morphological erosion, so a tree crown's
 *                 outermost fringe of pixels does not snag the player.
 *
 * Compile-time: canOccupy branches with `if constexpr`, so the unselected
 * branches are discarded and cost nothing at runtime. On ESP32 the unused
 * engine helper is then stripped by --gc-sections.
 */
enum class CollisionMode : uint8_t {
    WholeTile,
    PerPixel,
    PerPixelEroded,
};

/// Selects the collision mode. WholeTile restores the pre-per-pixel behaviour.
inline constexpr CollisionMode kCollisionMode = CollisionMode::PerPixelEroded;

/**
 * @brief Erosion radius applied to the tile bitmap, in pixels. Only read when
 *        kCollisionMode == CollisionMode::PerPixelEroded.
 *
 * The player hitbox stays at the full kPlayerSize; it is the *object's* solid
 * silhouette that shrinks by this many pixels on every side before it is
 * tested. A solid pixel counts only if the whole (2*erodePx+1)^2 square around
 * it is opaque, so a tree's 1px fringe stops blocking while its trunk stays
 * solid.
 *
 * Eroding the object rather than the body avoids the penetration problem of a
 * shrunken hitbox: the player never ends up caught inside a concavity, because
 * what shrank is the thing being walked into, not the thing walking.
 */
inline constexpr int kTileSolidErosionPx = 1;

// ---------------------------------------------------------------------------
// The two maps meet here
// ---------------------------------------------------------------------------
// The overworld and the dungeon are separate scenes with separate coordinate
// spaces, so the two ends of the doorway have to be written down somewhere.
// This is that somewhere: change a map and these move with it, and the seam
// checker verifies both cells are what they claim to be.

/// Where the overworld drops the player back after leaving the dungeon —
/// directly below the cave mouth, so walking up re-enters it.
inline constexpr int kCaveExitCol = 5;
inline constexpr int kCaveExitRow = 16;

/// Where the dungeon puts the player on entry: one tile above its stairs.
inline constexpr int kDungeonEntryCol = 7;
inline constexpr int kDungeonEntryRow = 18;

/**
 * Duration of the fade in and out of a dungeon, in milliseconds.
 *
 * Each phase runs for this long, so a doorway costs twice this end to end. The
 * NES holds a plain black screen for roughly a third of a second either side.
 */
inline constexpr unsigned long kDoorwayFadeMs = 180;

/**
 * How long one walk frame is held, in milliseconds.
 *
 * The NES reloads Link's animation counter with 6 and toggles the frame when it
 * hits zero, so a frame lasts 6 video frames — 99.8 ms at the console's 60.0988
 * Hz. Rounded to 100 ms here, which is the same number in wall-clock terms and
 * survives a variable frame budget.
 */
inline constexpr unsigned long kWalkFrameMs = 100;

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

} // namespace legend_of_clone
