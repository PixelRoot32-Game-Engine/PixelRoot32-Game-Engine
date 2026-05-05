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

// ============================================================================
// MCTS AI for 2048 - Implementation
// ============================================================================

// Board representation for MCTS (16 cells)
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

// Execute move and return new board
inline Game2048Board executeMove(const Game2048Board& board, MoveDirection dir) {
    Game2048Board result = board;
    bool moved = false;
    
    if (dir == MoveDirection::LEFT) {
        for (int r = 0; r < 4; r++) {
            int row[4] = {static_cast<int>(result.cells[r*4]), static_cast<int>(result.cells[r*4+1]), 
                          static_cast<int>(result.cells[r*4+2]), static_cast<int>(result.cells[r*4+3])};
            int temp[4] = {0,0,0,0};
            int out = 0;
            for (int c = 0; c < 4; c++) {
                if (row[c] != 0) {
                    if (out > 0 && temp[out-1] == row[c] && (temp[out-1] & 0x8000) == 0) {
                        temp[out-1] = row[c] * 2;
                        temp[out-1] |= 0x8000; // merged flag
                        moved = true;
                    } else {
                        temp[out++] = row[c];
                        if (c != out-1) moved = true;
                    }
                }
            }
            for (int c = 0; c < 4; c++) result.cells[r*4+c] = temp[c] & 0x7FFF;
        }
    } else if (dir == MoveDirection::RIGHT) {
        for (int r = 0; r < 4; r++) {
            int row[4] = {static_cast<int>(result.cells[r*4+3]), static_cast<int>(result.cells[r*4+2]), 
                          static_cast<int>(result.cells[r*4+1]), static_cast<int>(result.cells[r*4])};
            int temp[4] = {0,0,0,0};
            int out = 0;
            for (int c = 0; c < 4; c++) {
                if (row[c] != 0) {
                    if (out > 0 && temp[out-1] == row[c] && (temp[out-1] & 0x8000) == 0) {
                        temp[out-1] = row[c] * 2;
                        temp[out-1] |= 0x8000;
                        moved = true;
                    } else {
                        temp[out++] = row[c];
                        if (c != out-1) moved = true;
                    }
                }
            }
            for (int c = 0; c < 4; c++) result.cells[r*4+(3-c)] = temp[c] & 0x7FFF;
        }
    } else if (dir == MoveDirection::UP) {
        for (int c = 0; c < 4; c++) {
            int col[4] = {static_cast<int>(result.cells[c]), static_cast<int>(result.cells[4+c]), 
                          static_cast<int>(result.cells[8+c]), static_cast<int>(result.cells[12+c])};
            int temp[4] = {0,0,0,0};
            int out = 0;
            for (int r = 0; r < 4; r++) {
                if (col[r] != 0) {
                    if (out > 0 && temp[out-1] == col[r] && (temp[out-1] & 0x8000) == 0) {
                        temp[out-1] = col[r] * 2;
                        temp[out-1] |= 0x8000;
                        moved = true;
                    } else {
                        temp[out++] = col[r];
                        if (r != out-1) moved = true;
                    }
                }
            }
            for (int r = 0; r < 4; r++) result.cells[r*4+c] = temp[r] & 0x7FFF;
        }
    } else if (dir == MoveDirection::DOWN) {
        for (int c = 0; c < 4; c++) {
            int col[4] = {static_cast<int>(result.cells[12+c]), static_cast<int>(result.cells[8+c]), 
                          static_cast<int>(result.cells[4+c]), static_cast<int>(result.cells[c])};
            int temp[4] = {0,0,0,0};
            int out = 0;
            for (int r = 0; r < 4; r++) {
                if (col[r] != 0) {
                    if (out > 0 && temp[out-1] == col[r] && (temp[out-1] & 0x8000) == 0) {
                        temp[out-1] = col[r] * 2;
                        temp[out-1] |= 0x8000;
                        moved = true;
                    } else {
                        temp[out++] = col[r];
                        if (r != out-1) moved = true;
                    }
                }
            }
            for (int r = 0; r < 4; r++) result.cells[(3-r)*4+c] = temp[r] & 0x7FFF;
        }
    }
    
    if (!moved) return board;
    return result;
}

// Check if move is valid (changes board state)
inline bool isValidMove(const Game2048Board& board, MoveDirection dir) {
    Game2048Board result = executeMove(board, dir);
    return result != board;
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
// Phase 1: MCTS Data Structures
// ============================================================================

// MCTS Node structure
struct MCTSNode {
    uint32_t visits;
    float winScore;
    Game2048Board board;
    uint8_t childCount;
    int16_t firstChild;
    int16_t parentIndex;
    MoveDirection moveDir;
    
    MCTSNode() : visits(0), winScore(0.0f), childCount(0), firstChild(-1), parentIndex(-1), moveDir(MoveDirection::NONE) {}
};

// Fixed-size pool of MCTS nodes (stack-based allocation)
template<size_t POOL_SIZE>
class MCTSNodePool {
public:
    MCTSNodePool() : nodeCount_(0) {
        for (size_t i = 0; i < POOL_SIZE; i++) {
            nodes_[i].visits = 0;
            nodes_[i].winScore = 0.0f;
            nodes_[i].childCount = 0;
            nodes_[i].firstChild = -1;
            nodes_[i].parentIndex = -1;
        }
    }
    
    int allocate() {
        for (size_t i = 0; i < POOL_SIZE; i++) {
            if (nodes_[i].visits == 0 && nodes_[i].parentIndex == -1) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    void resetVisits() {
        for (size_t i = 0; i < POOL_SIZE; i++) {
            nodes_[i].visits = 0;
            nodes_[i].winScore = 0.0f;
        }
    }
    
    MCTSNode& getNode(int index) { return nodes_[index]; }
    const MCTSNode& getNode(int index) const { return nodes_[index]; }
    
private:
    MCTSNode nodes_[POOL_SIZE];
    int nodeCount_;
};

// UCT formula
inline float calculateUCT(float winScore, uint32_t visits, uint32_t parentVisits) {
    if (visits == 0) return 1e9f;
    constexpr float C = 1.41421356237f;
    float exploitation = winScore / static_cast<float>(visits);
    float exploration = C * sqrtf(static_cast<float>(logf(static_cast<float>(parentVisits)) / static_cast<float>(visits)));
    return exploitation + exploration;
}

// ============================================================================
// Phase 2: MCTS Core Implementation
// ============================================================================

template<size_t SIMULATION_BUDGET = 500>
class MCTSCore {
public:
    static constexpr int MAX_SIMULATION_MOVES = 50;
    static constexpr uint32_t BUDGET = SIMULATION_BUDGET;
    
    MCTSCore() : pool_(), rootIndex_(-1), simCounter_(0) {}
    
    int16_t search(const Game2048Board& board) {
        simCounter_++;
        rootIndex_ = pool_.allocate();
        if (rootIndex_ < 0) return 0;
        
        MCTSNode& root = pool_.getNode(rootIndex_);
        root.board = board;
        root.visits = 0;
        root.winScore = 0.0f;
        root.childCount = 0;
        root.firstChild = -1;
        root.parentIndex = -1;
        
        for (uint32_t sim = 0; sim < BUDGET; sim++) {
            int selectedNode = selectAndExpand(rootIndex_);
            if (selectedNode < 0) continue;
            float simResult = simulate(pool_.getNode(selectedNode).board);
            backpropagate(selectedNode, simResult);
        }
        
        return selectBestChild(rootIndex_);
    }
    
    MoveDirection getBestMove() {
        if (rootIndex_ < 0) return MoveDirection::UP;
        MCTSNode& root = pool_.getNode(rootIndex_);
        if (root.firstChild < 0) return MoveDirection::UP;
        
        int bestChild = -1;
        uint32_t bestVisits = 0;
        
        for (int i = root.firstChild; i >= 0; i = pool_.getNode(i).firstChild) {
            const MCTSNode& child = pool_.getNode(i);
            if (child.visits > bestVisits) {
                bestVisits = child.visits;
                bestChild = i;
            }
        }
        
        if (bestChild >= 0) return pool_.getNode(bestChild).moveDir;
        return MoveDirection::UP;
    }
    
    void reset() {
        pool_.resetVisits();
        rootIndex_ = -1;
    }

private:
    int selectAndExpand(int rootIdx) {
        int current = rootIdx;
        int loopCount = 0;
        
        while (loopCount < 100 && current >= 0) {
            loopCount++;
            MCTSNode& node = pool_.getNode(current);
            
            if (isGameOver(node.board)) return current;
            
            const MoveDirection dirs[4] = { MoveDirection::UP, MoveDirection::DOWN, MoveDirection::LEFT, MoveDirection::RIGHT };
            
            int validMoveCount = 0;
            for (int d = 0; d < 4; d++) {
                if (isValidMove(node.board, dirs[d])) validMoveCount++;
            }
            
            if (node.childCount < validMoveCount) {
                return expand(current);
            }
            
            int bestChild = selectBestUCTChild(current);
            if (bestChild < 0) return current;
            current = bestChild;
        }
        return rootIdx;
    }
    
    int selectBestUCTChild(int parentIdx) {
        const MCTSNode& parent = pool_.getNode(parentIdx);
        if (parent.firstChild < 0) return -1;
        
        int bestChild = -1;
        float bestUCT = -1e9f;
        
        bool visited[1024] = {false};
        int iterations = 0;
        
        for (int i = parent.firstChild; i >= 0 && iterations < 1024; i = pool_.getNode(i).firstChild) {
            if (visited[i]) break;
            visited[i] = true;
            iterations++;
            
            const MCTSNode& child = pool_.getNode(i);
            if (child.visits == 0) continue;
            float uct = calculateUCT(child.winScore, child.visits, parent.visits);
            if (uct > bestUCT) {
                bestUCT = uct;
                bestChild = i;
            }
        }
        return bestChild;
    }
    
    int expand(int parentIdx) {
        MCTSNode& parent = pool_.getNode(parentIdx);
        
        MoveDirection availableDirs[4];
        int availableCount = 0;
        
        const MoveDirection dirs[4] = { MoveDirection::UP, MoveDirection::DOWN, MoveDirection::LEFT, MoveDirection::RIGHT };
        
        for (int d = 0; d < 4; d++) {
            if (isValidMove(parent.board, dirs[d])) {
                availableDirs[availableCount++] = dirs[d];
            }
        }
        
        if (availableCount == 0) return -1;
        
        bool usedDir[4] = {false, false, false, false};
        for (int i = parent.firstChild; i >= 0; i = pool_.getNode(i).firstChild) {
            const MCTSNode& child = pool_.getNode(i);
            for (int d = 0; d < 4; d++) {
                if (child.moveDir == availableDirs[d]) usedDir[d] = true;
            }
        }
        
        int newDirIdx = -1;
        for (int d = 0; d < availableCount; d++) {
            if (!usedDir[d]) { newDirIdx = d; break; }
        }
        
        if (newDirIdx < 0) return -1;
        
        MoveDirection newDir = availableDirs[newDirIdx];
        Game2048Board newBoard = executeMove(parent.board, newDir);
        
        // Add random tile
        int emptyPos[16];
        int emptyCount;
        getEmptyCells(newBoard, emptyPos, emptyCount);
        
        if (emptyCount > 0) {
            uint32_t seed = static_cast<uint32_t>(simCounter_ * 1103515245 + 12345);
            int randIdx = seed % emptyCount;
            int pos = emptyPos[randIdx];
            uint16_t tileValue = ((seed % 10) < 9) ? 2 : 4;
            newBoard = placeTile(newBoard, pos, tileValue);
        }
        
        int newNodeIdx = pool_.allocate();
        if (newNodeIdx < 0) return parentIdx;
        
        MCTSNode& newNode = pool_.getNode(newNodeIdx);
        newNode.board = newBoard;
        newNode.visits = 0;
        newNode.winScore = 0.0f;
        newNode.childCount = 0;
        newNode.firstChild = -1;
        newNode.parentIndex = parentIdx;
        newNode.moveDir = newDir;
        
        newNode.firstChild = parent.firstChild;
        parent.firstChild = newNodeIdx;
        parent.childCount++;
        
        return newNodeIdx;
    }
    
    float simulate(Game2048Board board) {
        int moves = 0;
        float score = 0.0f;
        
        while (moves < MAX_SIMULATION_MOVES && !isGameOver(board)) {
            MoveDirection validMoves[4];
            int validCount = 0;
            
            const MoveDirection dirs[4] = { MoveDirection::UP, MoveDirection::DOWN, MoveDirection::LEFT, MoveDirection::RIGHT };
            
            for (int d = 0; d < 4; d++) {
                if (isValidMove(board, dirs[d])) {
                    validMoves[validCount++] = dirs[d];
                }
            }
            
            if (validCount == 0) break;
            
            // Random move
            uint32_t seed = static_cast<uint32_t>(simCounter_ * 1103515245 + 12345 + moves);
            int randIdx = seed % validCount;
            MoveDirection chosenMove = validMoves[randIdx];
            
            board = executeMove(board, chosenMove);
            moves++;
            
            // Add random tile
            if (!isGameOver(board)) {
                int emptyPos[16];
                int emptyCount;
                getEmptyCells(board, emptyPos, emptyCount);
                
                if (emptyCount > 0) {
                    uint32_t seed2 = static_cast<uint32_t>(simCounter_ * 7 + moves * 13 + 456789);
                    int randPosIdx = seed2 % emptyCount;
                    int pos = emptyPos[randPosIdx];
                    uint16_t tileValue = ((seed2 % 10) < 9) ? 2 : 4;
                    board = placeTile(board, pos, tileValue);
                }
            }
        }
        
        // Calculate final score: prefer higher max tile
        uint16_t maxTile = 0;
        for (int i = 0; i < 16; i++) {
            if (board.cells[i] > maxTile) maxTile = board.cells[i];
        }
        
        // Score based on max tile achieved
        if (maxTile >= 2048) score = 1000.0f;
        else if (maxTile >= 1024) score = 500.0f;
        else if (maxTile >= 512) score = 200.0f;
        else if (maxTile >= 256) score = 100.0f;
        else score = static_cast<float>(maxTile);
        
        return score;
    }
    
    void backpropagate(int nodeIdx, float score) {
        int current = nodeIdx;
        while (current >= 0) {
            MCTSNode& node = pool_.getNode(current);
            node.visits++;
            node.winScore += score;
            current = node.parentIndex;
        }
    }
    
    int selectBestChild(int parentIdx) {
        const MCTSNode& parent = pool_.getNode(parentIdx);
        if (parent.firstChild < 0) return 0;
        
        int bestChild = parent.firstChild;
        uint32_t maxVisits = pool_.getNode(bestChild).visits;
        
        for (int i = parent.firstChild; i >= 0; i = pool_.getNode(i).firstChild) {
            if (pool_.getNode(i).visits > maxVisits) {
                maxVisits = pool_.getNode(i).visits;
                bestChild = i;
            }
        }
        return bestChild;
    }
    
    MCTSNodePool<1024> pool_;
    int rootIndex_;
    uint32_t simCounter_;
};

// ============================================================================
// Phase 3: MCTSAI Class
// ============================================================================

template<uint32_t SIM_BUDGET = 500>
class MCTSAI {
public:
    MCTSAI() : core_() {}
    
    MoveDirection getBestMove(const Game2048Logic& logic) {
        const auto& grid = logic.getGrid();
        Game2048Board board = Game2048Board::fromGrid(grid);
        
        if (logic.isGameOver() || isGameOver(board)) {
            return MoveDirection::NONE;
        }
        
        core_.search(board);
        MoveDirection bestDir = core_.getBestMove();
        
        // Validate move
        if (!isValidMove(board, bestDir)) {
            const MoveDirection dirs[4] = { MoveDirection::UP, MoveDirection::DOWN, MoveDirection::LEFT, MoveDirection::RIGHT };
            for (int i = 0; i < 4; i++) {
                if (isValidMove(board, dirs[i])) return dirs[i];
            }
            return MoveDirection::NONE;
        }
        return bestDir;
    }
    
    void reset() { core_.reset(); }
    
private:
    MCTSCore<SIM_BUDGET> core_;
};

// ============================================================================
// Phase 4: Replace DefaultAI with MCTSAI
// ============================================================================

#ifdef GAME2048_AI_MODE
using DefaultAI = MCTSAI<500>;
#else
using DefaultAI = AI2048;
#endif

} // namespace ai
} // namespace game2048