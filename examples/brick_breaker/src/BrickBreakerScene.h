#pragma once
#include "actors/PaddleActor.h"
#include "actors/BallActor.h"
#include "actors/BrickActor.h"
#include <core/Scene.h>
#include <graphics/ui/UILabel.h>
#include <physics/StaticActor.h>
#include <graphics/particles/ParticleEmitter.h>
#include <audio/MusicPlayer.h>
#include <audio/AudioMusicTypes.h>
#include <array>
#include <bitset>
#include <memory>

namespace brickbreaker {

// Maximum number of bricks in any level (7 cols × 7 rows = 49, use 64 for alignment)
static constexpr int MAX_BRICK_POOL = 64;

/**
 * @class BrickBreakerScene
 * @brief Breakout-style scene with paddle, ball, destructible bricks, and particle effects.
 *
 * Manages level loading, ball lifecycle, and collision-driven gameplay.
 * Uses StaticActors for walls and bricks; RigidActor for the ball.
 * 
 * OPTIMIZATION: Uses object pooling for bricks to eliminate heap allocations at runtime.
 * All bricks are pre-allocated in init() and reused across levels.
 */
class BrickBreakerScene : public pixelroot32::core::Scene {
public:
    BrickBreakerScene();
    ~BrickBreakerScene();

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Adds points to the current score.
     * @param score Points to add.
     */
    void addScore(int score);

    /**
     * @brief Returns the particle emitter for brick destruction effects.
     */
    pixelroot32::graphics::particles::ParticleEmitter* getParticleEmiter() { return explosionEffect.get(); }

private:
    void loadLevel(int level);
    void resetBall();
    void setupMusic();
    
    // Object pool for bricks - eliminates runtime heap allocations
    std::array<std::unique_ptr<BrickActor>, MAX_BRICK_POOL> brickPool;
    std::bitset<MAX_BRICK_POOL> brickActive;  ///< Tracks which bricks are currently in use
    int activeBrickCount = 0;

    std::unique_ptr<pixelroot32::audio::MusicPlayer> musicPlayer;
    pixelroot32::audio::MusicTrack bgmTrack;

    std::unique_ptr<pixelroot32::graphics::particles::ParticleEmitter> explosionEffect;
    std::unique_ptr<PaddleActor> paddle;
    std::unique_ptr<BallActor> ball;

    std::unique_ptr<pixelroot32::physics::StaticActor> topWall;
    std::unique_ptr<pixelroot32::physics::StaticActor> leftWall;
    std::unique_ptr<pixelroot32::physics::StaticActor> rightWall;

    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblGameOver;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblStartMessage;

    int score;       ///< Current score
    int lives;       ///< Remaining lives
    int currentLevel; ///< Current level index
    bool gameStarted; ///< True after first launch
    bool gameOver;    ///< True when game has ended
};

}
