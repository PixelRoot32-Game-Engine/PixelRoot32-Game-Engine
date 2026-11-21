#pragma once
#include "platforms/EngineConfig.h"

#include <cstdint>

namespace platformerdemo {

/**
 * @file GameConstants.h
 * @brief Platformer example configuration: level size, player tuning, input.
 */

/// Logical screen is also the whole level: this example has no camera.
constexpr int LEVEL_WIDTH = DISPLAY_WIDTH;
constexpr int LEVEL_HEIGHT = DISPLAY_HEIGHT;

/// Fixed capacity of the scene's own arrays, so init() never allocates in-loop.
constexpr int MAX_LEVEL_BLOCKS = 8;
constexpr int MAX_CRATES = 3;

// Player tuning. Kinematic bodies have no world gravity: the game adds it, so
// these numbers are the whole physics of the character.
constexpr float PLAYER_GRAVITY = 400.0f;
constexpr float PLAYER_MOVE_SPEED = 90.0f;
constexpr float PLAYER_JUMP_VELOCITY = 220.0f;

constexpr int PLAYER_WIDTH = 12;
constexpr int PLAYER_HEIGHT = 14;
constexpr float PLAYER_START_X = 24.0f;
/// Resting exactly on the ground strip, whose top edge is at y = 208.
constexpr float PLAYER_START_Y = 194.0f;

constexpr int CRATE_SIZE = 10;

// --- Input mapping -------------------------------------------------------
// Six buttons are configured by the platform headers:
//   0 Up, 1 Down, 2 Left, 3 Right, 4 A (jump), 5 B.
constexpr uint8_t BTN_LEFT = 2;
constexpr uint8_t BTN_RIGHT = 3;
constexpr uint8_t BTN_JUMP = 4;
constexpr uint8_t BTN_RESET = 5;

}
