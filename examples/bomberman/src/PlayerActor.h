#pragma once
#include "core/Actor.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include "BombermanBoard.h"
#include "BombermanBombs.h"
#include "BombermanConstants.h"
#include "GridMove.h"

namespace bomberman {

/**
 * @class PlayerActor
 * @brief Interpolated grid movement driven by held input, bomb placement,
 *        and the own-bomb pass-through exemption.
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
 * and in the pass-through exemption below, which is player-only.
 * Collapsing the two loops early would remove the very comparison this
 * example exists to produce.
 */
class PlayerActor : public pixelroot32::core::Actor {
public:
    PlayerActor(int startCellX, int startCellY);

    /// Places a bomb if `bombPressed`, then advances interpolation and, when
    /// at rest, starts a new step toward a held direction if the target
    /// cell is enterable (board tiles and bombs, minus the own-bomb
    /// pass-through exemption).
    ///
    /// `bombPressed` is passed in rather than read from `input` because a
    /// press edge belongs to the frame that produced it, while this function
    /// runs on the fixed logic step -- the caller latches the edge across
    /// that gap. Held direction is a level state and is read from `input`
    /// directly, which is correct to sample per step.
    ///
    /// Returns true iff a bomb was actually placed this call (the press was
    /// latched AND the pool/limit checks in tryPlaceBomb() both passed) --
    /// the caller uses this to fire the bomb-placed audio event exactly once
    /// per successful placement, never on a press that was rejected.
    bool logicStep(const TileType (&board)[kCells], Bomb (&bombs)[kMaxBombs],
                   const pixelroot32::input::InputManager& input, bool bombPressed);

    void draw(pixelroot32::graphics::Renderer& renderer) override;
    pixelroot32::core::Rect getHitBox() override;
    void onCollision(pixelroot32::core::Actor* other) override;

    int cellX() const { return mv.cellX; }
    int cellY() const { return mv.cellY; }
    int firePower() const { return firePower_; }
    int maxBombs() const { return maxBombs_; }

    /// Applies a Fire power-up: increases the blast arm length by the fixed
    /// increment. Takes effect on the NEXT bomb placed -- an already-placed
    /// bomb keeps the range it snapshotted at placement (see BombermanBombs.h).
    void applyFirePowerUp() { firePower_ += kFirePowerIncrement; }

    /// Applies a Bomb power-up: increases the simultaneous-bomb limit by
    /// one.
    void applyBombPowerUp() { ++maxBombs_; }

    /// Puts the player at rest in a fresh cell (level restart). Also
    /// clears the own-bomb exemption, since a fresh level has no bombs.
    void resetTo(int startCellX, int startCellY);

private:
    GridMove mv;
    int firePower_ = kDefaultFirePower;
    int maxBombs_ = kDefaultMaxBombs;

    // Own-bomb pass-through: the one bomb-occupied cell that does NOT
    // block the player, valid only while it names the player's current
    // logical cell. Three rules, no fourth:
    //   1. Placing a bomb sets this to the player's logical cell
    //      (tryPlaceBomb()).
    //   2. canEnter() lets a bomb-occupied target through only if it
    //      equals this cell.
    //   3. The instant the logical cell changes (onArrive, inside
    //      logicStep()), this clears unless the arrival cell still
    //      matches it.
    // The logical cell changes at exactly one instant (step completion),
    // so there is no window where this is half-valid: mid-interpolation,
    // the logical cell is still the FROM-cell, and input toward a new step
    // is ignored anyway while a step is in flight.
    bool exemptValid_ = false;
    int exemptX_ = 0;
    int exemptY_ = 0;

    bool canEnter(int nx, int ny, const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs]) const;
    void updateInterpolatedPosition();

    /// Attempts to place a bomb in the player's current logical cell
    /// (mv.cellX, mv.cellY — the FROM-cell if a step is mid-flight, since
    /// that value never changes outside onArrive()). Delegates the pool
    /// write to placeBomb() and, on success, sets the exemption using the
    /// SAME cell just written, so the two can never disagree. No-op,
    /// returns false, if the player is already at their own max-bombs
    /// limit or the pool write itself fails.
    bool tryPlaceBomb(Bomb (&bombs)[kMaxBombs]);
};

}  // namespace bomberman
