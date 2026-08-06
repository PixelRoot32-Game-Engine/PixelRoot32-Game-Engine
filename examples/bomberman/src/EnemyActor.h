#pragma once
#include "core/Actor.h"
#include "graphics/Renderer.h"
#include "BombermanBoard.h"
#include "BombermanBombs.h"
#include "BombermanConstants.h"
#include "GridMove.h"
#include "assets/EnemySprites.h"

namespace bomberman {

/// Maps progress → the correct Ballom walk-cycle sprite.
/// progress: mv.progress (0 when at rest). 7 frames, wraps.
inline const pixelroot32::graphics::Sprite4bpp* enemyWalkSpriteFor(int progress) {
    const int frame = (progress / kEnemyAnimStepDiv) % 7;
    return &kEnemyBallomWalk[frame];
}

/**
 * @class EnemyActor
 * @brief Interpolated grid movement driven by a straight-else-PRNG AI: keep
 *        going in the current direction while it stays enterable, otherwise
 *        pick a new one from whatever is currently open.
 *
 * logicStep() is called once per fixed logic step from
 * BombermanScene::logicStep(), same as PlayerActor::logicStep(). This class
 * has no pointer, reference, or field naming the player anywhere -- the AI
 * below reads only the board and the bomb pool, never player state.
 *
 * Deliberately does NOT share an advance loop with PlayerActor even though
 * both embed a GridMove and both interpolate the same way: the two differ
 * in blocking policy (the player stays put when blocked; this actor
 * re-picks a direction), in direction source (held input vs. seeded PRNG),
 * and the player's own-bomb pass-through exemption has no counterpart here
 * -- an enemy treats every bomb as solid, with no exception. Collapsing the
 * two loops early would remove the very comparison this example exists to
 * produce.
 */
class EnemyActor : public pixelroot32::core::Actor {
public:
    /// Default-constructs at cell (0,0), dead. Exists only so
    /// BombermanScene's fixed `EnemyActor enemies_[kMaxEnemies]` pool
    /// array-initializes without a per-slot argument list; every slot that
    /// is actually going to play gets a real resetTo() from
    /// BombermanScene::startLevel() before it is added to the scene.
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

    /// Marks the enemy dead. Movement and collision checks skip a dead
    /// enemy; BombermanScene also removes it from the scene's entity list
    /// so it stops drawing, which is the only other thing a dead enemy
    /// could still do.
    void kill() { alive_ = false; }

private:
    GridMove mv;
    int dirX_ = 0;
    int dirY_ = 0;
    bool alive_ = true;

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

}  // namespace bomberman
