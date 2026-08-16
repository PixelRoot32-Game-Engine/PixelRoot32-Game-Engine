#include "EnemyActor.h"
#include "math/MathUtil.h"
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

namespace bomberbot {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

namespace {
constexpr int kDirX[4] = {0, 0, -1, 1};   // Up, Down, Left, Right
constexpr int kDirY[4] = {-1, 1, 0, 0};
}  // namespace

EnemyActor::EnemyActor(int startCellX, int startCellY)
    : core::Actor(gameplay::cellToWorld(startCellX, startCellY, kBoardGrid), kCellSize, kCellSize) {
    resetTo(startCellX, startCellY);
}

void EnemyActor::logicStep(const TileType (&board)[kCells], const Bomb (&bombs)[kMaxBombs]) {
    // Step mechanics come from gameplay::GridMotion; only the AI policy is
    // here -- see the class doc comment in EnemyActor.h and
    // PlayerActor::logicStep()'s for what stays per-actor and why.
    if (!alive_) {
        return;
    }

    if (gameplay::isMoving(mv)) {
        // A step is already in flight: finish it. Direction is never
        // re-decided in here -- only at the instant a step completes, below.
        // This actor ignores tickStep()'s arrival edge: arrival is already
        // observable as "at rest" on the next call, and the direction draw
        // below runs there.
        gameplay::tickStep(mv, kEnemyStepsPerCell);
        updateInterpolatedPosition();
        return;
    }

    // At rest: this is "on arrival" for every call after the first, and the
    // initial direction draw for the very first call after spawn (dirX_ ==
    // dirY_ == 0 makes the straight-ahead check below fail, so the first
    // decision always falls through to the PRNG draw).
    if (dirX_ != 0 || dirY_ != 0) {
        const int sx = mv.cellX + dirX_;
        const int sy = mv.cellY + dirY_;
        if (canEnter(sx, sy, board, bombs)) {
            facingLeft_ = (dirX_ < 0);  // refresh defensively
            gameplay::beginStep(mv, sx, sy);
            updateInterpolatedPosition();
            return;
        }
    }

    // Blocked (or no direction committed yet): collect every currently
    // enterable direction and draw one, uniformly, from the seeded PRNG.
    // Reverse is included on purpose -- excluding it would deadlock an
    // enemy in a dead-end corridor, which this board's corridor lattice
    // produces routinely.
    int valid[4];
    int validCount = 0;
    for (int i = 0; i < 4; ++i) {
        const int nx = mv.cellX + kDirX[i];
        const int ny = mv.cellY + kDirY[i];
        if (canEnter(nx, ny, board, bombs)) {
            valid[validCount++] = i;
        }
    }

    if (validCount == 0) {
        // Fully boxed in (walls/bombs on all four sides): stand still and
        // re-evaluate next step. Not an error, not a crash.
        dirX_ = 0;
        dirY_ = 0;
        updateInterpolatedPosition();
        return;
    }

    const int32_t picked = math::rand_int(0, validCount - 1);
    const int dir = valid[picked];
    dirX_ = kDirX[dir];
    dirY_ = kDirY[dir];
    facingLeft_ = (dirX_ < 0);
    gameplay::beginStep(mv, mv.cellX + dirX_, mv.cellY + dirY_);
    updateInterpolatedPosition();
}

bool EnemyActor::canEnter(int nx, int ny, const TileType (&board)[kCells],
                            const Bomb (&bombs)[kMaxBombs]) const {
    if (!gameplay::containsCell(nx, ny, kBoardGrid)) {
        return false;
    }
    const TileType t = board[cellIndex(nx, ny)];
    if (t == TileType::HardWall || isSoftWall(t)) {
        return false;
    }
    // No exemption of any kind -- every bomb is solid to an enemy.
    return !bombAt(bombs, nx, ny);
}

void EnemyActor::updateInterpolatedPosition() {
    position = gameplay::interpolatedWorld(mv, kEnemyStepsPerCell, kBoardGrid);
}

void EnemyActor::updateDeathAnimation(unsigned long deltaMs) {
    if (dying_ && deathAnimMs_ < kEnemyDeathDurationMs) {
        deathAnimMs_ += deltaMs;
        if (deathAnimMs_ > kEnemyDeathDurationMs) {
            deathAnimMs_ = kEnemyDeathDurationMs;
        }
    }
}

void EnemyActor::draw(gfx::Renderer& renderer) {
    if (!alive_ && !dying_) {
        return;
    }
    const int x = static_cast<int>(position.x);
    const int y = static_cast<int>(position.y);

    if (dying_) {
        // Death animation: 4 kEnemySlimeDeath frames spread across
        // kEnemyDeathDurationMs, clamped to the last frame so the corpse
        // stays visible until the scene removes the entity.
        int frame = static_cast<int>(deathAnimMs_ * kEnemyDeathFrameCount /
                                     kEnemyDeathDurationMs);
        if (frame >= kEnemyDeathFrameCount) {
            frame = kEnemyDeathFrameCount - 1;
        }
        renderer.drawSprite(kEnemySlimeDeath[frame], x, y, 0, false);
        return;
    }

    const Sprite4bpp* sprite = enemyWalkSpriteFor(mv.progress);
    renderer.drawSprite(*sprite, x, y, 0, facingLeft_);
}

core::Rect EnemyActor::getHitBox() {
    return {position, width, height};
}

void EnemyActor::onCollision(core::Actor* other) {
    // Physics is off in this example; movement/collision is resolved by
    // board lookups (canEnter()) and BomberbotScene's own logical-cell
    // checks, so the engine never calls this. Implemented only because
    // Actor::onCollision is pure virtual.
    (void)other;
}

void EnemyActor::resetTo(int startCellX, int startCellY) {
    gameplay::placeAt(mv, startCellX, startCellY);
    dirX_ = 0;
    dirY_ = 0;
    alive_ = true;
    dying_ = false;
    deathAnimMs_ = 0;
    updateInterpolatedPosition();
}

}  // namespace bomberbot
