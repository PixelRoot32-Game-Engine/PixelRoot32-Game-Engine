#pragma once

namespace metroidvania {

/**
 * @file GameConstants.h
 * @brief Metroidvania platformer configuration.
 */

struct PlatformRect {
    float x;
    float y;
    float w;
    float h;
};

/** Input button IDs */
constexpr int BTN_UP = 0;
constexpr int BTN_DOWN = 1;
constexpr int BTN_LEFT = 2;
constexpr int BTN_RIGHT = 3;
constexpr int BTN_JUMP = 4;

constexpr int PLAYER_WIDTH = 8;
constexpr int PLAYER_HEIGHT = 8;

constexpr float PLAYER_GRAVITY = 400.0f;
constexpr float PLAYER_MOVE_SPEED = 60.0f;
constexpr float PLAYER_CLIMB_SPEED = 40.0f;
constexpr float PLAYER_JUMP_VELOCITY = 160.0f;

constexpr unsigned long ANIMATION_FRAME_TIME_MS = 120;

constexpr float PLAYER_START_X = 40.0f;
constexpr float PLAYER_START_Y = 80.0f;

} // namespace metroidvania
