#pragma once
#include <cstdint>

namespace room_screen {

/// Display size in pixels (square 240x240).
inline constexpr int kDisplaySize = 240;

// Room geometry is NOT declared here — it lives in the exported scene asset
// (assets/roomscreen_main_scene.h) so there is a single source of truth for it.

/// Button indices matching the InputConfig order (Up, Down, Left, Right, A, B).
inline constexpr std::uint8_t BTN_UP    = 0;
inline constexpr std::uint8_t BTN_DOWN  = 1;
inline constexpr std::uint8_t BTN_LEFT  = 2;
inline constexpr std::uint8_t BTN_RIGHT = 3;

/// Player sprite and hitbox size in pixels (16x16, one tile).
inline constexpr int kPlayerSize = 16;

/// Collision strategy used against the Items layer (see Player::canOccupy).
///
/// WholeTile:      original behavior — a tile flagged TILE_SOLID blocks its
///                 whole 16x16 cell; the tile's bitmap is ignored.
/// PerPixel:       per-pixel test against the tile bitmap (palette index 0 is
///                 walkable). Transparent "dead pixels" are ignored.
/// PerPixelEroded: per-pixel test with morphological erosion — thin branches,
///                 dangling 1px protrusions and stray opaque pixels on
///                 irregular tiles don't snag the player.
///
/// This is a compile-time constant: Player::canOccupy branches on it with
/// `if constexpr`, so the unselected branches are discarded and cost nothing
/// at runtime (and on ESP32 the unused engine helper is stripped by
/// --gc-sections).
enum class CollisionMode : uint8_t {
    WholeTile,
    PerPixel,
    PerPixelEroded,
};

/// Selects the player's collision mode. Set to CollisionMode::WholeTile to
/// restore the pre-per-pixel behavior, or CollisionMode::PerPixel to keep
/// per-pixel without erosion.
inline constexpr CollisionMode kCollisionMode = CollisionMode::PerPixelEroded;

/// Collision erosion radius applied to the tile bitmap, in pixels. Only used
/// when kCollisionMode == CollisionMode::PerPixelEroded.
///
/// The player hitbox stays at the full kPlayerSize x kPlayerSize; instead the
/// tile's solid silhouette is eroded by this many pixels on every side before
/// it is tested (see pixelroot32::physics::isTilePixelSolid erodePx). A solid
/// pixel counts only if the whole (2*erodePx+1)^2 square around it is opaque,
/// so thin branches, dangling 1px protrusions and stray opaque pixels inside
/// an irregular tile (plants, tree canopies) stop blocking the player, while
/// the tile's core stays solid.
///
/// Inverting the margin this way avoids the penetration problem of shrinking
/// the player hitbox: the player never gets caught in a concavity because the
/// object's silhouette is what shrinks, not the body that walks into it.
inline constexpr int kTileSolidErosionPx = 1;
/// Player walk speed in pixels per second.
inline constexpr int kPlayerSpeedPxPerSec = 90;
/// How long one walk frame is held, in milliseconds.
inline constexpr unsigned long kWalkFrameMs = 100;

/// Index of the Items behavior layer in `behavior_layers[]` (roomscreen_main_scene.h).
/// The editor exported the Items layer as the collision layer; its tiles carry
/// TILE_SOLID where the player cannot walk.
inline constexpr std::uint8_t kItemsBehaviorLayer = 1;

} // namespace room_screen
