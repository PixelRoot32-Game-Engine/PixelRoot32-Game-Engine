#pragma once
#include <cstdint>

namespace snake {

/**
 * @file GameConstants.h
 * @brief Snake game configuration.
 */

    /* Input button IDs */
    constexpr std::uint8_t BTN_UP = 0;
    constexpr std::uint8_t BTN_DOWN = 1;
    constexpr std::uint8_t BTN_LEFT = 2;
    constexpr std::uint8_t BTN_RIGHT = 3;
    constexpr std::uint8_t BTN_RESTART = 4;

    constexpr int CELL_SIZE = 10;
    constexpr int GRID_WIDTH = 24;
    constexpr int GRID_HEIGHT = 24;
    constexpr int TOP_UI_GRID_ROWS = 2;

    /* Text positions */
    constexpr int SCORE_TEXT_X = CELL_SIZE / 2;
    constexpr int SCORE_TEXT_Y = CELL_SIZE / 2;
    constexpr int GAME_OVER_TEXT_Y = DISPLAY_HEIGHT / 2 - 10;
    constexpr int PRESS_ACTION_TEXT_Y = DISPLAY_HEIGHT / 2 + 10;

    /* Movement timing */
    constexpr int INITIAL_MOVE_INTERVAL_MS = 150;
    constexpr int MIN_MOVE_INTERVAL_MS = 50;
    constexpr int MOVE_INTERVAL_STEP_MS = 2;

    constexpr int SCORE_PER_FOOD = 10;
}

