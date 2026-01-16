#include "TicTacToeScene.h"
#include "core/Engine.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace pr32 = pixelroot32;

// Access to the global engine instance
extern pr32::core::Engine engine;

namespace tictactoe {

using Color = pr32::graphics::Color;
static constexpr float kDefaultAiErrorChance = 0.25f;

void TicTacToeScene::init() {
    int screenWidth = engine.getRenderer().getWidth();

    // Initialize UI
    lblStatus = new pr32::graphics::ui::UILabel("Player X Turn", 0, 10, Color::White, 1);
    lblStatus->centerX(screenWidth);
    
    lblInstructions = new pr32::graphics::ui::UILabel("BTN 2/1: Move | BTN 0: Select", 0, DISPLAY_HEIGHT - 20, Color::LightGray, 1);
    lblInstructions->centerX(screenWidth);

    addEntity(lblStatus);
    addEntity(lblInstructions);

    resetGame();

    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
}

void TicTacToeScene::resetGame() {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            board[i][j] = Player::None;
        }
    }
    humanPlayer = Player::X;
    aiPlayer = Player::O;
    aiErrorChance = kDefaultAiErrorChance;
    currentPlayer = humanPlayer;
    inputReady = false;
    gameState = GameState::Playing;
    cursorIndex = 4; // Start in middle (1, 1)
    gameOver = false;
    gameEndTime = 0;
    lblStatus->setText("Player X Turn");
    lblInstructions->setVisible(true);
}

void TicTacToeScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime);
    handleInput();
}

void TicTacToeScene::handleInput() {
    auto& input = engine.getInputManager();

    if (gameOver) {
        if (millis() - gameEndTime < 500) {
            return;
        }
        if (input.isButtonPressed(0)) {
            resetGame();
        }
        return;
    }

    if (!inputReady) {
        if (!engine.getInputManager().isButtonDown(0) &&
            !engine.getInputManager().isButtonDown(1) &&
            !engine.getInputManager().isButtonDown(2)) {
            inputReady = true;
        } else {
            return;
        }
    }

    bool leftDown = input.isButtonDown(2);
    bool rightDown = input.isButtonDown(1);

    static bool wasLeftDown = false;
    static bool wasRightDown = false;

    if (leftDown && !wasLeftDown) {
        cursorIndex--;
        if (cursorIndex < 0) cursorIndex = 8;
    }
    wasLeftDown = leftDown;

    if (rightDown && !wasRightDown) {
        cursorIndex++;
        if (cursorIndex > 8) cursorIndex = 0;
    }
    wasRightDown = rightDown;

    if (currentPlayer != humanPlayer) {
        return;
    }

    if (input.isButtonPressed(0)) {
        int row = cursorIndex / BOARD_SIZE;
        int col = cursorIndex % BOARD_SIZE;

        if (board[row][col] == Player::None) {
            board[row][col] = humanPlayer;
            checkWinCondition();
            if (!gameOver) {
                currentPlayer = aiPlayer;
                performAIMove();
            }
        }
    }
}

void TicTacToeScene::performAIMove() {
    int row = -1;
    int col = -1;
    if (!computeAIMove(row, col)) {
        currentPlayer = humanPlayer;
        return;
    }
    board[row][col] = aiPlayer;
    checkWinCondition();
    if (!gameOver) {
        currentPlayer = humanPlayer;
    }
}

bool TicTacToeScene::wouldWin(Player player) const {
    auto same = [&](Player a, Player b, Player c) {
        return a == player && b == player && c == player;
    };

    for (int i = 0; i < 3; ++i) {
        if (same(board[i][0], board[i][1], board[i][2])) return true;
        if (same(board[0][i], board[1][i], board[2][i])) return true;
    }

    if (same(board[0][0], board[1][1], board[2][2])) return true;
    if (same(board[0][2], board[1][1], board[2][0])) return true;

    return false;
}

bool TicTacToeScene::computeAIMove(int& outRow, int& outCol) {
    int emptyRows[9];
    int emptyCols[9];
    int emptyCount = 0;

    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (board[r][c] == Player::None) {
                emptyRows[emptyCount] = r;
                emptyCols[emptyCount] = c;
                emptyCount++;
            }
        }
    }

    if (emptyCount == 0) {
        return false;
    }

    int bestRow = -1;
    int bestCol = -1;

    for (int i = 0; i < emptyCount; ++i) {
        int r = emptyRows[i];
        int c = emptyCols[i];
        board[r][c] = aiPlayer;
        bool win = wouldWin(aiPlayer);
        board[r][c] = Player::None;
        if (win) {
            bestRow = r;
            bestCol = c;
            break;
        }
    }

    if (bestRow == -1) {
        for (int i = 0; i < emptyCount; ++i) {
            int r = emptyRows[i];
            int c = emptyCols[i];
            board[r][c] = humanPlayer;
            bool humanWin = wouldWin(humanPlayer);
            board[r][c] = Player::None;
            if (humanWin) {
                bestRow = r;
                bestCol = c;
                break;
            }
        }
    }

    if (bestRow == -1) {
        if (board[1][1] == Player::None) {
            bestRow = 1;
            bestCol = 1;
        } else {
            int cornerCandidates[4][2] = {{0,0},{0,2},{2,0},{2,2}};
            for (int i = 0; i < 4; ++i) {
                int r = cornerCandidates[i][0];
                int c = cornerCandidates[i][1];
                if (board[r][c] == Player::None) {
                    bestRow = r;
                    bestCol = c;
                    break;
                }
            }
            if (bestRow == -1) {
                for (int i = 0; i < emptyCount; ++i) {
                    int r = emptyRows[i];
                    int c = emptyCols[i];
                    if (board[r][c] == Player::None) {
                        bestRow = r;
                        bestCol = c;
                        break;
                    }
                }
            }
        }
    }

    if (bestRow == -1) {
        return false;
    }

    int bestIndex = -1;
    for (int i = 0; i < emptyCount; ++i) {
        if (emptyRows[i] == bestRow && emptyCols[i] == bestCol) {
            bestIndex = i;
            break;
        }
    }

    float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    bool makeError = r < aiErrorChance && emptyCount > 1;

    if (makeError && bestIndex != -1) {
        int choice = bestIndex;
        while (choice == bestIndex) {
            choice = std::rand() % emptyCount;
        }
        outRow = emptyRows[choice];
        outCol = emptyCols[choice];
    } else {
        outRow = bestRow;
        outCol = bestCol;
    }

    return true;
}

void TicTacToeScene::nextTurn() {
    currentPlayer = (currentPlayer == Player::X) ? Player::O : Player::X;
    if (currentPlayer == Player::X) {
        lblStatus->setText("Player X Turn");
    } else {
        lblStatus->setText("Player O Turn");
    }
}

void TicTacToeScene::checkWinCondition() {
    // Check Rows, Cols, Diagonals
    bool won = false;
    Player winner = Player::None;

    // Helper lambda to check 3 cells
    auto check3 = [&](Player p1, Player p2, Player p3) {
        return (p1 != Player::None && p1 == p2 && p2 == p3);
    };

    // Rows & Cols
    for (int i = 0; i < 3; ++i) {
        if (check3(board[i][0], board[i][1], board[i][2])) {
            won = true; winner = board[i][0];
        }
        if (check3(board[0][i], board[1][i], board[2][i])) {
            won = true; winner = board[0][i];
        }
    }

    // Diagonals
    if (check3(board[0][0], board[1][1], board[2][2])) {
        won = true; winner = board[0][0];
    }
    if (check3(board[0][2], board[1][1], board[2][0])) {
        won = true; winner = board[0][2];
    }

    if (won) {
        gameOver = true;
        gameEndTime = millis();
        gameState = (winner == Player::X) ? GameState::WinX : GameState::WinO;
        char buf[32];
        snprintf(buf, sizeof(buf), "WINNER: PLAYER %c!", (winner == Player::X) ? 'X' : 'O');
        lblStatus->setText(buf);
        lblInstructions->setText("Press BTN 0 to Reset");
    } else if (isBoardFull()) {
        gameOver = true;
        gameEndTime = millis();
        gameState = GameState::Draw;
        lblStatus->setText("DRAW GAME!");
        lblInstructions->setText("Press BTN 0 to Reset");
    }
}

bool TicTacToeScene::isBoardFull() {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (board[i][j] == Player::None) return false;
    return true;
}

void TicTacToeScene::draw(pr32::graphics::Renderer& renderer) {
    drawGrid(renderer);
    drawMarks(renderer);
    drawCursor(renderer);

    // Draw UI elements via base class
    Scene::draw(renderer);
}

void TicTacToeScene::drawGrid(pr32::graphics::Renderer& renderer) {
    int startX = BOARD_X_OFFSET;
    int startY = BOARD_Y_OFFSET;
    int fullSize = BOARD_SIZE * CELL_SIZE;

    // Vertical lines
    for (int i = 1; i < BOARD_SIZE; ++i) {
        int x = startX + i * CELL_SIZE;
        renderer.drawLine(x, startY, x, startY + fullSize, Color::White);
    }

    // Horizontal lines
    for (int i = 1; i < BOARD_SIZE; ++i) {
        int y = startY + i * CELL_SIZE;
        renderer.drawLine(startX, y, startX + fullSize, y, Color::White);
    }
    
    // Border (Optional, for aesthetics)
    renderer.drawRectangle(startX - 2, startY - 2, fullSize + 4, fullSize + 4, Color::DarkGray);
}

void TicTacToeScene::drawCursor(pr32::graphics::Renderer& renderer) {
    int row = cursorIndex / BOARD_SIZE;
    int col = cursorIndex % BOARD_SIZE;
    
    int x = BOARD_X_OFFSET + col * CELL_SIZE;
    int y = BOARD_Y_OFFSET + row * CELL_SIZE;

    // Draw a blinking selection box or just a highlight
    // Using a simple rectangle inset
    renderer.drawRectangle(x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4, Color::Yellow);
}

void TicTacToeScene::drawMarks(pr32::graphics::Renderer& renderer) {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            int x = BOARD_X_OFFSET + j * CELL_SIZE;
            int y = BOARD_Y_OFFSET + i * CELL_SIZE;

            if (board[i][j] == Player::X) {
                drawX(renderer, x, y);
            } else if (board[i][j] == Player::O) {
                drawO(renderer, x, y);
            }
        }
    }
}

void TicTacToeScene::drawX(pr32::graphics::Renderer& renderer, int x, int y) {
    int padding = 10;
    int x1 = x + padding;
    int y1 = y + padding;
    int x2 = x + CELL_SIZE - padding;
    int y2 = y + CELL_SIZE - padding;

    renderer.drawLine(x1, y1, x2, y2, Color::Red);
    renderer.drawLine(x2, y1, x1, y2, Color::Red);
}

void TicTacToeScene::drawO(pr32::graphics::Renderer& renderer, int x, int y) {
    int padding = 10;
    int radius = (CELL_SIZE / 2) - padding;
    int centerX = x + CELL_SIZE / 2;
    int centerY = y + CELL_SIZE / 2;

    renderer.drawCircle(centerX, centerY, radius, Color::Blue);
}

}
