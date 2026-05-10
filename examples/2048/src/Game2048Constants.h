#pragma once
#include <cstdint>

// Debug mode: Enable AI debugging - uncomment to use
// #define GAME2048_DEBUG_SPAWN 1

namespace game2048 {

/**
 * @file Game2048Constants.h
 * @brief 2048 game configuration and constants.
 */

// Grid configuration
constexpr int GRID_SIZE = 4;
constexpr int GRID_TOTAL_CELLS = GRID_SIZE * GRID_SIZE;

// Tile values
constexpr uint16_t EMPTY_TILE = 0;
constexpr uint16_t TILE_2 = 2;
constexpr uint16_t TILE_4 = 4;

// Tile spawn probabilities
constexpr float SPAWN_TILE_2_PROBABILITY = 0.9f;  // 90% chance
constexpr float SPAWN_TILE_4_PROBABILITY = 0.1f;  // 10% chance

// Score values
constexpr int SCORE_2_MERGED = 2;
constexpr int SCORE_4_MERGED = 4;
constexpr int SCORE_8_MERGED = 8;
constexpr int SCORE_16_MERGED = 16;
constexpr int SCORE_32_MERGED = 32;
constexpr int SCORE_64_MERGED = 64;
constexpr int SCORE_128_MERGED = 128;
constexpr int SCORE_256_MERGED = 256;
constexpr int SCORE_512_MERGED = 512;
constexpr int SCORE_1024_MERGED = 1024;
constexpr int SCORE_2048_MERGED = 2048;

// Input button IDs
constexpr uint8_t BTN_UP = 0;
constexpr uint8_t BTN_DOWN = 1;
constexpr uint8_t BTN_LEFT = 2;
constexpr uint8_t BTN_RIGHT = 3;
constexpr uint8_t BTN_SELECT = 4;  // For restart

// Display constants
constexpr int CELL_SIZE = 50;
constexpr int GRID_OFFSET_X = 10;
constexpr int GRID_OFFSET_Y = 30;
constexpr int TILE_SPACING = 3;
constexpr int TILE_CORNER_RADIUS = 4;

// Maximum tile value that can be displayed
constexpr uint16_t MAX_DISPLAYED_TILE = 2048;

} // namespace game2048