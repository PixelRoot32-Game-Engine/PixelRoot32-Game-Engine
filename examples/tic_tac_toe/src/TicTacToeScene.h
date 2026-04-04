#pragma once
#include <core/Scene.h>
#include <platforms/EngineConfig.h>
#include <input/TouchEvent.h>
#include "GameConstants.h"

#if PIXELROOT32_ENABLE_UI_SYSTEM
#include <graphics/ui/UITouchButton.h>
#endif

namespace tictactoe {

/** Cell ownership */
enum class Player {
    None,
    X,
    O
};

enum class GameState {
    Playing,
    WinX,
    WinO,
    Draw
};

/**
 * @class TicTacToeScene
 * @brief Tic-tac-toe with custom palette and AI opponent.
 *
 * Uses cursor-based input and minimax-style AI with configurable error chance.
 * Supports both GPIO buttons (DPAD + A) and touch input on ESP32-CYD.
 */
class TicTacToeScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

#if PIXELROOT32_ENABLE_UI_SYSTEM
    void initUI() override;
#endif

    /**
     * @brief Reset the game state - called by GPIO button and touch button.
     */
    void resetGame();

    /**
     * @brief Handle unconsumed touch events for board cell selection.
     * Maps touch coordinates to board cells and places the current player's mark.
     */
    void onUnconsumedTouchEvent(const pixelroot32::input::TouchEvent& event) override;

#if PIXELROOT32_ENABLE_UI_SYSTEM
    /** Reset button target for callback - needed because C-style callbacks can't use member functions directly. */
    static TicTacToeScene* sResetButtonTarget;
#endif

private:
    Player board[BOARD_SIZE][BOARD_SIZE];
    Player currentPlayer;
    Player humanPlayer;
    Player aiPlayer;
    float aiErrorChance;
    bool inputReady;
    GameState gameState;
    int cursorIndex;  ///< 0-8, maps to board[row][col]
    bool gameOver;
    unsigned long gameEndTime;

    pixelroot32::math::Vector2 boardPosition;
    char statusText[32];
    char instructionsText[64];
    bool instructionsVisible;

    /** Touch hit slop in pixels (padding around each cell for easier touching). */
    static constexpr int16_t kTouchHitSlop = 8;

#if PIXELROOT32_ENABLE_UI_SYSTEM
    /** UI reset button - only visible when game is over. */
    std::unique_ptr<pixelroot32::graphics::ui::UITouchButton> resetTouchButton;
#endif

    /**
     * @brief Convert touch coordinates to board cell indices.
     * @param touchX Touch X coordinate
     * @param touchY Touch Y coordinate
     * @param outRow Output row index (0-2)
     * @param outCol Output column index (0-2)
     * @return true if touch is within board bounds, false otherwise
     */
    bool touchToCell(int16_t touchX, int16_t touchY, int& outRow, int& outCol) const;

    /**
     * @brief Place a mark on the board at the specified cell.
     * @param row Row index (0-2)
     * @param col Column index (0-2)
     * @return true if placement succeeded (cell was empty), false otherwise
     */
    bool placeMark(int row, int col);

    /**
     * @brief Update reset button visibility based on game state.
     */
    void updateResetButtonVisibility();

    void handleInput();
    void performAIMove();
    bool computeAIMove(int& outRow, int& outCol);
    bool wouldWin(Player player) const;
    void checkWinCondition();
    bool isBoardFull();
    void nextTurn();

    void drawGrid(pixelroot32::graphics::Renderer& renderer);
    void drawMarks(pixelroot32::graphics::Renderer& renderer);
    void drawCursor(pixelroot32::graphics::Renderer& renderer);
    void drawX(pixelroot32::graphics::Renderer& renderer, int x, int y);
    void drawO(pixelroot32::graphics::Renderer& renderer, int x, int y);
};

} // namespace tictactoe
