#pragma once
#include "core/Actor.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include "BomberbotBoard.h"
#include "BomberbotBombs.h"
#include "BomberbotConstants.h"
#include "gameplay/GridMotion.h"
#include "assets/PlayerSprites.h"

namespace bomberbot {

/// Return value for playerWalkSpriteFor(): sprite pointer + whether the
/// renderer should mirror it horizontally (for left-facing).
struct PlayerWalkFrame {
    const pixelroot32::graphics::Sprite4bpp* sprite;
    bool flipX;
};

/// Maps (facing, progress) → the correct walk-cycle sprite + flipX.
/// facing: 0=Down, 1=Up, 2=Left (Right sprite mirrored), 3=Right.
/// progress: mv.progress (0 when at rest).
inline PlayerWalkFrame playerWalkSpriteFor(uint8_t facing, int progress) {
    const int frame = (progress / kPlayerAnimStepDiv) % 3;
    switch (facing) {
        case 0:  // Down
            return { &kPlayerWalkDown[frame], false };
        case 1:  // Up
            return { &kPlayerWalkUp[frame], false };
        case 2:  // Left (Right sprite, mirrored)
            return { &kPlayerWalkRight[frame], false };
        case 3:  // Right
        default:
            return { &kPlayerWalkRight[frame], true };
    }
}

/**
 * @class PlayerActor
 * @brief Interpolated grid movement driven by held input, bomb placement,
 *        and the own-bomb pass-through exemption.
 *
 * logicStep() is called once per fixed logic step from
 * BomberbotScene::logicStep() — never from Entity::update(unsigned long),
 * which stays the inherited no-op. This keeps deltaTime out of every
 * movement rule (see BomberbotConstants.h's kLogicStepMs comment).
 *
 * The mechanical half of the movement — logical cell, in-flight target,
 * progress counter, arrival edge and the cell-to-pixel lerp — comes from the
 * engine's gameplay::GridMotion, shared with EnemyActor. What stays here is
 * the policy the two actors disagree on: blocking behaviour (the player
 * stays put when blocked; the enemy re-picks a direction), direction source
 * (held input vs. seeded PRNG), and the pass-through exemption below, which
 * is player-only.
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

    /// Last chosen movement direction, refreshed when a new step begins.
    /// 0=Down, 1=Up, 2=Left, 3=Right. Read by draw() and by the scene to
    /// pick the correct footstep SFX (vertical vs horizontal).
    uint8_t facing() const { return facing_; }

    /// True iff logicStep() started a NEW step this call (at-rest → a held
    /// direction passed canEnter()), as opposed to merely advancing an
    /// already in-flight step. Read by the scene right after logicStep() to
    /// fire the footstep exactly once per step start; cleared by the next
    /// logicStep() call.
    bool stepStarted() const { return stepStarted_; }

    /// Starts the player death animation (kPlayerDeath[6]). The scene calls
    /// this from the single death funnel (handlePlayerDeath()) and then
    /// freezes the simulation until isDeathAnimationDone() is true. Does
    /// not stop the player from being drawn: draw() switches to the death
    /// frames while dying.
    void startDeathAnimation();

    /// True once the death animation has played its full duration. The
    /// scene reads this to know when to apply the deferred life loss /
    /// level restart.
    bool isDeathAnimationDone() const { return dying_ && deathAnimMs_ >= kPlayerDeathDurationMs; }

    /// Advances the death animation clock by real wall-clock ms. Called by
    /// BomberbotScene::update() every frame; a no-op while not dying. This
    /// deliberately does NOT touch the fixed logic clock.
    void updateDeathAnimation(unsigned long deltaMs);

    /// True while the death animation is running (started but not yet
    /// done). draw() renders the death frames in this state.
    bool isDying() const { return dying_; }

    /// Applies a Fire power-up: increases the blast arm length by the fixed
    /// increment. Takes effect on the NEXT bomb placed -- an already-placed
    /// bomb keeps the range it snapshotted at placement (see BomberbotBombs.h).
    void applyFirePowerUp() { firePower_ += kFirePowerIncrement; }

    /// Applies a Bomb power-up: increases the simultaneous-bomb limit by
    /// one.
    void applyBombPowerUp() { ++maxBombs_; }

    /// Puts the player at rest in a fresh cell (level restart). Also
    /// clears the own-bomb exemption, since a fresh level has no bombs.
    void resetTo(int startCellX, int startCellY);

private:
    pixelroot32::gameplay::GridMotion mv;

    /// Last chosen movement direction. 0=Down, 1=Up, 2=Left, 3=Right.
    /// Updated in logicStep() when a new step begins (after canEnter()
    /// succeeds, before beginStep()). Read by draw() to select the
    /// correct walk-cycle sprite row.
    uint8_t facing_ = 0;

    /// Set true by logicStep() at the instant a new step begins (held
    /// direction passes canEnter() while at rest); otherwise false. See
    /// stepStarted().
    bool stepStarted_ = false;

    /// Death animation state. dying_ is latched by startDeathAnimation()
    /// and cleared by resetTo() (fresh level). deathAnimMs_ accumulates
    /// wall-clock ms via updateDeathAnimation(); draw() maps it onto the
    /// 6 kPlayerDeath frames.
    bool dying_ = false;
    unsigned long deathAnimMs_ = 0;
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

}  // namespace bomberbot
