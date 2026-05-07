#pragma once
#include "Game2048Constants.h"
#include <cstdint>
#include <functional>

namespace game2048 {

/**
 * @class Game2048Logic
 * @brief Core game logic for 2048 puzzle game.
 */
class Game2048Logic {
public:
    using Grid = uint16_t[GRID_TOTAL_CELLS];

    Game2048Logic();

    // Game state accessors
    const Grid& getGrid() const { return grid; }
    int getScore() const { return score; }
    int getHighestTile() const { return highestTile; }
    bool isGameOver() const { return gameOver; }
    bool hasWon() const { return won; }
    bool moved() const { return movedThisTurn; }

    // Game actions
    void reset();
    void spawnTile();
    bool moveLeft();
    bool moveRight();
    bool moveUp();
    bool moveDown();
    void clearMovedFlag() { movedThisTurn = false; }
    void checkGameOver();

    // Win condition check
    static constexpr uint16_t WINNING_TILE = 2048;

private:
    Grid grid;
    int score = 0;
    int highestTile = 0;
    bool gameOver = false;
    bool won = false;
    bool movedThisTurn = false;

    // Row operations
    void compressRow(uint16_t* row);
    void mergeRow(uint16_t* row);
    void secondCompressRow(uint16_t* row);
    bool slideAndMergeRow(uint16_t* row);

    // Rotation helpers
    void rotateGridLeft();
    void rotateGridRight();

    // Game state checks
    bool canMerge(const Grid& g) const;
    bool hasEmptyCells() const;
};

} // namespace game2048