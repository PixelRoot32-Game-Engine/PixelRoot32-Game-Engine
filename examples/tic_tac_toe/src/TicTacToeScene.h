#pragma once
#include <core/Scene.h>
#include <platforms/EngineConfig.h>
#include <input/TouchEvent.h>
#include "GameConstants.h"

#include <graphics/ui/UILabel.h>

#if PIXELROOT32_ENABLE_TOUCH
#include <graphics/ui/UITouchButton.h>
#else
#include <graphics/ui/UIButton.h>
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

    /**
     * @brief Reset the game state - called by GPIO button and touch button.
     */
    void resetGame();

    /**
     * @brief Handle unconsumed touch events for board cell selection.
     * Maps touch coordinates to board cells and places the current player's mark.
     */
    void onUnconsumedTouchEvent(const pixelroot32::input::TouchEvent& event) override;

    /** Reset button target for callback - needed because C-style callbacks can't use member functions directly. */
    static TicTacToeScene* sResetButtonTarget;

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

    /** Touch hit slop in pixels (padding around each cell for easier touching). */
    static constexpr int16_t kTouchHitSlop = 8;

    // UI Elements - always use UILabel for text
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> statusLabel;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> instructionsLabel;

    // Reset button - type depends on TOUCH_ENABLED
#if PIXELROOT32_ENABLE_TOUCH
    std::unique_ptr<pixelroot32::graphics::ui::UITouchButton> resetTouchButton;
#else
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> resetButton;
#endif

    /**
     * @brief Create UI labels for status and instructions text.
     */
    void createLabels();

    /**
     * @brief Create reset button (Play Again).
     */
    void createResetButton();

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