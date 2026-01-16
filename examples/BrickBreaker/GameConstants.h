#pragma once
#include <cstdint>

namespace brickbreaker {

    // Input Constants
    constexpr uint8_t BTN_START = 0;
    constexpr uint8_t BTN_RIGHT = 1;
    constexpr uint8_t BTN_LEFT  = 2;

    // Game Constants
    constexpr int BRICK_WIDTH = 30;
    constexpr int BRICK_HEIGHT = 12;
    constexpr int PADDLE_W = 40;
    constexpr int PADDLE_H = 8;
    constexpr int BALL_SIZE = 6;
    constexpr float BORDER_TOP = 20.0f;

}
