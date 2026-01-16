#pragma once

#include "core/Scene.h"
#include "graphics/ui/UILabel.h"
#include "Config.h"
#include "GameConstants.h"

namespace tictactoe {

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

class TicTacToeScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    // Game Logic
    Player board[BOARD_SIZE][BOARD_SIZE];
    Player currentPlayer;
    Player humanPlayer;
    Player aiPlayer;
    float aiErrorChance;
    bool inputReady;
    GameState gameState;
    int cursorIndex; // 0-8, mapping to board[row][col]
    bool gameOver;
    unsigned long gameEndTime;
    
    // Layout
    int boardXOffset;

    // UI Elements
    pixelroot32::graphics::ui::UILabel* lblStatus;
    pixelroot32::graphics::ui::UILabel* lblInstructions;

    // Helper methods
    void resetGame();
    void handleInput();
    void performAIMove();
    bool computeAIMove(int& outRow, int& outCol);
    bool wouldWin(Player player) const;
    void checkWinCondition();
    bool isBoardFull();
    void nextTurn();
    
    // Drawing helpers
    void drawGrid(pixelroot32::graphics::Renderer& renderer);
    void drawMarks(pixelroot32::graphics::Renderer& renderer);
    void drawCursor(pixelroot32::graphics::Renderer& renderer);
    void drawX(pixelroot32::graphics::Renderer& renderer, int x, int y);
    void drawO(pixelroot32::graphics::Renderer& renderer, int x, int y);
};

}
