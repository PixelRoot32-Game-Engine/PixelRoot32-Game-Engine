#pragma once
#include <cstdint>
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"
#include "BoardRenderer.h"
#include "BombermanBoard.h"
#include "BombermanConstants.h"
#include "PlayerActor.h"

namespace bomberman {

/**
 * @class BombermanScene
 * @brief Scene shell: fixed-step drain, board init, and player spawn.
 *
 * The bomb pool, explosion resolution, enemy actors, HUD, and the full
 * five-state level enum (Loading, Playing, ExitUnlocked, StageClear,
 * GameOver) are assembled in later phases. This slice only needs
 * Loading -> Playing, since nothing yet transitions past Playing.
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
    BoardRenderer renderer_;
    PlayerActor player_;

    LevelState state_;
    unsigned long accumulatorMs_;
    uint32_t seed_;

    /// (Re)generates the board and resets the player. Deliberately does
    /// NOT call the base Scene::init(), since Scene::resetState() clears
    /// every entity and this scene's entities are added once, in the
    /// constructor, and never re-added — matching the snake/2048 examples.
    void startLevel();

    /// The fixed-step logic pipeline, driven by update()'s accumulator.
    /// Only player movement exists at this phase; bombs, explosions,
    /// enemies, collisions, and victory are added in later phases.
    void logicStep();
};

}  // namespace bomberman
