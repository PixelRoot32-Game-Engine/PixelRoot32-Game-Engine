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

bool PlayerActor::logicStep(const TileType (&board)[kCells], Bomb (&bombs)[kMaxBombs],
                             const input::InputManager& inputManager, bool bombPressed) {
    // This advance loop is a deliberate, near-duplicate of
    // EnemyActor::logicStep()'s. They are not merged into one shared
    // controller: this one stays put when blocked and reads held input,
    // while the enemy's re-picks a direction and reads the seeded PRNG, and
    // only this one has the own-bomb pass-through exemption below. Sharing
    // just the five-field GridMove struct is the entire real overlap; a
    // controller that also owned blocking policy or the arrival callback
    // would need three behaviour hooks to configure it back into these two
    // shapes, which is worse than the loop it would replace.

    // Bomb placement is handled first, before movement advances below. That
    // guarantees a bomb dropped this step always lands in the cell that
    // was logical at the START of this call, even on a call where movement
    // also happens to complete an in-flight step — "placement uses the
    // FROM-cell" stays true by construction instead of depending on
    // statement order elsewhere.
    //
    // `bombPressed` arrives already latched by the caller rather than being
    // read from inputManager here: the press edge is a frame-scoped fact and
    // this function runs on the fixed logic step, which is a different
    // clock. Held movement below is a level state, so sampling it per step
    // is correct as-is.
    bool bombPlaced = false;
    if (bombPressed) {
        bombPlaced = tryPlaceBomb(bombs);
    }

    if (mv.progress > 0) {
        // A step is already in flight: finish it, ignore movement input
        // this step.
        ++mv.progress;
        if (mv.progress >= kPlayerStepsPerCell) {
            // The LOGICAL cell flips here, and only here.
            mv.cellX = mv.toX;
            mv.cellY = mv.toY;
            mv.progress = 0;

            // onArrive(): the own-bomb exemption is valid only while it
            // names the player's current logical cell. Leaving the bombed
            // cell makes that bomb solid again from this instant. A no-op
            // if no exemption is active, or if the arrival cell still
            // matches it.
            if (exemptValid_ && (mv.cellX != exemptX_ || mv.cellY != exemptY_)) {
                exemptValid_ = false;
            }
        }
        updateInterpolatedPosition();
        return bombPlaced;
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
        if (canEnter(nx, ny, board, bombs)) {
            // Update facing before starting the step. Encode (dx, dy) into a
            // single facing value: Down=0, Up=1, Left=2, Right=3.
            if (dx == 0 && dy == 1)       facing_ = 0;  // Down
            else if (dx == 0 && dy == -1) facing_ = 1;  // Up
            else if (dx == -1)            facing_ = 2;  // Left
            else if (dx == 1)             facing_ = 3;  // Right
            // else: dx=dy=0 (no input); keep current facing
            mv.toX = nx;
            mv.toY = ny;
            mv.progress = 1;
        }
        // else: stay put. Not a death, not an error, not a retry — holding
        // a direction into a blocked cell is inert.
    }

    updateInterpolatedPosition();
    return bombPlaced;
}

bool PlayerActor::canEnter(int nx, int ny, const TileType (&board)[kCells],
                            const Bomb (&bombs)[kMaxBombs]) const {
    if (!gameplay::containsCell(nx, ny, kBoardGrid)) {
        return false;
    }
    const TileType t = board[cellIndex(nx, ny)];
    if (t == TileType::HardWall || isSoftWall(t)) {
        return false;
    }
    if (bombAt(bombs, nx, ny)) {
        // A bomb blocks unless it is the one cell the player is currently
        // exempt from. The exemption clears the instant the player's
        // logical cell changes (see onArrive() above), so this can only
        // ever admit the cell the player is standing on right now, never a
        // bomb anywhere else.
        return exemptValid_ && nx == exemptX_ && ny == exemptY_;
    }
    return true;
}

bool PlayerActor::tryPlaceBomb(Bomb (&bombs)[kMaxBombs]) {
    if (activeBombCount(bombs) >= maxBombs_) {
        return false;
    }
    if (!placeBomb(bombs, mv.cellX, mv.cellY, firePower_)) {
        return false;
    }
    // The bomb was just written into (mv.cellX, mv.cellY) above; exempt
    // that same cell so the two can never disagree.
    exemptValid_ = true;
    exemptX_ = mv.cellX;
    exemptY_ = mv.cellY;
    return true;
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
    const PlayerWalkFrame frame = playerWalkSpriteFor(facing_, mv.progress);
    renderer.drawSprite(*frame.sprite, x, y, 0, frame.flipX);
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
    exemptValid_ = false;
    exemptX_ = 0;
    exemptY_ = 0;
    updateInterpolatedPosition();
}

}  // namespace bomberman
