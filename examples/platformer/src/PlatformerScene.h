#pragma once
#include "core/Scene.h"

#include "GameConstants.h"

#include <memory>

namespace platformerdemo {

class CrateActor;
class LevelBlock;
class PlayerCharacter;

/**
 * @class PlatformerScene
 * @brief One screen, one subject: the three physics actors and the collision
 *        matrix that connects them.
 *
 * - `KinematicActor` — the character (`PlayerCharacter`), moved by script.
 * - `RigidActor`     — the crates (`CrateActor`), moved by the solver.
 * - `StaticActor`    — the level (`LevelBlock`), moved by nobody.
 *
 * There is no camera: the level is exactly one logical screen, so the actors
 * stay the subject. See `examples/camera` for `Camera2D`, parallax and effects.
 */
class PlatformerScene : public pixelroot32::core::Scene {
public:
    PlatformerScene();
    ~PlatformerScene() override;

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

protected:
    void resetState() noexcept override;

private:
    /// Builds the level from the table in the .cpp: one body per run of tiles.
    void spawnLevel();
    /// Builds the crates above the shelf.
    void spawnCrates();
    /// Actor legend and control hints, drawn over the level.
    void drawHud(pixelroot32::graphics::Renderer& renderer);

    std::unique_ptr<PlayerCharacter> player;
    std::unique_ptr<LevelBlock> blocks[MAX_LEVEL_BLOCKS];
    int blockCount = 0;
    std::unique_ptr<CrateActor> crates[MAX_CRATES];
    int crateCount = 0;
};

}
