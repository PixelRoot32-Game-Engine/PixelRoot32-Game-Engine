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
/// Player walk speed in pixels per second.
inline constexpr int kPlayerSpeedPxPerSec = 90;
/// How long one walk frame is held, in milliseconds.
inline constexpr unsigned long kWalkFrameMs = 100;

/// Index of the Items behavior layer in `behavior_layers[]` (roomscreen_main_scene.h).
/// The editor exported the Items layer as the collision layer; its tiles carry
/// TILE_SOLID where the player cannot walk.
inline constexpr std::uint8_t kItemsBehaviorLayer = 1;

} // namespace room_screen
