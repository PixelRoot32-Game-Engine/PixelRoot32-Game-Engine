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
    // Duplicated advance loop, deliberately -- see the class doc comment in
    // EnemyActor.h and PlayerActor::logicStep()'s own doc comment for why
    // this is not collapsed into one shared function with the player's.
    if (!alive_) {
        return;
    }

    if (mv.progress > 0) {
        // A step is already in flight: finish it. Direction is never
        // re-decided in here -- only at the instant a step completes, below.
        ++mv.progress;
        if (mv.progress >= kEnemyStepsPerCell) {
            mv.cellX = mv.toX;
            mv.cellY = mv.toY;
            mv.progress = 0;
        }
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
            mv.toX = sx;
            mv.toY = sy;
            mv.progress = 1;
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
    mv.toX = mv.cellX + dirX_;
    mv.toY = mv.cellY + dirY_;
    mv.progress = 1;
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
    const int fromPx = gameplay::cellToWorldX(mv.cellX, kBoardGrid);
    const int fromPy = gameplay::cellToWorldY(mv.cellY, kBoardGrid);
    const int toPx = gameplay::cellToWorldX(mv.toX, kBoardGrid);
    const int toPy = gameplay::cellToWorldY(mv.toY, kBoardGrid);
    const int x = fromPx + (toPx - fromPx) * mv.progress / kEnemyStepsPerCell;
    const int y = fromPy + (toPy - fromPy) * mv.progress / kEnemyStepsPerCell;
    position = math::Vector2(x, y);
}

void EnemyActor::draw(gfx::Renderer& renderer) {
    if (!alive_) {
        return;
    }
    const int x = static_cast<int>(position.x);
    const int y = static_cast<int>(position.y);
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
    mv.cellX = mv.toX = startCellX;
    mv.cellY = mv.toY = startCellY;
    mv.progress = 0;
    dirX_ = 0;
    dirY_ = 0;
    alive_ = true;
    updateInterpolatedPosition();
}

}  // namespace bomberbot
