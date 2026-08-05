#pragma once
#include <cstdint>
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"
#include "BoardRenderer.h"
#include "BombermanBoard.h"
#include "BombermanBombs.h"
#include "BombermanConstants.h"
#include "PlayerActor.h"

namespace bomberman {

/**
 * @class BombermanScene
 * @brief Scene shell: fixed-step drain, board init, player + bomb/chain
 *        logic, and lives/restart on explosion death.
 *
 * Enemy actors, the HUD, and the full five-state level enum (Loading,
 * Playing, ExitUnlocked, StageClear, GameOver) are assembled in later
 * work. This slice only needs Loading -> Playing, plus a simple frozen
 * state once lives run out, since nothing here yet needs the other three
 * states.
 */
class BombermanScene : public pixelroot32::core::Scene {
public:
    BombermanScene();

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    enum class LevelState : uint8_t { Loading, Playing };

    TileType board_[kCells];
    Bomb bombs_[kMaxBombs];
    uint8_t blastSteps_[kCells];
    TileType hiddenPowerUp_;
    BoardRenderer renderer_;
    PlayerActor player_;

    LevelState state_;
    unsigned long accumulatorMs_;
    uint32_t seed_;
    int lives_;

    /// (Re)generates the board, resets the bomb pool/explosion mask, and
    /// resets the player. Deliberately does NOT call the base Scene::init(),
    /// since Scene::resetState() clears every entity and this scene's
    /// entities are added once, in the constructor, and never re-added —
    /// matching the snake/2048 examples. Deliberately does NOT touch
    /// `lives_`: that field lives outside this function entirely so a
    /// death-with-lives-remaining restart carries the decremented value
    /// over instead of resetting it.
    void startLevel();

    /// The fixed-step logic pipeline, driven by update()'s accumulator:
    /// player input/movement/placement, bomb fuse tick, chain-reaction
    /// drain, explosion decay, then explosion-contact death. Enemy
    /// movement, power-ups, and victory are added in later work.
    void logicStep();

    /// One life lost to an explosion. Restarts the level if lives remain;
    /// otherwise leaves the board frozen — logicStep()'s own guard stops
    /// processing once lives_ reaches zero.
    void handlePlayerDeath();
};

}  // namespace bomberman
