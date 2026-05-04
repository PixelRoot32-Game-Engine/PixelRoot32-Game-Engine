#pragma once
#include <core/Scene.h>
#include <platforms/EngineConfig.h>
#include <input/TouchEvent.h>
#include "Game2048Constants.h"
#include "Game2048Logic.h"

#ifdef GAME2048_AI_MODE
#include "assets/ai2048.h"
#endif

#include <graphics/ui/UILabel.h>

#include <input/TouchManager.h>

#if PIXELROOT32_ENABLE_AUDIO
#include <audio/AudioEngine.h>
#include <audio/AudioTypes.h>
#include <audio/MusicPlayer.h>
#include <audio/AudioMusicTypes.h>
#endif

namespace game2048 {

/**
 * @class Game2048Scene
 * @brief 2048 puzzle game scene with D-pad input and rendering.
 *
 * Implements grid rendering, tile display with values, score tracking,
 * and game over state with reset functionality.
 */
class Game2048Scene : public pixelroot32::core::Scene {
public:
    /** Default constructor */
    Game2048Scene();
    
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Reset the game state - called by reset button.
     */
    void resetGame();

    /**
     * @brief Handle unconsumed touch events for tile movement.
     * Maps touch swipe to direction.
     */
    void onUnconsumedTouchEvent(const pixelroot32::input::TouchEvent& event) override;

private:
    Game2048Logic gameLogic;

    /** Input cooldown to prevent too-fast movement */
    unsigned long lastMoveTime;
    static constexpr unsigned long MOVE_COOLDOWN_MS = 150;

    /** Track input state for edge detection */
    bool inputReady;
    
    /** Drag/swipe state for touch input */
    int16_t dragStartX;
    int16_t dragStartY;
    bool dragActive;
    
    /** Game state tracking for audio triggers */
    bool wasGameOver;
    bool wasWon;

#ifdef GAME2048_AI_MODE
    /** AI auto-play mode (controlled by -D GAME2048_AI_MODE) */
    ai::AI2048 aiController;
#endif

    /** Grid position on screen */
    pixelroot32::math::Vector2 gridPosition;

    // UI Elements
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> scoreLabel;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> statusLabel;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> instructionLabel;

    /**
     * @brief Create UI labels.
     */
    void createLabels();

    /**
     * @brief Handle D-pad input.
     */
    void handleInput();
    
    /**
     * @brief Handle a move result (shared by keyboard and touch).
     * @param moved True if any tile moved or merged
     * @param scoreBefore Score value before the move (to detect merge)
     */
    void doMove(bool moved, int scoreBefore);

    /**
     * @brief Check if any move is possible and update game over state.
     */
    void checkGameState();

    /**
     * @brief Update score and status label text.
     */
    void updateLabels();

    /**
     * @brief Draw the game grid.
     */
    void drawGrid(pixelroot32::graphics::Renderer& renderer);

    /**
     * @brief Draw all tiles.
     */
    void drawTiles(pixelroot32::graphics::Renderer& renderer);

    /**
     * @brief Draw a single tile.
     */
    void drawTile(pixelroot32::graphics::Renderer& renderer, int row, int col, uint16_t value);

    /**
     * @brief Get color for a tile value.
     */
    pixelroot32::graphics::Color getTileColor(uint16_t value) const;
};

} // namespace game2048