#pragma once
#include "core/Actor.h"
#include "graphics/Renderer.h"
#include "BomberbotBoard.h"
#include "BomberbotBombs.h"
#include "BomberbotConstants.h"
#include "gameplay/GridMotion.h"
#include "assets/EnemySprites.h"

namespace bomberbot {

/// Maps progress → the correct slime walk-cycle sprite.
/// progress: mv.progress (0 when at rest). 7 frames, wraps.
inline const pixelroot32::graphics::Sprite4bpp* enemyWalkSpriteFor(int progress) {
    const int frame = (progress / kEnemyAnimStepDiv) % 7;
    return &kEnemySlimeWalk[frame];
}

/**
 * @class EnemyActor
 * @brief Interpolated grid movement driven by a straight-else-PRNG AI: keep
 *        going in the current direction while it stays enterable, otherwise
 *        pick a new one from whatever is currently open.
 *
 * logicStep() is called once per fixed logic step from
 * BomberbotScene::logicStep(), same as PlayerActor::logicStep(). This class
 * has no pointer, reference, or field naming the player anywhere -- the AI
 * below reads only the board and the bomb pool, never player state.
 *
 * The mechanical half of the movement -- logical cell, in-flight target,
 * progress counter, arrival edge and the cell-to-pixel lerp -- comes from the
 * engine's gameplay::GridMotion, shared with PlayerActor. What stays here is
 * the policy the two actors disagree on: blocking behaviour (the player stays
 * put when blocked; this actor re-picks a direction), direction source (held
 * input vs. seeded PRNG), and the player's own-bomb pass-through exemption,
 * which has no counterpart here -- an enemy treats every bomb as solid, with
 * no exception.
 */
class EnemyActor : public pixelroot32::core::Actor {
public:
    /// Default-constructs at cell (0,0), dead. Exists only so
    /// BomberbotScene's fixed `EnemyActor enemies_[kMaxEnemies]` pool
    /// array-initializes without a per-slot argument list; every slot that
    /// is actually going to play gets a real resetTo() from
    /// BomberbotScene::startLevel() before it is added to the scene.
    EnemyActor() : EnemyActor(0, 0) { alive_ = false; }
    EnemyActor(int startCellX, int startCellY);

    /// Advances interpolation and, only at the instant a step completes
    /// (never mid-step), re-decides direction: continue straight if the
    /// current direction is still enterable, else draw one seeded PRNG
    /// value over whatever directions currently ARE enterable, else stand
    /// still if none are (fully boxed in).
    void logicStep(const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs]);

    void draw(pixelroot32::graphics::Renderer& renderer) override;
    pixelroot32::core::Rect getHitBox() override;
    void onCollision(pixelroot32::core::Actor* other) override;

    int cellX() const { return mv.cellX; }
    int cellY() const { return mv.cellY; }
    bool isAlive() const { return alive_; }

    /// Puts the enemy at rest in a fresh cell, alive, with no committed
    /// direction yet (the first logicStep() call after this draws the
    /// initial direction from the same seeded PRNG as any later redecision).
    void resetTo(int startCellX, int startCellY);

    /// Marks the enemy dead and starts the death animation
    /// (kEnemySlimeDeath[4]). The scene calls this from killEnemy() instead
    /// of the plain kill() so the death frames are shown before the enemy
    /// is removed from the entity list.
    void kill() {
        alive_ = false;
        dying_ = true;
        deathAnimMs_ = 0;
    }

    /// True while the death animation is running. draw() renders the death
    /// frames in this state; the scene removes the entity once
    /// isDeathAnimationDone().
    bool isDying() const { return dying_; }

    /// True once the death animation has played its full duration. The
    /// scene reads this to know when the enemy can be removed from the
    /// entity list (the corpse has finished its 4-frame death).
    bool isDeathAnimationDone() const { return dying_ && deathAnimMs_ >= kEnemyDeathDurationMs; }

    /// Advances the death animation clock by real wall-clock ms. Called by
    /// BomberbotScene::update() every frame; a no-op while not dying.
    void updateDeathAnimation(unsigned long deltaMs);

private:
    pixelroot32::gameplay::GridMotion mv;
    int dirX_ = 0;
    int dirY_ = 0;
    bool alive_ = true;

    /// Death animation state. Latch set by kill(); cleared by resetTo().
    /// deathAnimMs_ accumulates wall-clock ms via updateDeathAnimation();
    /// draw() maps it onto the 4 kEnemySlimeDeath frames.
    bool dying_ = false;
    unsigned long deathAnimMs_ = 0;

    /// Whether the enemy currently faces left. Updated in logicStep()
    /// when dirX_ is decided (PRNG draw or continue-straight).
    /// Passed as flipX to drawSprite() in draw().
    bool facingLeft_ = false;

    /// Board + bomb-pool blocking check only -- no exemption of any kind,
    /// unlike PlayerActor::canEnter(). An enemy treats every bomb as a
    /// solid obstacle, including one it could theoretically have "just
    /// left", because enemies never place bombs.
    bool canEnter(int nx, int ny, const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs]) const;
    void updateInterpolatedPosition();
};

}  // namespace bomberbot
