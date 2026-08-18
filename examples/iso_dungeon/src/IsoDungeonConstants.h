#pragma once

#include <cstdint>

#include "gameplay/Projection.h"
#include "graphics/Color.h"

/**
 * @file IsoDungeonConstants.h
 * @brief Room layout and the projection everything in this example is placed
 *        with.
 *
 * The whole isometric view of this game is six integers. There is no isometric
 * mode, no diamond-specific renderer and no coordinate class: a
 * `ProjectionSpec` says where cell (0, 0) lands and how far one step along each
 * cell axis moves on screen, and that is the entire model.
 */

namespace iso_dungeon {

namespace gameplay = pixelroot32::gameplay;

/// Display size in pixels (square 240x240).
inline constexpr int kDisplaySize = 240;

/// Button indices matching the InputConfig order (Up, Down, Left, Right, A, B).
inline constexpr std::uint8_t BTN_UP    = 0;
inline constexpr std::uint8_t BTN_DOWN  = 1;
inline constexpr std::uint8_t BTN_LEFT  = 2;
inline constexpr std::uint8_t BTN_RIGHT = 3;

// --- Geometry ---------------------------------------------------------------

/// Room extent in tiles, both axes.
inline constexpr int kRoomTiles = 7;

/// Half the width and half the height of one floor diamond, in pixels.
///
/// 32x16 is the classic 2:1 isometric tile. It is not an arbitrary choice
/// here: at 7 tiles per side the room's diamond is (7 + 7) * 16 = 224 px wide,
/// which is the largest 2:1 tile that keeps the whole room inside a 240 px
/// display without scrolling. A 48x24 tile would need 336 px.
inline constexpr int kTileHalfWidth  = 16;
inline constexpr int kTileHalfHeight = 8;

/// Screen position of tile (0, 0)'s diamond CENTRE.
///
/// The origin is the centre rather than the sprite's top-left corner because
/// every sprite in this example is anchored by a FOOT_Y measured to the same
/// point (see DungeonTiles.h). One anchor convention, one subtraction, no
/// per-sprite fudge factors.
inline constexpr int kIsoOriginX = 120;
inline constexpr int kIsoOriginY = 88;

/**
 * @brief Tile grid: +1 tileX steps down-right, +1 tileY steps down-left.
 *
 * det = 256, a power of two, so `screenToCell`'s single division is
 * strength-reduced to a shift for this `constexpr` spec.
 */
inline constexpr gameplay::ProjectionSpec kTileProjection{
    kIsoOriginX, kIsoOriginY,
     kTileHalfWidth, kTileHalfHeight,   // +1 tileX -> right and down
    -kTileHalfWidth, kTileHalfHeight};  // +1 tileY -> left  and down

static_assert(gameplay::projectionSpecIsValid(kTileProjection, kRoomTiles, kRoomTiles),
              "kTileProjection exceeds Scalar's fixed-point range.");
static_assert(gameplay::projectionDet(kTileProjection) == 256,
              "kTileProjection's determinant is no longer a power of two; "
              "screenToCell would stop strength-reducing to a shift.");

// --- Room layout ------------------------------------------------------------

/// Room extent is fixed for every room in the dungeon, so a single constant
/// sizes every layout array and every bounds check. The layouts themselves,
/// and the doors between them, live in RoomCatalog.h -- this header stays the
/// geometry and hero tuning it started as.
///
/// Tile roles, indexed `layout[tileY][tileX]`:
///
/// | Char | Meaning                                    | Walkable |
/// |------|--------------------------------------------|----------|
/// | `W`  | stone wall                                 | no       |
/// | `D`  | doorway in the wall running along +x       | yes      |
/// | `E`  | doorway in the wall running along +y       | yes      |
/// | `.`  | plain floor                                | yes      |
/// | `a`  | accent floor (the ritual square)           | yes      |
/// | `A`  | altar, standing on accent floor            | no       |
/// | `P`  | pillar                                     | no       |
///
/// The two back edges (tileY == 0 and tileX == 0) are walls; the two front
/// edges are open, with the black backdrop reading as the drop beyond them.
/// That framing is not decoration -- see RoomCatalog.h for why the front edges
/// can never carry a wall, and what that costs the dungeon's topology.
///
/// The doorways are two chars rather than one because which face a doorway is
/// carved into depends on which wall it sits in, and inferring that from the
/// coordinates would silently do the wrong thing the first time a wall moves.
///
/// Doorways are walkable: standing on one is what triggers the room change.

/// Where the hero starts, in tiles, in room 0.
inline constexpr int kSpawnTileX = 2;
inline constexpr int kSpawnTileY = 5;

// --- Hero -------------------------------------------------------------------

/// Length of one logic step in milliseconds.
///
/// Movement runs on a fixed clock rather than the render frame so a slow frame
/// stretches nothing: `gameplay::tickStep` advances by exactly one, and the
/// hero covers a tile in the same wall-clock time on a 240 MHz ESP32 as it
/// does on a desktop that renders four times faster.
inline constexpr unsigned long kLogicStepMs = 16;

/// Logic steps one tile of travel takes (~256 ms per tile).
///
/// This is the number that makes the movement read as a board rather than as
/// a walk: the hero is always either at rest on a tile or travelling between
/// two named tiles, never anywhere else.
inline constexpr int kStepsPerCell = 16;

// --- Colours ----------------------------------------------------------------

/// The backdrop beyond the room.
///
/// Under this example's custom palette a `graphics::Color` is an INDEX, and its
/// name is meaningless: `Color::Black` is index 0, which the renderer treats as
/// transparent. Index 1 -- spelled `Color::White` -- is where the palette puts
/// pure black. Naming the intent here keeps that trap in one place.
inline constexpr pixelroot32::graphics::Color kVoidColor =
    pixelroot32::graphics::Color::White;

// --- Cell helpers -----------------------------------------------------------

/// True when a tile coordinate pair is inside the room.
constexpr bool containsTile(int tileX, int tileY) {
    return tileX >= 0 && tileY >= 0 && tileX < kRoomTiles && tileY < kRoomTiles;
}

}  // namespace iso_dungeon
