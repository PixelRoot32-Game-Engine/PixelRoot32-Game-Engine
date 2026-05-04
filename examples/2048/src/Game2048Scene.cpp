#include "Game2048Scene.h"

#include <core/Engine.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#if PIXELROOT32_ENABLE_AUDIO
#include <audio/AudioEngine.h>
#endif

#include "assets/sfx.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace game2048 {

namespace gfx = pr32::graphics;
namespace input = pr32::input;

// Default constructor - initialize swipe state
Game2048Scene::Game2048Scene()
    : lastMoveTime(0)
    , inputReady(true)
    , dragStartX(0)
    , dragStartY(0)
    , dragActive(false)
    , wasGameOver(false)
    , wasWon(false) {
}

// Static member for callback target
Game2048Scene* Game2048Scene::sResetButtonTarget = nullptr;

// Callback for reset button click
static void onResetButtonClickStatic() {
    if (Game2048Scene::sResetButtonTarget != nullptr) {
        Game2048Scene::sResetButtonTarget->resetGame();
    }
}

void Game2048Scene::init() {
    // Set custom 2048 palette
    gfx::setPalette(gfx::PaletteType::PR32);
    
    createLabels();
    createResetButton();
    resetGame();
}

void Game2048Scene::createLabels() {
    auto& renderer = engine.getRenderer();
    const int sw = renderer.getLogicalWidth();

    // Score label at top (white for contrast)
    scoreLabel = std::make_unique<gfx::ui::UILabel>(
        "Score: 0",
        pr32::math::Vector2(pr32::math::toScalar(0), pr32::math::toScalar(16)),
        gfx::Color::White, 2);
    scoreLabel->centerX(sw);
    addEntity(scoreLabel.get());

    // Status label (game over / win) - position in center
    statusLabel = std::make_unique<gfx::ui::UILabel>(
        "",
        pr32::math::Vector2(pr32::math::toScalar(0), pr32::math::toScalar(DISPLAY_HEIGHT / 2 - 30)),
        gfx::Color::Cyan, 2);  // Larger size, cyan color
    statusLabel->centerX(sw);
    addEntity(statusLabel.get());

    // Instruction label (Try again! / Reach 2048) - position below status
    instructionLabel = std::make_unique<gfx::ui::UILabel>(
        "",
        pr32::math::Vector2(pr32::math::toScalar(0), pr32::math::toScalar(DISPLAY_HEIGHT / 2)),
        gfx::Color::LightGray, 1);
    instructionLabel->centerX(sw);
    addEntity(instructionLabel.get());
}

void Game2048Scene::createResetButton() {
    auto& renderer = engine.getRenderer();
    const int sw = renderer.getLogicalWidth();
    const int sh = renderer.getLogicalHeight();

    constexpr int btnW = 80;
    constexpr int btnH = 24;
    const int btnX = (sw - btnW) / 2;
    const int btnY = (sh - btnH) / 2;

    pr32::math::Vector2 pos(pr32::math::toScalar(btnX), pr32::math::toScalar(btnY));
    pr32::math::Vector2 sz(pr32::math::toScalar(btnW), pr32::math::toScalar(btnH));

    sResetButtonTarget = this;

#if PIXELROOT32_ENABLE_TOUCH
    resetTouchButton = std::make_unique<gfx::ui::UITouchButton>("Play Again", pos, sz, onResetButtonClickStatic);
    resetTouchButton->setColors(gfx::Color::Navy, gfx::Color::LightBlue, gfx::Color::DarkGray);
    resetTouchButton->autoSize(4);
    resetTouchButton->setVisible(false);
    engine.getUIManager().addElement(resetTouchButton.get());
    addEntity(resetTouchButton.get());
#else
    resetButton = std::make_unique<gfx::ui::UIButton>("Play Again", 0, pos, sz, onResetButtonClickStatic);
    resetButton->setVisible(false);
    addEntity(resetButton.get());
#endif
}

void Game2048Scene::resetGame() {
    gameLogic.reset();

    // Calculate grid position (centered)
    int gridSize = GRID_SIZE * CELL_SIZE;
    int gridX = (DISPLAY_WIDTH - gridSize) / 2;
    int gridY = (DISPLAY_HEIGHT - gridSize) / 2;
    gridPosition = pr32::math::Vector2(pr32::math::toScalar(gridX), pr32::math::toScalar(gridY));

    inputReady = false;
    lastMoveTime = 0;
    
    // Reset game state flags
    wasGameOver = false;
    wasWon = false;

    updateLabels();

    // Clear status/instruction labels
    statusLabel->setText("");
    instructionLabel->setText("");
    
    // Reset button visibility
#if PIXELROOT32_ENABLE_TOUCH
    if (resetTouchButton) resetTouchButton->setVisible(false);
#else
    if (resetButton) resetButton->setVisible(false);
#endif
}

void Game2048Scene::update(unsigned long deltaTime) {
    handleInput();
    checkGameState();
    Scene::update(deltaTime);
}

void Game2048Scene::handleInput() {
    auto& input = engine.getInputManager();

    // Game over state - wait for select button to reset
    if (gameLogic.isGameOver()) {
        if (input.isButtonPressed(BTN_SELECT)) {
            resetGame();
        }
        return;
    }

    // Cooldown check
    unsigned long now = millis();
    if (now - lastMoveTime < MOVE_COOLDOWN_MS) {
        return;
    }

    // Wait for button release before accepting new input
    if (!inputReady) {
        if (!input.isButtonDown(BTN_UP) &&
            !input.isButtonDown(BTN_DOWN) &&
            !input.isButtonDown(BTN_LEFT) &&
            !input.isButtonDown(BTN_RIGHT)) {
            inputReady = true;
        }
        return;
    }

    // Track previous state for edge detection
    static bool wasUpDown = false;
    static bool wasDownDown = false;
    static bool wasLeftDown = false;
    static bool wasRightDown = false;

    bool upDown = input.isButtonDown(BTN_UP);
    bool downDown = input.isButtonDown(BTN_DOWN);
    bool leftDown = input.isButtonDown(BTN_LEFT);
    bool rightDown = input.isButtonDown(BTN_RIGHT);

    if (upDown && !wasUpDown) {
        int scoreBefore = gameLogic.getScore();
        bool moved = gameLogic.moveUp();
        doMove(moved, scoreBefore);
    } else if (downDown && !wasDownDown) {
        int scoreBefore = gameLogic.getScore();
        bool moved = gameLogic.moveDown();
        doMove(moved, scoreBefore);
    } else if (leftDown && !wasLeftDown) {
        int scoreBefore = gameLogic.getScore();
        bool moved = gameLogic.moveLeft();
        doMove(moved, scoreBefore);
    } else if (rightDown && !wasRightDown) {
        int scoreBefore = gameLogic.getScore();
        bool moved = gameLogic.moveRight();
        doMove(moved, scoreBefore);
    }

    wasUpDown = upDown;
    wasDownDown = downDown;
    wasLeftDown = leftDown;
    wasRightDown = rightDown;
}

void Game2048Scene::checkGameState() {
    auto& renderer = engine.getRenderer();
    const int sw = renderer.getLogicalWidth();
    
    // Check for win - use hasWon() which is set when a 2048 tile is created
    bool hasWonTheGame = gameLogic.hasWon();
    
    if (hasWonTheGame && !wasWon && !gameLogic.isGameOver()) {
        // Player just won!
#if PIXELROOT32_ENABLE_AUDIO
        auto& audio = engine.getAudioEngine();
        playWinSound(audio);
#endif
        wasWon = true;
    }
    
    if (hasWonTheGame && !gameLogic.isGameOver()) {
        // Player won!
        statusLabel->setText("YOU WIN!");
        statusLabel->centerX(sw);
        instructionLabel->setText("Keep Playing!");
        instructionLabel->centerX(sw);
        
#if PIXELROOT32_ENABLE_TOUCH
        if (resetTouchButton) resetTouchButton->setVisible(true);
#else
        if (resetButton) resetButton->setVisible(true);
#endif
    }
    // Check for game over
    else if (gameLogic.isGameOver() && !wasGameOver) {
#if PIXELROOT32_ENABLE_AUDIO
        auto& audio = engine.getAudioEngine();
        playGameOverSound(audio);
#endif
        wasGameOver = true;
        
        statusLabel->setText("GAME OVER!");
        statusLabel->centerX(sw);
        instructionLabel->setText("Try Again!");
        instructionLabel->centerX(sw);
        
#if PIXELROOT32_ENABLE_TOUCH
        if (resetTouchButton) resetTouchButton->setVisible(true);
#else
        if (resetButton) resetButton->setVisible(true);
#endif
    } else {
        // Clear labels when playing
        statusLabel->setText("");
        instructionLabel->setText("");
        
#if PIXELROOT32_ENABLE_TOUCH
        if (resetTouchButton) resetTouchButton->setVisible(false);
#else
        if (resetButton) resetButton->setVisible(false);
#endif
    }
}

void Game2048Scene::updateLabels() {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Score: %d", gameLogic.getScore());
    scoreLabel->setText(buf);
}

void Game2048Scene::draw(pixelroot32::graphics::Renderer& renderer) {
    // Clear screen (use palette color index 0 = background)
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx::Color::Black);

    drawGrid(renderer);
    drawTiles(renderer);

    Scene::draw(renderer);
}

void Game2048Scene::drawGrid(pixelroot32::graphics::Renderer& renderer) {
    int startX = static_cast<int>(gridPosition.x);
    int startY = static_cast<int>(gridPosition.y);
    int gridPixelSize = GRID_SIZE * CELL_SIZE;

    // Draw background cells
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            int cellX = startX + col * CELL_SIZE + TILE_SPACING;
            int cellY = startY + row * CELL_SIZE + TILE_SPACING;
            int cellSize = CELL_SIZE - TILE_SPACING * 2;

            // Use DarkGray for empty cells (better contrast)
            renderer.drawFilledRectangle(cellX, cellY, cellSize, cellSize, gfx::Color::DarkGray);
        }
    }

    // Draw grid border
    renderer.drawRectangle(startX - 2, startY - 2, gridPixelSize + 4, gridPixelSize + 4, gfx::Color::Gray);
}

void Game2048Scene::drawTiles(pixelroot32::graphics::Renderer& renderer) {
    const auto& grid = gameLogic.getGrid();

    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            uint16_t value = grid[row * GRID_SIZE + col];
            if (value != EMPTY_TILE) {
                drawTile(renderer, row, col, value);
            }
        }
    }
}

void Game2048Scene::drawTile(pixelroot32::graphics::Renderer& renderer, int row, int col, uint16_t value) {
    int startX = static_cast<int>(gridPosition.x);
    int startY = static_cast<int>(gridPosition.y);

    int tileX = startX + col * CELL_SIZE + TILE_SPACING;
    int tileY = startY + row * CELL_SIZE + TILE_SPACING;
    int tileSize = CELL_SIZE - TILE_SPACING * 2;

    // Draw tile background
    gfx::Color tileColor = getTileColor(value);
    renderer.drawFilledRectangle(tileX, tileY, tileSize, tileSize, tileColor);

    // Draw tile value as text (if not too large)
    if (value <= MAX_DISPLAYED_TILE) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%u", value);

        // Calculate text color - use contrast based on background
        // Light backgrounds (2,4) -> dark text (DarkBlue/Navy)
        // Dark backgrounds (8+) -> white text
        gfx::Color textColor = (value <= 4) ? gfx::Color::Navy : gfx::Color::White;

        // Center text in tile
        int textW = static_cast<int>(std::strlen(buf)) * 6;
        int textX = tileX + (tileSize - textW) / 2;
        int textY = tileY + (tileSize - 8) / 2;

        renderer.drawText(buf, textX, textY, textColor, 1);
    }
}

gfx::Color Game2048Scene::getTileColor(uint16_t value) const {
    // PR32 palette - prioritize contrast
    switch (value) {
        case 2:    return gfx::Color::LightGray;  // cream/white
        case 4:    return gfx::Color::White;    // white lighter
        case 8:    return gfx::Color::Orange;  // orange
        case 16:   return gfx::Color::LightRed; // light red
        case 32:   return gfx::Color::Red;     // red
        case 64:   return gfx::Color::DarkRed;  // dark red
        case 128:  return gfx::Color::Yellow;   // yellow
        case 256:  return gfx::Color::Gold;       // gold/yellow
        case 512:  return gfx::Color::LightGreen; // bright green
        case 1024: return gfx::Color::Cyan;    // cyan
        case 2048: return gfx::Color::Blue;    // blue super
        default:   return gfx::Color::Magenta;    // magenta
    }
}

void Game2048Scene::onUnconsumedTouchEvent(const input::TouchEvent& event) {
    using T = input::TouchEventType;
    auto ty = event.getType();
    
    // Ignore if game over
    if (gameLogic.isGameOver()) {
        return;
    }
    
    // Check for swipe: DragEnd indicates swipe gesture completed
    if (ty == T::DragEnd) {
        int16_t dx = event.x - dragStartX;
        int16_t dy = event.y - dragStartY;
        
        // Require minimum swipe distance
        const int16_t SWIPE_MIN = 30;
        
        // Determine direction based on major axis
        if (dx > SWIPE_MIN && dx > dy) {
            // Swipe right
            int scoreBefore = gameLogic.getScore();
            doMove(gameLogic.moveRight(), scoreBefore);
        } else if (dx < -SWIPE_MIN && dx < dy) {
            // Swipe left
            int scoreBefore = gameLogic.getScore();
            doMove(gameLogic.moveLeft(), scoreBefore);
        } else if (dy > SWIPE_MIN && dy > dx) {
            // Swipe down
            int scoreBefore = gameLogic.getScore();
            doMove(gameLogic.moveDown(), scoreBefore);
        } else if (dy < -SWIPE_MIN && dy < dx) {
            // Swipe up
            int scoreBefore = gameLogic.getScore();
            doMove(gameLogic.moveUp(), scoreBefore);
        }
        
        // Reset drag state
        dragActive = false;
        return;
    }
    
    // Track drag start position
    if (ty == T::DragStart || ty == T::TouchDown) {
        dragStartX = event.x;
        dragStartY = event.y;
        dragActive = true;
    }
}

void Game2048Scene::doMove(bool moved, int scoreBefore) {
    if (moved) {
        unsigned long now = millis();
        lastMoveTime = now;
        
#if PIXELROOT32_ENABLE_AUDIO
        auto& audio = engine.getAudioEngine();
#endif
        
        gameLogic.spawnTile();
        
#if PIXELROOT32_ENABLE_AUDIO
        // Play spawn sound when new tile appears
        playSpawnSound(audio);
        
        // Check if merge happened by comparing score (score was already updated by move)
        int scoreAfter = gameLogic.getScore();
        if (scoreAfter > scoreBefore) {
            // Merge occurred - play merge sound
            playMergeSound(audio);
        } else {
            // Just movement without merge - play move sound
            playMoveSound(audio);
        }
        
        // Check win after spawn new tile
        if (gameLogic.hasWon()) {
            playWinSound(audio);
        }
#endif
        
        gameLogic.clearMovedFlag();
        gameLogic.checkGameOver();
        
        updateLabels();
    }
}

} // namespace game2048