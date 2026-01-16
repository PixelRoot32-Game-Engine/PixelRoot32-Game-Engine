#pragma once
#include <cstdint>

namespace tictactoe {

    // Input Constants
    constexpr uint8_t BTN_SELECT = 0;
    constexpr uint8_t BTN_NEXT = 1;
    constexpr uint8_t BTN_PREV = 2;

    // Game Constants
    constexpr int BOARD_SIZE = 3;
    constexpr int CELL_SIZE = 50;
    constexpr int BOARD_Y_OFFSET = 40;
    
    // AI
    constexpr float DEFAULT_AI_ERROR_CHANCE = 0.25f;

}
