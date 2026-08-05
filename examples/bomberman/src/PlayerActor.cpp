#include "PlayerActor.h"
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

namespace bomberman {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace input = pr32::input;
namespace math = pr32::math;

PlayerActor::PlayerActor(int startCellX, int startCellY)
    : core::Actor(gameplay::cellToWorld(startCellX, startCellY, kBoardGrid), kCellSize, kCellSize) {
    resetTo(startCellX, startCellY);
}

void PlayerActor::logicStep(const TileType (&board)[kCells], const input::InputManager& inputManager) {
    if (mv.progress > 0) {
        // A step is already in flight: finish it, ignore input this step.
        ++mv.progress;
        if (mv.progress >= kPlayerStepsPerCell) {
            // The LOGICAL cell flips here, and only here.
            mv.cellX = mv.toX;
            mv.cellY = mv.toY;
            mv.progress = 0;
            // onArrive() hook point: nothing to do yet in this slice. Phase
            // 3 clears the bomb pass-through exemption here.
        }
        updateInterpolatedPosition();
        return;
    }

    // At rest: held input may start a new step. Fixed priority
    // Up > Down > Left > Right over currently-held buttons; diagonals are
    // unrepresentable by construction — exactly one (dx, dy) with
    // |dx| + |dy| == 1 is ever chosen.
    int dx = 0;
    int dy = 0;
    if (inputManager.isButtonDown(BTN_UP)) {
        dy = -1;
    } else if (inputManager.isButtonDown(BTN_DOWN)) {
        dy = 1;
    } else if (inputManager.isButtonDown(BTN_LEFT)) {
        dx = -1;
    } else if (inputManager.isButtonDown(BTN_RIGHT)) {
        dx = 1;
    }

    if (dx != 0 || dy != 0) {
        const int nx = mv.cellX + dx;
        const int ny = mv.cellY + dy;
        if (canEnter(nx, ny, board)) {
            mv.toX = nx;
            mv.toY = ny;
            mv.progress = 1;
        }
        // else: stay put. Not a death, not an error, not a retry — holding
        // a direction into a blocked cell is inert.
    }

    updateInterpolatedPosition();
}

bool PlayerActor::canEnter(int nx, int ny, const TileType (&board)[kCells]) const {
    if (!gameplay::containsCell(nx, ny, kBoardGrid)) {
        return false;
    }
    const TileType t = board[cellIndex(nx, ny)];
    // Bomb blocking and the own-bomb pass-through exemption are added in
    // Phase 3 (3.3); this phase only ever has board tiles to consult.
    return t != TileType::HardWall && !isSoftWall(t);
}

void PlayerActor::updateInterpolatedPosition() {
    // Integer-pixel lerp, exact at both endpoints (progress == 0 or
    // progress == kPlayerStepsPerCell - 1 followed by the flip above
    // reproduces cellToWorld exactly) — see GridMove.h and
    // BombermanConstants.h for why this stays integer-only.
    const int fromPx = gameplay::cellToWorldX(mv.cellX, kBoardGrid);
    const int fromPy = gameplay::cellToWorldY(mv.cellY, kBoardGrid);
    const int toPx = gameplay::cellToWorldX(mv.toX, kBoardGrid);
    const int toPy = gameplay::cellToWorldY(mv.toY, kBoardGrid);
    const int x = fromPx + (toPx - fromPx) * mv.progress / kPlayerStepsPerCell;
    const int y = fromPy + (toPy - fromPy) * mv.progress / kPlayerStepsPerCell;
    position = math::Vector2(x, y);
}

void PlayerActor::draw(gfx::Renderer& renderer) {
    const int x = static_cast<int>(position.x);
    const int y = static_cast<int>(position.y);
    renderer.drawFilledRectangle(x + 3, y + 4, 10, 12, gfx::Color::White);
    renderer.drawFilledCircle(x + 8, y + 4, 4, gfx::Color::LightGreen);
}

core::Rect PlayerActor::getHitBox() {
    return {position, width, height};
}

void PlayerActor::onCollision(core::Actor* other) {
    // Physics is off in this example (movement/collision is resolved by
    // board lookups in canEnter()), so the engine never calls this. It is
    // implemented only because Actor::onCollision is pure virtual.
    (void)other;
}

void PlayerActor::resetTo(int startCellX, int startCellY) {
    mv.cellX = mv.toX = startCellX;
    mv.cellY = mv.toY = startCellY;
    mv.progress = 0;
    updateInterpolatedPosition();
}

}  // namespace bomberman
