#pragma once
#include <cstdint>

namespace pong {

    // Input Constants
    constexpr uint8_t BTN_UP = 0;
    constexpr uint8_t BTN_DOWN = 1;
    constexpr uint8_t BTN_START = 0; 

    // Game Constants
    constexpr int PADDLE_WIDTH = 10;
    constexpr int PADDLE_HEIGHT = 50;
    constexpr int BALL_RADIUS = 6;
    constexpr float BALL_SPEED = 120.0f;
    constexpr int SCORE_TO_WIN = 5;

    // AI Constants
    constexpr float AI_TARGET_OFFSET = 6.0f;
    constexpr float AI_MAX_SPEED = 90.0f;
    constexpr float AI_MOVEMENT_THRESHOLD = 0.5f;
    
    // Play Area
    constexpr int PONG_PLAY_AREA_HEIGHT = 160;

}
