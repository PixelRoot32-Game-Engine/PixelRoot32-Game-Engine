#pragma once
#include <cstdint>
#include "platforms/EngineConfig.h"
#include "gameplay/GridSpace.h"

namespace tictactoe {

/**
 * @file GameConstants.h
 * @brief Tic-tac-toe configuration.
 */
    namespace gameplay = pixelroot32::gameplay;

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

    /// Cell<->world grid for the tic-tac-toe board. The origin centres the
    /// board on the display and is compile-time derivable, so kBoardGrid is
    /// constexpr and costs no SRAM.
    inline constexpr gameplay::GridSpec kBoardGrid{(DISPLAY_WIDTH - BOARD_SIZE * CELL_SIZE) / 2, (DISPLAY_HEIGHT - BOARD_SIZE * CELL_SIZE) / 2, CELL_SIZE, CELL_SIZE, BOARD_SIZE, BOARD_SIZE};
    static_assert(gameplay::gridSpecIsValid(kBoardGrid), "kBoardGrid exceeds Scalar's range or has an invalid cell size.");

}
