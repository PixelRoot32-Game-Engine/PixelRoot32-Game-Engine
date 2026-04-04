#pragma once
#include <cstdint>

namespace tictactoe {

/**
 * @file GameConstants.h
 * @brief Tic-tac-toe configuration.
 */

    /** Input button IDs */
    constexpr uint8_t BTN_UP = 0;
    constexpr uint8_t BTN_DOWN = 1;
    constexpr uint8_t BTN_SELECT = 4;
    constexpr uint8_t BTN_NEXT = 3;
    constexpr uint8_t BTN_PREV = 2;

    // Game Constants
    constexpr int BOARD_SIZE = 3;
    constexpr int CELL_SIZE = 50;
    constexpr int BOARD_Y_OFFSET = 40;

    /** AI difficulty (0=perfect, 1=random) */
    constexpr float DEFAULT_AI_ERROR_CHANCE = 0.25f;

}
