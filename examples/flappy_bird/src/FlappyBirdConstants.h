#pragma once
#include <cstdint>

namespace flappy {

    /**
     * @file FlappyBirdConstants.h
     * @brief Game configuration for Flappy Bird clone.
     */

    /** Game flow states */
    enum class GameState {
        WAITING,   ///< Pre-start, waiting for first jump
        RUNNING,   ///< Active gameplay
        GAME_OVER  ///< Bird collided
    };

    /** Input button ID */
    constexpr std::uint8_t BTN_JUMP = 0;

    /** Physics */
    constexpr float GRAVITY = 0.08f;
    constexpr float JUMP_FORCE = -80.0f;

    /** Pipe layout and movement */
    constexpr float PIPE_SPEED = 0.6f;
    constexpr int PIPE_WIDTH = 12;      
    constexpr int PIPE_GAP = 22;

    /** Bird dimensions */
    constexpr int BIRD_RADIUS = 2;
    constexpr float BIRD_START_X = 10.0f;

    /** UI layout */
    constexpr int TOP_BOTTOM_PADDING = 10;
    constexpr int SCORE_X_OFFSET = 15;
    constexpr int SCORE_Y_OFFSET = 8;
}
