#pragma once
#include <cstdint>
#include "gameplay/GridSpace.h"

namespace snake {

/**
 * @file GameConstants.h
 * @brief Snake game configuration.
 */
    namespace gameplay = pixelroot32::gameplay;

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

    /// Cell<->world grid for the snake playfield (design.md D1); NOT used with
    /// containsCell() for the wall check (design.md D6) — see SnakeScene::update().
    inline constexpr gameplay::GridSpec kSnakeGrid{0, 0, CELL_SIZE, CELL_SIZE, GRID_WIDTH, GRID_HEIGHT};
    static_assert(gameplay::gridSpecIsValid(kSnakeGrid), "kSnakeGrid exceeds Scalar's range or has an invalid cell size.");

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

