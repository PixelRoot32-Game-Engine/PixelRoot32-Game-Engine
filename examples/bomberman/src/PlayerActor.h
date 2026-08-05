#pragma once
#include "core/Actor.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include "BombermanBoard.h"
#include "BombermanConstants.h"
#include "GridMove.h"

namespace bomberman {

/**
 * @class PlayerActor
 * @brief Interpolated grid movement driven by held input.
 *
 * logicStep() is called once per fixed logic step from
 * BombermanScene::logicStep() — never from Entity::update(unsigned long),
 * which stays the inherited no-op. This keeps deltaTime out of every
 * movement rule (see BombermanConstants.h's kLogicStepMs comment).
 *
 * Deliberately does NOT share an advance loop with EnemyActor even though
 * both embed a GridMove and both interpolate the same way: the two differ
 * in blocking policy (the player stays put when blocked; the enemy
 * re-picks a direction), in direction source (held input vs. seeded PRNG),
 * and — from Phase 3 on — in the pass-through exemption, which is
 * player-only. Collapsing the two loops early would remove the very
 * comparison this example exists to produce.
 */
class PlayerActor : public pixelroot32::core::Actor {
public:
    PlayerActor(int startCellX, int startCellY);

    /// Advances interpolation and, when at rest, starts a new step toward
    /// a held direction if the target cell is enterable. Board-only
    /// blocking at this phase; bomb blocking and the own-bomb pass-through
    /// exemption are added in Phase 3.
    void logicStep(const TileType (&board)[kCells], const pixelroot32::input::InputManager& input);

    void draw(pixelroot32::graphics::Renderer& renderer) override;
    pixelroot32::core::Rect getHitBox() override;
    void onCollision(pixelroot32::core::Actor* other) override;

    int cellX() const { return mv.cellX; }
    int cellY() const { return mv.cellY; }

    /// Puts the player at rest in a fresh cell (level restart).
    void resetTo(int startCellX, int startCellY);

private:
    GridMove mv;

    bool canEnter(int nx, int ny, const TileType (&board)[kCells]) const;
    void updateInterpolatedPosition();
};

}  // namespace bomberman
