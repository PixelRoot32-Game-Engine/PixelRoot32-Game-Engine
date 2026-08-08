#pragma once
#include <cstdint>

namespace room_screen {

/// Display size in pixels (square 240x240).
inline constexpr int kDisplaySize = 240;

// Room geometry is NOT declared here — it lives in the exported room layer
// (assets/RoomScreenRooms.h) so there is a single source of truth for it.

/// Button indices matching the InputConfig order (Up, Down, Left, Right, A, B).
inline constexpr std::uint8_t BTN_UP    = 0;
inline constexpr std::uint8_t BTN_DOWN  = 1;
inline constexpr std::uint8_t BTN_LEFT  = 2;
inline constexpr std::uint8_t BTN_RIGHT = 3;

} // namespace room_screen
