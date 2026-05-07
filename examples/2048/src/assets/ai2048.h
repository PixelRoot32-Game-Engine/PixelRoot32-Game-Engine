#pragma once
#include "Game2048Constants.h"
#include "Game2048Logic.h"
#include <cstdint>
#include <cstdio>
#include <cmath>

/**
 * @file ai2048.h
 * @brief AI for 2048 - Expectimax with optimized heuristic
 *
 * Algorithm: Expectimax search with depth adaptation per platform
 * Heuristic: Smoothness + Monotonicity + Empty cells + Corner bonus + Penalty breaks
 */

namespace game2048 {
namespace ai {

enum class MoveDirection {
    UP, DOWN, LEFT, RIGHT, NONE
};

// ============================================================================
// Board Representation and Core Operations
// ============================================================================

struct Game2048Board {
    uint16_t cells[16];
    
    static Game2048Board fromGrid(const uint16_t grid[16]) {
        Game2048Board b;
        for (int i = 0; i < 16; i++) b.cells[i] = grid[i];
        return b;
    }
    
    bool operator==(const Game2048Board& other) const {
        for (int i = 0; i < 16; i++) if (cells[i] != other.cells[i]) return false;
        return true;
    }
    
    bool operator!=(const Game2048Board& other) const { return !(*this == other); }
};

// Execute move and return new board (no random tile spawn here)
inline Game2048Board executeMovePure(const Game2048Board& board, MoveDirection dir, bool& moved) {
    Game2048Board result = board;
    moved = false;
    
    auto processLine = [](int* in, int* out) -> bool {
        int temp[4] = {0,0,0,0};
        int pos = 0;
        bool lineMoved = false;
        
        // Slide and merge
        for (int i = 0; i < 4; i++) {
            if (in[i] != 0) {
                if (pos > 0 && temp[pos-1] == in[i]) {
                    temp[pos-1] *= 2;
                    lineMoved = true;
                } else {
                    temp[pos++] = in[i];
                    if (pos-1 != i) lineMoved = true;
                }
            }
        }
        
        for (int i = 0; i < 4; i++) out[i] = temp[i];
        return lineMoved;
    };
    
    if (dir == MoveDirection::LEFT) {
        for (int r = 0; r < 4; r++) {
            int row[4] = {result.cells[r*4], result.cells[r*4+1], result.cells[r*4+2], result.cells[r*4+3]};
            int out[4];
            if (processLine(row, out)) moved = true;
            for (int c = 0; c < 4; c++) result.cells[r*4+c] = static_cast<uint16_t>(out[c]);
        }
    } else if (dir == MoveDirection::RIGHT) {
        for (int r = 0; r < 4; r++) {
            int row[4] = {result.cells[r*4+3], result.cells[r*4+2], result.cells[r*4+1], result.cells[r*4]};
            int out[4];
            if (processLine(row, out)) moved = true;
            for (int c = 0; c < 4; c++) result.cells[r*4+(3-c)] = static_cast<uint16_t>(out[c]);
        }
    } else if (dir == MoveDirection::UP) {
        for (int c = 0; c < 4; c++) {
            int col[4] = {result.cells[c], result.cells[4+c], result.cells[8+c], result.cells[12+c]};
            int out[4];
            if (processLine(col, out)) moved = true;
            for (int r = 0; r < 4; r++) result.cells[r*4+c] = static_cast<uint16_t>(out[r]);
        }
    } else if (dir == MoveDirection::DOWN) {
        for (int c = 0; c < 4; c++) {
            int col[4] = {result.cells[12+c], result.cells[8+c], result.cells[4+c], result.cells[c]};
            int out[4];
            if (processLine(col, out)) moved = true;
            for (int r = 0; r < 4; r++) result.cells[(3-r)*4+c] = static_cast<uint16_t>(out[r]);
        }
    }
    
    return result;
}

inline Game2048Board executeMove(const Game2048Board& board, MoveDirection dir) {
    bool moved;
    return executeMovePure(board, dir, moved);
}

// Check if move is valid (changes board state)
inline bool isValidMove(const Game2048Board& board, MoveDirection dir) {
    bool moved;
    Game2048Board result = executeMovePure(board, dir, moved);
    (void)result;
    return moved;
}

// Get empty cell positions
inline void getEmptyCells(const Game2048Board& board, int* positions, int& count) {
    count = 0;
    for (int i = 0; i < 16; i++) {
        if (board.cells[i] == 0) {
            positions[count++] = i;
        }
    }
}

// Place tile at position
inline Game2048Board placeTile(const Game2048Board& board, int pos, uint16_t value) {
    Game2048Board result = board;
    result.cells[pos] = value;
    return result;
}

// Check if game is over
inline bool isGameOver(const Game2048Board& board) {
    for (int i = 0; i < 16; i++) {
        if (board.cells[i] == 0) return false;
    }
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            if (board.cells[r*4+c] == board.cells[r*4+c+1]) return false;
        }
    }
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 3; r++) {
            if (board.cells[r*4+c] == board.cells[(r+1)*4+c]) return false;
        }
    }
    return true;
}

// ============================================================================
// Transposition Table for caching evaluated states
// Simple fixed-size hash table with LRU replacement
// Platform-adaptive: smaller on ESP32 to avoid DRAM overflow
// ============================================================================

#ifdef PLATFORM_NATIVE
    static constexpr int TT_SIZE = 8192;   // 8K entries for PC (plenty of RAM)
#else
    static constexpr int TT_SIZE = 1024;   // 1K entries for ESP32 (saves ~112KB DRAM)
#endif
static constexpr int TT_MASK = TT_SIZE - 1;

struct TTEntry {
    uint64_t key;      // Board hash
    float score;       // Cached score
    uint8_t depth;     // Depth at which this was evaluated
    bool valid;        // Is this entry valid?
    
    TTEntry() : key(0), score(0.0f), depth(0), valid(false) {}
};

// Simple transposition table (global for simplicity in this header-only design)
class TranspositionTable {
public:
    TTEntry entries[TT_SIZE];
    
    TranspositionTable() {
        clear();
    }
    
    void clear() {
        for (int i = 0; i < TT_SIZE; i++) {
            entries[i].valid = false;
        }
    }
    
    // FNV-1a hash for 64-bit
    static uint64_t hashBoard(const Game2048Board& board) {
        uint64_t hash = 14695981039346656037ULL;  // FNV offset basis
        for (int i = 0; i < 16; i++) {
            hash ^= static_cast<uint64_t>(board.cells[i]);
            hash *= 1099511628211ULL;  // FNV prime
        }
        return hash;
    }
    
    bool lookup(const Game2048Board& board, int depth, float& outScore) {
        uint64_t key = hashBoard(board);
        int idx = static_cast<int>(key & TT_MASK);
        
        const TTEntry& entry = entries[idx];
        if (entry.valid && entry.key == key && entry.depth >= depth) {
            outScore = entry.score;
            return true;
        }
        return false;
    }
    
    void store(const Game2048Board& board, int depth, float score) {
        uint64_t key = hashBoard(board);
        int idx = static_cast<int>(key & TT_MASK);
        
        TTEntry& entry = entries[idx];
        // Simple replacement: only replace if deeper or empty
        if (!entry.valid || depth >= entry.depth) {
            entry.key = key;
            entry.score = score;
            entry.depth = static_cast<uint8_t>(depth);
            entry.valid = true;
        }
    }
};

// ============================================================================
// Optimized Heuristic Evaluation for 2048
// Based on nneonneo/Ovolve approach - classical heuristics with aggressive weights
//
// Key changes from previous version:
// - Removed rigid snake pattern (blocks T/L formations needed for recovery)
// - Using direct tile values (not log2) for monotonicity - larger tiles matter more
// - 5 weighted factors: empty, monotonicity penalty, smoothness, merges, corner bonus
// ============================================================================

inline float evaluateBoardHeuristic(const Game2048Board& board) {
    float score = 0.0f;
    uint16_t maxTile = 0;
    int maxTilePos = 0;
    int emptyCount = 0;

    // Basic stats
    for (int i = 0; i < 16; i++) {
        if (board.cells[i] > maxTile) {
            maxTile = board.cells[i];
            maxTilePos = i;
        }
        if (board.cells[i] == 0) emptyCount++;
    }

    // 1. EMPTY SQUARES - critical for mobility
    // Weight increases dramatically as board fills (exponential urgency)
    float emptyWeight;
    if (emptyCount == 0) {
        score -= 200000.0f;  // Game over imminent
        return score;
    } else if (emptyCount <= 2) {
        emptyWeight = 100000.0f;
    } else if (emptyCount <= 4) {
        emptyWeight = 30000.0f;
    } else if (emptyCount <= 6) {
        emptyWeight = 10000.0f;
    } else {
        emptyWeight = 4000.0f;
    }
    score += emptyCount * emptyWeight;

    // 2. MONOTONICITY PENALTY (using direct values, not log!)
    // For each row/col, compute penalty for non-monotonic sequences
    // Larger tiles contribute more to penalty (critical fix)
    float monoPenalty = 0.0f;

    // Rows: check left-to-right and right-to-left, take min penalty (AI picks direction)
    for (int r = 0; r < 4; r++) {
        float leftToRight = 0.0f;
        float rightToLeft = 0.0f;
        for (int c = 0; c < 3; c++) {
            int idx1 = r*4+c;
            int idx2 = r*4+c+1;
            if (board.cells[idx1] > board.cells[idx2]) {
                leftToRight += board.cells[idx1] - board.cells[idx2];  // decreasing
            } else {
                rightToLeft += board.cells[idx2] - board.cells[idx1];  // increasing
            }
        }
        monoPenalty += (leftToRight < rightToLeft) ? leftToRight : rightToLeft;
    }

    // Columns: check top-to-bottom and bottom-to-top
    for (int c = 0; c < 4; c++) {
        float topToBottom = 0.0f;
        float bottomToTop = 0.0f;
        for (int r = 0; r < 3; r++) {
            int idx1 = r*4+c;
            int idx2 = (r+1)*4+c;
            if (board.cells[idx1] > board.cells[idx2]) {
                topToBottom += board.cells[idx1] - board.cells[idx2];  // decreasing
            } else {
                bottomToTop += board.cells[idx2] - board.cells[idx1];    // increasing
            }
        }
        monoPenalty += (topToBottom < bottomToTop) ? topToBottom : bottomToTop;
    }

    score -= monoPenalty * 100.0f;  // Penalty weight - MUCH more aggressive!

    // 3. SMOOTHNESS - reward similar adjacent tiles (merge potential)
    // Penalize differences between neighbors (using log for smoothness only)
    float smoothness = 0.0f;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            if (board.cells[r*4+c] > 0 && board.cells[r*4+c+1] > 0) {
                float v1 = static_cast<float>(board.cells[r*4+c]);
                float v2 = static_cast<float>(board.cells[r*4+c+1]);
                // Use square root to smooth differences
                float diff = sqrtf(v1) - sqrtf(v2);
                smoothness -= (diff < 0.0f ? -diff : diff);
            }
        }
    }
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 3; r++) {
            if (board.cells[r*4+c] > 0 && board.cells[(r+1)*4+c] > 0) {
                float v1 = static_cast<float>(board.cells[r*4+c]);
                float v2 = static_cast<float>(board.cells[(r+1)*4+c]);
                float diff = sqrtf(v1) - sqrtf(v2);
                smoothness -= (diff < 0.0f ? -diff : diff);
            }
        }
    }
    score += smoothness * 100.0f;  // Increased smoothness weight

    // 4. MERGE POTENTIAL - count adjacent equal tiles (big bonus!)
    int mergeCount = 0;
    int mergeValue = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            if (board.cells[r*4+c] > 0 && board.cells[r*4+c] == board.cells[r*4+c+1]) {
                mergeCount++;
                mergeValue += board.cells[r*4+c];
            }
        }
    }
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 3; r++) {
            if (board.cells[r*4+c] > 0 && board.cells[r*4+c] == board.cells[(r+1)*4+c]) {
                mergeCount++;
                mergeValue += board.cells[r*4+c];
            }
        }
    }
    score += mergeCount * 1000.0f;     // Bonus per available merge - doubled!
    score += mergeValue * 5.0f;        // Bonus proportional to merge value - increased

    // 5. MAX TILE IN CORNER - EXTREME bonus/penalty
    // This is CRITICAL - the max tile MUST be in a corner
    const int corners[4] = {0, 3, 12, 15};
    bool maxInCorner = false;
    for (int c : corners) {
        if (maxTilePos == c) { maxInCorner = true; break; }
    }
    if (maxInCorner) {
        score += static_cast<float>(maxTile) * 100.0f;   // Good bonus for corner
    } else {
        score -= static_cast<float>(maxTile) * 1000.0f;  // CATASTROPHIC penalty for non-corner!
    }
    
    // 6. SECONDARY tiles should also be near the max tile (adjacent to corner)
    // Check if the 2nd and 3rd largest tiles are adjacent to the max tile
    if (maxInCorner) {
        int r = maxTilePos / 4;
        int c = maxTilePos % 4;
        // Adjacent positions (up/down/left/right within bounds)
        const int dr[4] = {-1, 1, 0, 0};
        const int dc[4] = {0, 0, -1, 1};
        float adjacentBonus = 0.0f;
        for (int i = 0; i < 4; i++) {
            int nr = r + dr[i];
            int nc = c + dc[i];
            if (nr >= 0 && nr < 4 && nc >= 0 && nc < 4) {
                int idx = nr * 4 + nc;
                adjacentBonus += static_cast<float>(board.cells[idx]) * 0.5f;
            }
        }
        score += adjacentBonus;
    }

    return score;
}

// ============================================================================
// Complete Expectimax Algorithm
// Player turn (MAX): chooses best move from 4 directions
// Chance turn: evaluates ALL empty cells with spawn-2 (90%) AND spawn-4 (10%)
//
// Key improvements:
// - No sampling: evaluates all empty cells (up to 16)
// - Proper probability weighting: 90% spawn-2, 10% spawn-4
// - Probability cutoff: stops exploring paths with < 0.0001 probability
// ============================================================================

template<int MAX_DEPTH_AI, int MAX_DEPTH_CHANCE>
class ExpectimaxAI {
public:
    ExpectimaxAI() = default;

    MoveDirection getBestMove(const Game2048Logic& logic) {
        const auto& grid = logic.getGrid();
        Game2048Board board = Game2048Board::fromGrid(grid);

        if (logic.isGameOver() || isGameOver(board)) {
            return MoveDirection::NONE;
        }
        
        // Phase 3: Clear transposition table at start of each move
        tt.clear();

        return searchBestMove(board);
    }

    void reset() {
        // No persistent state to reset in Expectimax
    }

private:
    // Phase 3: Transposition table for caching evaluated states
    TranspositionTable tt;
    
    // Phase 4: Adaptive depth calculation based on empty cells
    // Conservative depth to avoid freezing - speed is critical
    int calculateAdaptiveDepth(int emptyCount) const {
        // Base depth from template parameters
        constexpr int BASE_DEPTH = MAX_DEPTH_AI;
        
        // Conservative: avoid freezing, especially with many empty cells
        if (emptyCount > 10) {
            return BASE_DEPTH - 1 > 1 ? BASE_DEPTH - 1 : 2;  // Reduce depth when board is open
        } else if (emptyCount >= 6) {
            return BASE_DEPTH;  // Normal depth
        } else if (emptyCount >= 3) {
            return BASE_DEPTH + 1;  // Slightly deeper when tight
        } else {
            return BASE_DEPTH + 2;  // Deep search in critical moments only
        }
    }
    
    // Search for best move at depth 0 (AI turn - MAX node)
    MoveDirection searchBestMove(const Game2048Board& board) {
        const MoveDirection dirs[4] = {
            MoveDirection::UP, MoveDirection::DOWN,
            MoveDirection::LEFT, MoveDirection::RIGHT
        };

        float bestScore = -1e9f;
        MoveDirection bestDir = MoveDirection::NONE;
        bool foundValid = false;

        for (int d = 0; d < 4; d++) {
            bool moved;
            Game2048Board afterMove = executeMovePure(board, dirs[d], moved);

            if (!moved) continue;
            foundValid = true;

            // Now it's chance turn (spawn tile)
            float score = expectiChance(afterMove, 0, 1.0f);

            if (score > bestScore) {
                bestScore = score;
                bestDir = dirs[d];
            }
        }

        if (!foundValid) {
            return MoveDirection::NONE;
        }

        return bestDir;
    }

    // Chance node: spawn tiles with smart sampling
    // When many cells empty: sample representative cells to avoid explosion
    // When few cells empty: evaluate all (critical moments need accuracy)
    float expectiChance(const Game2048Board& board, int depth, float probSoFar) {
        int emptyPos[16];
        int emptyCount;
        getEmptyCells(board, emptyPos, emptyCount);

        // Phase 4: Use adaptive effective depth
        int effectiveMaxDepth = calculateAdaptiveDepth(emptyCount);
        
        if (emptyCount == 0) {
            if (isGameOver(board)) {
                return evaluateBoardHeuristic(board);
            }
            return expectiMax(board, depth + 1, probSoFar);
        }
        
        if (depth >= effectiveMaxDepth) {
            return evaluateBoardHeuristic(board);
        }

        // Probability cutoff: if this path is extremely unlikely, stop exploring
        if (probSoFar < 0.0001f) {
            return evaluateBoardHeuristic(board);
        }

        // SMART SAMPLING: When many empty cells, sample to avoid explosion
        // When board is tight (< 8 empty), evaluate all for accuracy
        int cellsToEval = emptyCount;
        if (emptyCount > 8) {
            // Sample 6 cells: first, last, and 4 evenly spaced
            cellsToEval = 6;
        }
        
        float totalScore = 0.0f;
        float probPerCell = 1.0f / static_cast<float>(cellsToEval);

        // Evaluate spawn-2 (90%) AND spawn-4 (10%) for selected cells
        for (int i = 0; i < cellsToEval; i++) {
            int idx = (emptyCount > 8) ? (i * emptyCount / cellsToEval) : i;
            
            // Spawn 2 with 90% probability
            {
                Game2048Board afterSpawn = placeTile(board, emptyPos[idx], 2);
                float score = expectiMax(afterSpawn, depth + 1, probSoFar * probPerCell * 0.9f);
                totalScore += score * probPerCell * 0.9f;
            }

            // Spawn 4 with 10% probability
            {
                Game2048Board afterSpawn = placeTile(board, emptyPos[idx], 4);
                float score = expectiMax(afterSpawn, depth + 1, probSoFar * probPerCell * 0.1f);
                totalScore += score * probPerCell * 0.1f;
            }
        }

        return totalScore;
    }

    // MAX node: AI chooses best move
    float expectiMax(const Game2048Board& board, int depth, float probSoFar) {
        int emptyCount = 0;
        for (int i = 0; i < 16; i++) if (board.cells[i] == 0) emptyCount++;
        
        // Phase 4: Use adaptive effective depth
        int effectiveMaxDepth = calculateAdaptiveDepth(emptyCount);
        
        if (depth >= effectiveMaxDepth || isGameOver(board)) {
            return evaluateBoardHeuristic(board);
        }
        
        // Phase 3: Check transposition table
        float cachedScore;
        if (tt.lookup(board, depth, cachedScore)) {
            return cachedScore;
        }

        const MoveDirection dirs[4] = {
            MoveDirection::UP, MoveDirection::DOWN,
            MoveDirection::LEFT, MoveDirection::RIGHT
        };

        float bestScore = -1e9f;
        bool hasValidMove = false;

        for (int d = 0; d < 4; d++) {
            bool moved;
            Game2048Board afterMove = executeMovePure(board, dirs[d], moved);

            if (!moved) continue;
            hasValidMove = true;

            float score = expectiChance(afterMove, depth, probSoFar);
            if (score > bestScore) {
                bestScore = score;
            }
        }

        if (!hasValidMove) {
            return evaluateBoardHeuristic(board);
        }
        
        // Phase 3: Store result in transposition table
        tt.store(board, depth, bestScore);

        return bestScore;
    }
};

// ============================================================================
// Platform-Adaptive AI Configuration
// ============================================================================

// Phase 4: Adaptive depth configuration
// Balanced: good planning without freezing
// Native: base 3, adaptive 2-5 (reduced when many empty cells)
// ESP32: base 2, adaptive 1-4
#ifdef PLATFORM_NATIVE
    using DefaultAI = ExpectimaxAI<3, 3>;
#else
    using DefaultAI = ExpectimaxAI<2, 2>;
#endif

} // namespace ai
} // namespace game2048