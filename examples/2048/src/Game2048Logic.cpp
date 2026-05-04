#include "Game2048Logic.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>

namespace game2048 {

Game2048Logic::Game2048Logic() : score(0), highestTile(0), gameOver(false), won(false), movedThisTurn(false) {
    // Initialize RNG
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    // Initialize empty grid
    reset();
}

/**
 * @brief Reset the game to initial state.
 * Spawns two initial tiles as per 2048 rules.
 */
void Game2048Logic::reset() {
    // Clear grid
    for (int i = 0; i < GRID_TOTAL_CELLS; ++i) {
        grid[i] = EMPTY_TILE;
    }

    score = 0;
    highestTile = 0;
    gameOver = false;
    won = false;
    movedThisTurn = false;

    // Spawn two initial tiles
    spawnTile();
    spawnTile();
}

/**
 * @brief Spawn a new tile in a random empty cell.
 * 90% chance of 2, 10% chance of 4.
 */
void Game2048Logic::spawnTile() {
    // Find empty cells
    int emptyIndices[GRID_TOTAL_CELLS];
    int emptyCount = 0;

    for (int i = 0; i < GRID_TOTAL_CELLS; ++i) {
        if (grid[i] == EMPTY_TILE) {
            emptyIndices[emptyCount++] = i;
        }
    }

    if (emptyCount == 0) {
        return;  // No empty cells
    }

    // Pick random empty cell
    int randIndex = std::rand() % emptyCount;
    int cellIndex = emptyIndices[randIndex];

    // Determine tile value (90% 2, 10% 4)
    float randFloat = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    grid[cellIndex] = (randFloat < SPAWN_TILE_2_PROBABILITY) ? TILE_2 : TILE_4;

    // Update highest tile (for win detection fallback)
    if (grid[cellIndex] > highestTile) {
        highestTile = grid[cellIndex];
    }
}

/**
 * @brief Compress a row - move all non-zero tiles to the left.
 * Example: [0, 2, 0, 4] -> [2, 4, 0, 0]
 */
void Game2048Logic::compressRow(uint16_t* row) {
    int writeIndex = 0;

    // Find non-zero tiles and compress left
    for (int readIndex = 0; readIndex < GRID_SIZE; ++readIndex) {
        if (row[readIndex] != 0) {
            if (writeIndex != readIndex) {
                row[writeIndex] = row[readIndex];
                row[readIndex] = 0;
            }
            ++writeIndex;
        }
    }
}

/**
 * @brief Merge adjacent equal tiles in a row.
 * Only one merge per tile per turn.
 * Score increases by the merged value.
 */
void Game2048Logic::mergeRow(uint16_t* row) {
    for (int i = 0; i < GRID_SIZE - 1; ++i) {
        if (row[i] != 0 && row[i] == row[i + 1]) {
            // Merge tiles
            row[i] *= 2;
            row[i + 1] = 0;

            // Add to score
            score += row[i];

            // Update highest tile
            if (row[i] > highestTile) {
                highestTile = row[i];
            }

            // Check for win
            if (row[i] == WINNING_TILE && !won) {
                won = true;
            }

            // Mark this row as having a merge for this turn
            // (we'll reset this in the main move function)
        }
    }
}

/**
 * @brief Second compression after merge to close gaps.
 * Example: [2, 0, 2, 0] after first compress and merge [4, 0, 0, 0]
 */
void Game2048Logic::secondCompressRow(uint16_t* row) {
    // Same as compressRow - run again to close gaps from merges
    int writeIndex = 0;

    for (int readIndex = 0; readIndex < GRID_SIZE; ++readIndex) {
        if (row[readIndex] != 0) {
            if (writeIndex != readIndex) {
                row[writeIndex] = row[readIndex];
                row[readIndex] = 0;
            }
            ++writeIndex;
        }
    }
}

/**
 * @brief Slide and merge a single row (for moveLeft).
 * Implements: Compress -> Merge -> Second Compress
 */
bool Game2048Logic::slideAndMergeRow(uint16_t* row) {
    // Make a copy to detect changes
    uint16_t original[GRID_SIZE];
    for (int i = 0; i < GRID_SIZE; ++i) {
        original[i] = row[i];
    }

    // Step 1: Compress (move non-zero left)
    compressRow(row);

    // Step 2: Merge adjacent equals
    mergeRow(row);

    // Step 3: Second compress (close gaps from merge)
    secondCompressRow(row);

    // Check if row changed
    for (int i = 0; i < GRID_SIZE; ++i) {
        if (original[i] != row[i]) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Rotate grid 90 degrees counter-clockwise.
 */
void Game2048Logic::rotateGridLeft() {
    Grid newGrid;
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            // Rotate 90 degrees counter-clockwise:
            // new[row][col] = old[col][GRID_SIZE - 1 - row]
            // But we use flat index: index = row * GRID_SIZE + col
            int fromIndex = col * GRID_SIZE + (GRID_SIZE - 1 - row);
            newGrid[row * GRID_SIZE + col] = grid[fromIndex];
        }
    }
    for (int i = 0; i < GRID_TOTAL_CELLS; ++i) {
        grid[i] = newGrid[i];
    }
}

/**
 * @brief Rotate grid 90 degrees clockwise.
 */
void Game2048Logic::rotateGridRight() {
    // Three left rotations = one right rotation
    rotateGridLeft();
    rotateGridLeft();
    rotateGridLeft();
}

/**
 * @brief Move all tiles left and merge.
 * Implements the core 2048 movement logic:
 * 1. For each row: compress -> merge -> second compress
 * 2. Track if any move occurred
 */
bool Game2048Logic::moveLeft() {
    bool anyMove = false;

    for (int row = 0; row < GRID_SIZE; ++row) {
        // Get row as array
        uint16_t* rowPtr = &grid[row * GRID_SIZE];
        bool rowMoved = slideAndMergeRow(rowPtr);
        if (rowMoved) {
            anyMove = true;
        }
    }

    return anyMove;
}

/**
 * @brief Move all tiles right.
 * For each row: reverse, move left, reverse back.
 * This ensures correct merge behavior for all tile positions.
 */
bool Game2048Logic::moveRight() {
    bool anyMove = false;

    for (int row = 0; row < GRID_SIZE; ++row) {
        uint16_t* rowPtr = &grid[row * GRID_SIZE];
        
        // Reverse the row to process as if moving left
        // [16, 4, 0, 4] -> [4, 0, 4, 16]
        std::reverse(rowPtr, rowPtr + GRID_SIZE);
        
        // Apply slide and merge (same as moveLeft)
        bool rowMoved = slideAndMergeRow(rowPtr);
        
        // Reverse back to restore original orientation
        // [8, 16, 0, 0] -> [0, 0, 16, 8]
        std::reverse(rowPtr, rowPtr + GRID_SIZE);
        
        if (rowMoved) {
            anyMove = true;
        }
    }

    return anyMove;
}

/**
 * @brief Move all tiles up.
 * Process columns as rows: reverse each column, move left, reverse back.
 */
bool Game2048Logic::moveUp() {
    bool anyMove = false;

    for (int col = 0; col < GRID_SIZE; ++col) {
        // Extract column into temp array
        uint16_t colData[GRID_SIZE];
        for (int row = 0; row < GRID_SIZE; ++row) {
            colData[row] = grid[row * GRID_SIZE + col];
        }
        
        // Reverse column to process as if moving left
        std::reverse(colData, colData + GRID_SIZE);
        
        // Apply slide and merge
        uint16_t temp[GRID_SIZE];
        for (int i = 0; i < GRID_SIZE; ++i) temp[i] = colData[i];
        bool rowMoved = slideAndMergeRow(temp);
        
        // Reverse back
        std::reverse(temp, temp + GRID_SIZE);
        
        // Write back to column
        for (int row = 0; row < GRID_SIZE; ++row) {
            grid[row * GRID_SIZE + col] = temp[row];
        }
        
        if (rowMoved) anyMove = true;
    }

    return anyMove;
}

/**
 * @brief Move all tiles down.
 * Process columns as rows: reverse, move left, reverse, reverse again (for down direction).
 */
bool Game2048Logic::moveDown() {
    bool anyMove = false;

    for (int col = 0; col < GRID_SIZE; ++col) {
        // Extract column into temp array
        uint16_t colData[GRID_SIZE];
        for (int row = 0; row < GRID_SIZE; ++row) {
            colData[row] = grid[row * GRID_SIZE + col];
        }
        
        // For down: first reverse (because moveLeft goes to left, we want right)
        // [a,b,c,d] -> [d,c,b,a], then after compress->merge->compress->reverse again
        // becomes [d,c,b,a] -> [d,c,b,a], merge, reverse -> [a,b,c,d] moved right
        
        // Actually simpler: just reverse twice - once to flip, once to restore after processing
        // But since moveLeft pushes to LEFT, for DOWN we need to:
        // 1. Reverse the column
        // 2. Process as moveLeft (compresses to left)
        // 3. Reverse again (now rightmost becomes leftmost of original, i.e. moved right)
        
        std::reverse(colData, colData + GRID_SIZE);
        
        // Apply slide and merge
        uint16_t temp[GRID_SIZE];
        for (int i = 0; i < GRID_SIZE; ++i) temp[i] = colData[i];
        bool rowMoved = slideAndMergeRow(temp);
        
        // Reverse back to get proper down direction
        std::reverse(temp, temp + GRID_SIZE);
        
        // Write back to column
        for (int row = 0; row < GRID_SIZE; ++row) {
            grid[row * GRID_SIZE + col] = temp[row];
        }
        
        if (rowMoved) anyMove = true;
    }

    return anyMove;
}

/**
 * @brief Check if there are empty cells in the grid.
 */
bool Game2048Logic::hasEmptyCells() const {
    for (int i = 0; i < GRID_TOTAL_CELLS; ++i) {
        if (grid[i] == EMPTY_TILE) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if any adjacent tiles can be merged.
 */
bool Game2048Logic::canMerge(const Grid& g) const {
    // Check horizontal merges
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE - 1; ++col) {
            int idx = row * GRID_SIZE + col;
            if (g[idx] != 0 && g[idx] == g[idx + 1]) {
                return true;
            }
        }
    }

    // Check vertical merges
    for (int col = 0; col < GRID_SIZE; ++col) {
        for (int row = 0; row < GRID_SIZE - 1; ++row) {
            int idx = row * GRID_SIZE + col;
            int belowIdx = (row + 1) * GRID_SIZE + col;
            if (g[idx] != 0 && g[idx] == g[belowIdx]) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Check if game is over (no empty cells and no merges possible).
 */
void Game2048Logic::checkGameOver() {
    // Game is over if no empty cells AND no merges possible
    gameOver = !hasEmptyCells() && !canMerge(grid);
}

} // namespace game2048