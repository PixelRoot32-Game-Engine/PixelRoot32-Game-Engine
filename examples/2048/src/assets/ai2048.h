#pragma once
#include "Game2048Constants.h"
#include "Game2048Logic.h"
#include <cstdint>

/**
 * @file ai2048.h
 * @brief AI for 2048 - ANY MERGE IS A WIN
 *
 * Problem: When maxTile >= 256, need to MERGE ANYWHERE to reach next level.
 * Solution: Take ANY merge immediately, regardless of position.
 */

namespace game2048 {
namespace ai {

enum class MoveDirection {
    UP, DOWN, LEFT, RIGHT, NONE
};

class AI2048 {
public:
    explicit AI2048(int d = 3) : depth(d), targetCorner(2), initialized(false) {}

    MoveDirection getBestMove(const Game2048Logic& logic);
    void setDepth(int d) { depth = d; }
    void reset() { initialized = false; }

private:
    int depth;
    int targetCorner;  // 0=TL, 1=TR, 2=BL, 3=BR
    bool initialized;

    MoveDirection selectBestDirection(const Game2048Logic& logic) const;

    // Check if move creates any merge (any value)
    bool createsAnyMerge(const uint16_t* grid, MoveDirection dir) const;

    // Check if move specifically creates merge for target value
    bool createsMergeFor(const uint16_t* grid, MoveDirection dir, int targetValue) const;

    float evaluate(const uint16_t* grid, int maxTile) const;

    int countEmpty(const uint16_t* grid) const;
    int getMaxTile(const uint16_t* grid) const;
    int getMaxPosition(const uint16_t* grid) const;
};

// ============================================================================

inline MoveDirection AI2048::getBestMove(const Game2048Logic& logic) {
    const auto& grid = logic.getGrid();
    int maxPos = getMaxPosition(grid);

    if (!initialized) {
        // Start with corner closest to current max
        const int corners[4] = {0, 3, 12, 15};
        int minDist = 100;
        for (int i = 0; i < 4; i++) {
            int dist = abs(maxPos - corners[i]);
            if (dist < minDist) {
                minDist = dist;
                targetCorner = i;
            }
        }
        initialized = true;
    } else {
        // ALWAYs follow max to a corner if it's not already in one
        const int corners[4] = {0, 3, 12, 15};
        for (int i = 0; i < 4; i++) {
            if (maxPos == corners[i]) {
                targetCorner = i;  // Update to follow max!
                break;
            }
        }
        // If not in a corner, targetCorner stays same - will move toward it
    }

    return selectBestDirection(logic);
}

// ============================================================================
// CORE DECISION LOGIC
// ============================================================================

inline MoveDirection AI2048::selectBestDirection(const Game2048Logic& logic) const {
    const MoveDirection dirs[] = {
        MoveDirection::UP, MoveDirection::DOWN,
        MoveDirection::LEFT, MoveDirection::RIGHT
    };

    const auto& grid = logic.getGrid();
    int maxTile = getMaxTile(grid);

    // ========================================================================
    // PRIORITY 1 (HIGHEST): Any merge? TAKE IT IMMEDIATELY!
    // Check for TWO types of merges:
    // a) Merge that reaches 2048 (WIN!)
    // b) Merge that creates next power of 2 (progress)
    // c) Any merge (fallback)
    // ========================================================================

    // First: check if any merge reaches 2048 (the ultimate goal)
    for (auto dir : dirs) {
        if (createsMergeFor(grid, dir, 2048)) {
            Game2048Logic test = logic;
            bool moved = false;
            switch (dir) {
                case MoveDirection::UP:    moved = test.moveUp(); break;
                case MoveDirection::DOWN:  moved = test.moveDown(); break;
                case MoveDirection::LEFT:  moved = test.moveLeft(); break;
                case MoveDirection::RIGHT: moved = test.moveRight(); break;
                default: continue;
            }
            if (moved) return dir;
        }
    }

    // Second: check for merge that creates next power of 2 (e.g., 256->512, 512->1024)
    // This is more valuable than random merges
    int nextLevel = maxTile * 2;
    for (auto dir : dirs) {
        if (createsMergeFor(grid, dir, nextLevel)) {
            Game2048Logic test = logic;
            bool moved = false;
            switch (dir) {
                case MoveDirection::UP:    moved = test.moveUp(); break;
                case MoveDirection::DOWN:  moved = test.moveDown(); break;
                case MoveDirection::LEFT:  moved = test.moveLeft(); break;
                case MoveDirection::RIGHT: moved = test.moveRight(); break;
                default: continue;
            }
            if (moved) return dir;
        }
    }

    // Third: any other merge - take it to keep things moving
    for (auto dir : dirs) {
        if (createsAnyMerge(grid, dir)) {
            Game2048Logic test = logic;
            bool moved = false;
            switch (dir) {
                case MoveDirection::UP:    moved = test.moveUp(); break;
                case MoveDirection::DOWN:  moved = test.moveDown(); break;
                case MoveDirection::LEFT:  moved = test.moveLeft(); break;
                case MoveDirection::RIGHT: moved = test.moveRight(); break;
                default: continue;
            }
            if (moved) return dir;
        }
    }

    // ========================================================================
    // PRIORITY 2: If no merge, maximize potential future merges
    // ========================================================================

    float bestScore = -1000.0f;
    MoveDirection bestDir = MoveDirection::NONE;
    MoveDirection firstValid = MoveDirection::NONE;

    for (auto dir : dirs) {
        Game2048Logic test = logic;
        bool moved = false;
        switch (dir) {
            case MoveDirection::UP:    moved = test.moveUp(); break;
            case MoveDirection::DOWN:  moved = test.moveDown(); break;
            case MoveDirection::LEFT:  moved = test.moveLeft(); break;
            case MoveDirection::RIGHT: moved = test.moveRight(); break;
            default: continue;
        }

        if (!moved) continue;
        if (firstValid == MoveDirection::NONE) firstValid = dir;

        // Get NEW maxTile after the move (it might have increased!)
        int newMaxTile = getMaxTile(test.getGrid());
        float score = evaluate(test.getGrid(), newMaxTile);
        if (score > bestScore) {
            bestScore = score;
            bestDir = dir;
        }
    }

    return (bestDir != MoveDirection::NONE) ? bestDir : firstValid;
}

// ============================================================================
// Check if move creates ANY merge (any value)
// ============================================================================

inline bool AI2048::createsAnyMerge(const uint16_t* grid, MoveDirection dir) const {
    if (dir == MoveDirection::NONE) return false;

    // Simulate move - check if any merge happens
    if (dir == MoveDirection::LEFT || dir == MoveDirection::RIGHT) {
        for (int row = 0; row < GRID_SIZE; row++) {
            uint16_t rowData[GRID_SIZE];
            for (int c = 0; c < GRID_SIZE; c++) {
                rowData[c] = grid[row * GRID_SIZE + c];
            }

            // Compact
            uint16_t compact[GRID_SIZE] = {0};
            int w = 0;
            for (int i = 0; i < GRID_SIZE; i++) {
                if (rowData[i] != 0) compact[w++] = rowData[i];
            }

            // Check for any merge
            for (int i = 0; i < w - 1; i++) {
                if (compact[i] == compact[i + 1] && compact[i] > 0) {
                    return true;
                }
            }
        }
    } else {  // UP or DOWN
        for (int col = 0; col < GRID_SIZE; col++) {
            uint16_t colData[GRID_SIZE];
            for (int r = 0; r < GRID_SIZE; r++) {
                colData[r] = grid[r * GRID_SIZE + col];
            }

            uint16_t compact[GRID_SIZE] = {0};
            int w = 0;
            for (int i = 0; i < GRID_SIZE; i++) {
                if (colData[i] != 0) compact[w++] = colData[i];
            }

            for (int i = 0; i < w - 1; i++) {
                if (compact[i] == compact[i + 1] && compact[i] > 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

// ============================================================================
// Check if move creates merge for specific target value
// ============================================================================

inline bool AI2048::createsMergeFor(const uint16_t* grid, MoveDirection dir, int targetValue) const {
    if (dir == MoveDirection::NONE || targetValue <= 0) return false;

    if (dir == MoveDirection::LEFT || dir == MoveDirection::RIGHT) {
        for (int row = 0; row < GRID_SIZE; row++) {
            uint16_t compact[GRID_SIZE] = {0};
            int w = 0;
            for (int c = 0; c < GRID_SIZE; c++) {
                uint16_t val = grid[row * GRID_SIZE + c];
                if (val != 0) compact[w++] = val;
            }

            for (int i = 0; i < w - 1; i++) {
                if (compact[i] == compact[i + 1]) {
                    int merged = compact[i] * 2;
                    if (merged == targetValue) return true;
                }
            }
        }
    } else {
        for (int col = 0; col < GRID_SIZE; col++) {
            uint16_t compact[GRID_SIZE] = {0};
            int w = 0;
            for (int r = 0; r < GRID_SIZE; r++) {
                uint16_t val = grid[r * GRID_SIZE + col];
                if (val != 0) compact[w++] = val;
            }

            for (int i = 0; i < w - 1; i++) {
                if (compact[i] == compact[i + 1]) {
                    int merged = compact[i] * 2;
                    if (merged == targetValue) return true;
                }
            }
        }
    }

    return false;
}

// ============================================================================
// Heuristic: AGGRESSIVELY prioritize merges (game_rules.md rule 7: spawn only 2/4)
// To reach 2048 with only 2/4 spawns, we MUST create merges at every opportunity
// ============================================================================

inline float AI2048::evaluate(const uint16_t* grid, int maxTile) const {
    float score = 0.0f;

    int maxPos = getMaxPosition(grid);
    int empty = countEmpty(grid);
    const int corners[4] = {0, 3, 12, 15};
    int cornerIdx = corners[targetCorner];

    // ========================================================================
    // 1. Count PRESENT merges (how many pairs exist NOW that can merge)
    // This is CRITICAL - we must create merges to progress (game_objetive.md)
    // ========================================================================

    int currentMerges = 0;
    int maxMergeValue = 0;
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE - 1; c++) {
            int idx = r * GRID_SIZE + c;
            if (grid[idx] == grid[idx + 1] && grid[idx] > 0) {
                currentMerges++;
                maxMergeValue = std::max(maxMergeValue, static_cast<int>(grid[idx]) * 2);
            }
        }
    }
    for (int c = 0; c < GRID_SIZE; c++) {
        for (int r = 0; r < GRID_SIZE - 1; r++) {
            int idx = r * GRID_SIZE + c;
            int below = (r + 1) * GRID_SIZE + c;
            if (grid[idx] == grid[below] && grid[idx] > 0) {
                currentMerges++;
                maxMergeValue = std::max(maxMergeValue, static_cast<int>(grid[idx]) * 2);
            }
        }
    }

    // CRITICAL: Each merge is essential for reaching 2048
    // More merges = more progress toward goal
    score += static_cast<float>(currentMerges) * 25.0f;

    // Extra bonus if max merge value can increase maxTile (progress!)
    if (maxMergeValue > maxTile) {
        score += 50.0f;  // Big progress toward next level!
    }

    // ========================================================================
    // 2. Empty cells - but less important when merges available
    // ========================================================================

    score += static_cast<float>(empty) * 4.0f;

    // ========================================================================
    // 3. Max in corner - MANDATORY when no merges available!
    // This is critical for building a chain toward 2048
    // ========================================================================

    if (maxPos == cornerIdx) {
        score += 30.0f;  // Big bonus for keeping max in corner
    } else if (currentMerges == 0) {
        // No merges AND not in corner - PENALIZE HEAVILY
        // Must move max toward corner!
        int dist = abs(maxPos - cornerIdx);
        score -= static_cast<float>(dist) * 8.0f;  // Heavy penalty
    }
    // If merges exist, corner doesn't matter - take the merge!

    // ========================================================================
    // 4. Bonus for higher max (game progression)
    // ========================================================================

    score += static_cast<float>(maxTile) / 64.0f;

    // ========================================================================
    // 5. Penalty for monotonicity - keep similar values together
    // Helps create future merges
    // ========================================================================

    float monotonicity = 0.0f;
    // Horizontal
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE - 1; c++) {
            int idx = r * GRID_SIZE + c;
            if (grid[idx] > 0 && grid[idx + 1] > 0) {
                if (grid[idx] <= grid[idx + 1]) monotonicity += 1.0f;
            }
        }
    }
    // Vertical
    for (int c = 0; c < GRID_SIZE; c++) {
        for (int r = 0; r < GRID_SIZE - 1; r++) {
            int idx = r * GRID_SIZE + c;
            int below = (r + 1) * GRID_SIZE + c;
            if (grid[idx] > 0 && grid[below] > 0) {
                if (grid[idx] <= grid[below]) monotonicity += 1.0f;
            }
        }
    }
    score += monotonicity * 2.0f;

    return score;
}

// ============================================================================
// Helpers
// ============================================================================

inline int AI2048::countEmpty(const uint16_t* grid) const {
    int count = 0;
    for (int i = 0; i < GRID_TOTAL_CELLS; i++) {
        if (grid[i] == 0) count++;
    }
    return count;
}

inline int AI2048::getMaxTile(const uint16_t* grid) const {
    int maxVal = 0;
    for (int i = 0; i < GRID_TOTAL_CELLS; i++) {
        if (grid[i] > maxVal) maxVal = static_cast<int>(grid[i]);
    }
    return maxVal;
}

inline int AI2048::getMaxPosition(const uint16_t* grid) const {
    int maxVal = 0;
    int maxPos = 0;
    for (int i = 0; i < GRID_TOTAL_CELLS; i++) {
        if (grid[i] > maxVal) {
            maxVal = grid[i];
            maxPos = i;
        }
    }
    return maxPos;
}

} // namespace ai
} // namespace game2048