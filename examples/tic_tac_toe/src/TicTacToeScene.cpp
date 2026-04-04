#include "TicTacToeScene.h"
#include <core/Engine.h>
#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>
#include <input/TouchEventTypes.h>
#include <math/MathUtil.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#if PIXELROOT32_ENABLE_UI_SYSTEM
#include <graphics/ui/UITouchButton.h>
#endif

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace tictactoe {

namespace gfx = pr32::graphics;
namespace audio = pr32::audio;
namespace math = pr32::math;
namespace input = pr32::input;

using Color = gfx::Color;
using AudioEvent = audio::AudioEvent;
using WaveType = audio::WaveType;
using Note = audio::Note;
using MusicNote = audio::MusicNote;
using MusicTrack = audio::MusicTrack;

static constexpr float kDefaultAiErrorChance = 0.25f;

#if PIXELROOT32_ENABLE_UI_SYSTEM
// Static member for callback target
TicTacToeScene* TicTacToeScene::sResetButtonTarget = nullptr;

namespace {
// Callback for reset button click
static void onResetButtonClick() {
    if (TicTacToeScene::sResetButtonTarget != nullptr) {
        TicTacToeScene::sResetButtonTarget->resetGame();
    }
}
}
#endif

static const MusicNote BG_MELODY[] = {
    audio::makeNote(audio::INSTR_TRIANGLE_PAD, Note::C, 0.6f),
    audio::makeNote(audio::INSTR_TRIANGLE_PAD, Note::G, 0.6f),
    audio::makeNote(audio::INSTR_TRIANGLE_PAD, Note::E, 0.6f),
    audio::makeRest(0.3f),
    audio::makeNote(audio::INSTR_TRIANGLE_PAD, Note::D, 0.6f),
    audio::makeNote(audio::INSTR_TRIANGLE_PAD, Note::A, 0.6f),
    audio::makeNote(audio::INSTR_TRIANGLE_PAD, Note::F, 0.6f),
    audio::makeRest(0.4f)
};

static const MusicTrack BG_MUSIC = {
    BG_MELODY,
    sizeof(BG_MELODY) / sizeof(MusicNote),
    true,
    WaveType::TRIANGLE,
    0.5f
};

static const MusicNote WIN_MELODY[] = {
    audio::makeNote(audio::INSTR_PULSE_LEAD, Note::C, 0.18f),
    audio::makeNote(audio::INSTR_PULSE_LEAD, Note::E, 0.18f),
    audio::makeNote(audio::INSTR_PULSE_LEAD, Note::G, 0.30f)
};

static const MusicTrack WIN_MUSIC = {
    WIN_MELODY,
    sizeof(WIN_MELODY) / sizeof(MusicNote),
    false,
    WaveType::PULSE,
    0.5f
};

// Custom Neon Palette for TicTacToe
// We use a custom palette to give the game a "Cyberpunk" look.
// This demonstrates how to use setCustomPalette().
static const uint16_t CUSTOM_NEON_PALETTE[16] = {
    0x0000, // 0: Black (Background)
    0xFFFF, // 1: White (Text)
    0x0010, // 2: Navy
    0x07FF, // 3: Blue (LightBlue maps here) -> Neon Cyan (O Color)
    0x07E0, // 4: Cyan
    0x2104, // 5: DarkGreen (Olive maps here) -> Dark Gray (Grid)
    0x07E0, // 6: Green
    0x7FE0, // 7: LightGreen
    0xFFE0, // 8: Yellow (Gold maps here) -> Neon Yellow (Border/Cursor)
    0xFD20, // 9: Orange
    0xF81F, // 10: LightRed -> Neon Pink (X Color)
    0xF800, // 11: Red
    0x8000, // 12: DarkRed
    0x8010, // 13: Purple
    0xF81F, // 14: Magenta
    0x8410  // 15: Gray (LightGray maps here) -> Gray (Instructions)
};

void TicTacToeScene::init() {
    gfx::setCustomPalette(CUSTOM_NEON_PALETTE);
    
    std::snprintf(statusText, sizeof(statusText), "Player X Turn");
    std::snprintf(instructionsText, sizeof(instructionsText), "DPAD/Touch: Move | A/Click: Select");
    instructionsVisible = true;

    resetGame();

    engine.getMusicPlayer().play(BG_MUSIC);

    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

#if PIXELROOT32_ENABLE_UI_SYSTEM
    initUI();
#endif
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
    cursorIndex = 4;
    gameOver = false;
    gameEndTime = 0;
    std::snprintf(statusText, sizeof(statusText), "Player X Turn");
    std::snprintf(instructionsText, sizeof(instructionsText), "DPAD/Touch: Move | A/Click: Select");
    instructionsVisible = true;
    
    // Center the board on screen
    // Board size = BOARD_SIZE * CELL_SIZE = 3 * 50 = 150 pixels
    int boardSize = BOARD_SIZE * CELL_SIZE;
    int boardX = (DISPLAY_WIDTH - boardSize) / 2;
    int boardY = (DISPLAY_HEIGHT - boardSize) / 2;  
    boardPosition = math::Vector2(math::toScalar(boardX), math::toScalar(boardY));
}

void TicTacToeScene::update(unsigned long deltaTime) {
    handleInput();
    
#if PIXELROOT32_ENABLE_UI_SYSTEM
    updateResetButtonVisibility();
#endif
    
    Scene::update(deltaTime);
}

void TicTacToeScene::handleInput() {
    auto& input = engine.getInputManager();
    auto& audio = engine.getAudioEngine();

    if (gameOver) {
        if (millis() - gameEndTime < 500) {
            return;
        }
        if (input.isButtonPressed(BTN_SELECT)) {
            engine.getMusicPlayer().play(BG_MUSIC);
            resetGame();
        }
        return;
    }

    if (!inputReady) {
        if (!input.isButtonDown(BTN_SELECT) &&
            !input.isButtonDown(BTN_NEXT) &&
            !input.isButtonDown(BTN_PREV) &&
            !input.isButtonDown(BTN_UP) &&
            !input.isButtonDown(BTN_DOWN)) {
            inputReady = true;
        } else {
            return;
        }
    }

    bool leftDown = input.isButtonDown(BTN_PREV);
    bool rightDown = input.isButtonDown(BTN_NEXT);
    bool upDown = input.isButtonDown(BTN_UP);
    bool downDown = input.isButtonDown(BTN_DOWN);

    static bool wasLeftDown = false;
    static bool wasRightDown = false;
    static bool wasUpDown = false;
    static bool wasDownDown = false;

    if (leftDown && !wasLeftDown) {
        int row = cursorIndex / BOARD_SIZE;
        int col = cursorIndex % BOARD_SIZE;
        col--;
        if (col < 0) col = BOARD_SIZE - 1;
        cursorIndex = row * BOARD_SIZE + col;
    }
    wasLeftDown = leftDown;

    if (rightDown && !wasRightDown) {
        int row = cursorIndex / BOARD_SIZE;
        int col = cursorIndex % BOARD_SIZE;
        col++;
        if (col >= BOARD_SIZE) col = 0;
        cursorIndex = row * BOARD_SIZE + col;
    }
    wasRightDown = rightDown;

    if (upDown && !wasUpDown) {
        int row = cursorIndex / BOARD_SIZE;
        int col = cursorIndex % BOARD_SIZE;
        row--;
        if (row < 0) row = BOARD_SIZE - 1;
        cursorIndex = row * BOARD_SIZE + col;
    }
    wasUpDown = upDown;

    if (downDown && !wasDownDown) {
        int row = cursorIndex / BOARD_SIZE;
        int col = cursorIndex % BOARD_SIZE;
        row++;
        if (row >= BOARD_SIZE) row = 0;
        cursorIndex = row * BOARD_SIZE + col;
    }
    wasDownDown = downDown;

    if (currentPlayer != humanPlayer) {
        return;
    }

    if (input.isButtonPressed(BTN_SELECT)) {
        int row = cursorIndex / BOARD_SIZE;
        int col = cursorIndex % BOARD_SIZE;

        if (board[row][col] == Player::None) {
            board[row][col] = humanPlayer;
            AudioEvent placeEv{};
            placeEv.type = WaveType::PULSE;
            placeEv.frequency = 900.0f;
            placeEv.duration = 0.08f;
            placeEv.volume = 0.6f;
            placeEv.duty = 0.5f;
            audio.playEvent(placeEv);
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
    auto& audio = engine.getAudioEngine();

    if (!computeAIMove(row, col)) {
        currentPlayer = humanPlayer;
        return;
    }
    board[row][col] = aiPlayer;
    AudioEvent placeEv{};
    placeEv.type = WaveType::PULSE;
    placeEv.frequency = 750.0f;
    placeEv.duration = 0.08f;
    placeEv.volume = 0.5f;
    placeEv.duty = 0.5f;
    audio.playEvent(placeEv);
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
        std::snprintf(statusText, sizeof(statusText), "Player X Turn");
    } else {
        std::snprintf(statusText, sizeof(statusText), "Player O Turn");
    }
}

void TicTacToeScene::checkWinCondition() {
    bool won = false;
    Player winner = Player::None;

    auto check3 = [&](Player p1, Player p2, Player p3) {
        return (p1 != Player::None && p1 == p2 && p2 == p3);
    };

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
        auto& audio = engine.getAudioEngine();
        gameOver = true;
        gameEndTime = millis();
        gameState = (winner == Player::X) ? GameState::WinX : GameState::WinO;

        if (winner == humanPlayer) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "WINNER: PLAYER %c!", (winner == Player::X) ? 'X' : 'O');
            std::snprintf(statusText, sizeof(statusText), "%s", buf);
        } else {
            std::snprintf(statusText, sizeof(statusText), "YOU LOSE");
        }

        std::snprintf(instructionsText, sizeof(instructionsText), "Press A to Reset");
        instructionsVisible = true;

        engine.getMusicPlayer().stop();
        if (winner == humanPlayer) {
            engine.getMusicPlayer().play(WIN_MUSIC);
        } else {
            AudioEvent loseEv{};
            loseEv.type = WaveType::NOISE;
            loseEv.frequency = 600.0f;
            loseEv.duration = 0.4f;
            loseEv.volume = 0.7f;
            loseEv.duty = 0.5f;
            audio.playEvent(loseEv);
        }
    } else if (isBoardFull()) {
        auto& audio = engine.getAudioEngine();
        gameOver = true;
        gameEndTime = millis();
        gameState = GameState::Draw;
        std::snprintf(statusText, sizeof(statusText), "DRAW GAME!");
        std::snprintf(instructionsText, sizeof(instructionsText), "Press A to Reset");
        instructionsVisible = true;

        engine.getMusicPlayer().stop();

        pr32::audio::AudioEvent loseEv{};
        loseEv.type = pr32::audio::WaveType::NOISE;
        loseEv.frequency = 600.0f;
        loseEv.duration = 0.4f;
        loseEv.volume = 0.7f;
        loseEv.duty = 0.5f;
        audio.playEvent(loseEv);
    }
}

bool TicTacToeScene::isBoardFull() {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (board[i][j] == Player::None) return false;
    return true;
}

void TicTacToeScene::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, Color::Black);

    renderer.drawTextCentered(statusText, 10, Color::White, 1);
    if (instructionsVisible) {
        renderer.drawTextCentered(instructionsText, DISPLAY_HEIGHT - 20, Color::LightGray, 1);
    }

    drawGrid(renderer);
    drawMarks(renderer);
    drawCursor(renderer);

    Scene::draw(renderer);
}

void TicTacToeScene::drawGrid(gfx::Renderer& renderer) {
    int startX = static_cast<int>(boardPosition.x);
    int startY = static_cast<int>(boardPosition.y);
    int fullSize = BOARD_SIZE * CELL_SIZE;
    Color gridColor = Color::Olive;
    Color borderColor = Color::Gold;

    for (int i = 1; i < BOARD_SIZE; ++i) {
        int x = startX + i * CELL_SIZE;
        renderer.drawLine(x, startY, x, startY + fullSize, gridColor);
    }

    for (int i = 1; i < BOARD_SIZE; ++i) {
        int y = startY + i * CELL_SIZE;
        renderer.drawLine(startX, y, startX + fullSize, y, gridColor);
    }

    renderer.drawRectangle(startX - 2, startY - 2, fullSize + 4, fullSize + 4, borderColor);
}

void TicTacToeScene::drawCursor(gfx::Renderer& renderer) {
    int row = cursorIndex / BOARD_SIZE;
    int col = cursorIndex % BOARD_SIZE;
    
    int x = static_cast<int>(boardPosition.x) + col * CELL_SIZE;
    int y = static_cast<int>(boardPosition.y) + row * CELL_SIZE;

    renderer.drawRectangle(x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4, Color::Yellow);
}

void TicTacToeScene::drawMarks(gfx::Renderer& renderer) {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            int x = static_cast<int>(boardPosition.x) + j * CELL_SIZE;
            int y = static_cast<int>(boardPosition.y) + i * CELL_SIZE;

            if (board[i][j] == Player::X) {
                drawX(renderer, x, y);
            } else if (board[i][j] == Player::O) {
                drawO(renderer, x, y);
            }
        }
    }
}

void TicTacToeScene::drawX(gfx::Renderer& renderer, int x, int y) {
    int padding = 8;
    int x1 = x + padding;
    int y1 = y + padding;
    int x2 = x + CELL_SIZE - padding;
    int y2 = y + CELL_SIZE - padding;
    Color xColor = Color::LightRed;

    renderer.drawLine(x1, y1, x2, y2, xColor);
    renderer.drawLine(x1 + 1, y1, x2 + 1, y2, xColor);
    renderer.drawLine(x1, y1 + 1, x2, y2 + 1, xColor);

    renderer.drawLine(x2, y1, x1, y2, xColor);
    renderer.drawLine(x2 - 1, y1, x1 - 1, y2, xColor);
    renderer.drawLine(x2, y1 + 1, x1, y2 + 1, xColor);
}

void TicTacToeScene::drawO(gfx::Renderer& renderer, int x, int y) {
    int padding = 8;
    int size = CELL_SIZE - padding * 2;
    int ox = x + padding;
    int oy = y + padding;
    Color oColor = Color::LightBlue;

    renderer.drawRectangle(ox, oy, size, size, oColor);
    renderer.drawRectangle(ox + 1, oy + 1, size - 2, size - 2, oColor);
}

bool TicTacToeScene::touchToCell(int16_t touchX, int16_t touchY, int& outRow, int& outCol) const {
    int boardStartX = static_cast<int>(boardPosition.x);
    int boardStartY = static_cast<int>(boardPosition.y);
    int fullSize = BOARD_SIZE * CELL_SIZE;

    // Apply hit slop padding - check if touch is anywhere near the board
    int paddedStartX = boardStartX - kTouchHitSlop;
    int paddedStartY = boardStartY - kTouchHitSlop;
    int paddedFullSize = fullSize + kTouchHitSlop * 2;

    if (touchX < paddedStartX || touchX >= paddedStartX + paddedFullSize ||
        touchY < paddedStartY || touchY >= paddedStartY + paddedFullSize) {
        return false;
    }

    // Calculate the cell position
    int relX = touchX - boardStartX;
    int relY = touchY - boardStartY;

    // Clamp to board bounds
    if (relX < 0 || relX >= fullSize || relY < 0 || relY >= fullSize) {
        return false;
    }

    outCol = relX / CELL_SIZE;
    outRow = relY / CELL_SIZE;

    return (outRow >= 0 && outRow < BOARD_SIZE && outCol >= 0 && outCol < BOARD_SIZE);
}

bool TicTacToeScene::placeMark(int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }
    if (board[row][col] != Player::None) {
        return false;
    }
    if (gameOver || currentPlayer != humanPlayer) {
        return false;
    }

    board[row][col] = humanPlayer;

    auto& audio = engine.getAudioEngine();
    AudioEvent placeEv{};
    placeEv.type = WaveType::PULSE;
    placeEv.frequency = 900.0f;
    placeEv.duration = 0.08f;
    placeEv.volume = 0.6f;
    placeEv.duty = 0.5f;
    audio.playEvent(placeEv);

    checkWinCondition();
    if (!gameOver) {
        currentPlayer = aiPlayer;
        performAIMove();
    }

    return true;
}

void TicTacToeScene::onUnconsumedTouchEvent(const input::TouchEvent& event) {
    using T = input::TouchEventType;

    // Only handle Click events for cell selection
    // Ignore drag events (player can use D-pad for movement)
    auto ty = event.getType();
    if (ty != T::Click && ty != T::TouchUp) {
        return;
    }

    int row, col;
    if (touchToCell(event.x, event.y, row, col)) {
        placeMark(row, col);
    }
}

#if PIXELROOT32_ENABLE_UI_SYSTEM
void TicTacToeScene::initUI() {
    auto& ui = getUIManager();
    ui.clear();
    resetTouchButton.reset();

    auto& renderer = engine.getRenderer();
    const int sw = renderer.getLogicalWidth();
    const int sh = renderer.getLogicalHeight();

    // Position button at bottom center, above instructions
    constexpr int btnW = 100;
    constexpr int btnH = 32;
    constexpr int margin = 6;
    const int btnX = (sw - btnW) / 2;
    const int btnY = sh - btnH - margin - 20; // Above instructions text

    resetTouchButton = std::make_unique<gfx::ui::UITouchButton>("Play Again", btnX, btnY, btnW, btnH);

    if (resetTouchButton) {
        resetTouchButton->setColors(Color::Navy, Color::LightBlue, Color::DarkGray);
        resetTouchButton->setOnClick(onResetButtonClick);

        // Initially hidden - only visible when game is over
        resetTouchButton->setVisible(false);
        ui.addElement(resetTouchButton.get());
        addEntity(resetTouchButton.get());
    }

    // Set callback target
    sResetButtonTarget = this;
}

void TicTacToeScene::updateResetButtonVisibility() {
    if (resetTouchButton) {
        resetTouchButton->setVisible(gameOver);
    }
}
#endif

}
