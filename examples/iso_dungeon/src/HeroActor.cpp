#include "HeroActor.h"

#include "core/Engine.h"
#include "math/Vector2.h"
#include "IsoDraw.h"
#include "assets/HeroSprites.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace iso_dungeon {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

HeroActor::HeroActor(int startTileX, int startTileY)
    : core::Entity(math::Vector2::ZERO(), HERO_WIDTH, HERO_HEIGHT,
                   core::EntityType::GENERIC) {
    setRenderLayer(1);
    gameplay::placeAt(motion_, startTileX, startTileY);
    updateProjectedPosition();
}

void HeroActor::logicStep() {
    if (gameplay::isMoving(motion_)) {
        // A step is already in flight: finish it and ignore input until it
        // lands. That refusal is what makes the movement a board rather than a
        // walk -- the hero cannot be redirected mid-tile, so it can never come
        // to rest anywhere but on a cell.
        gameplay::tickStep(motion_, kStepsPerCell);
        return;
    }

    auto& input = engine.getInputManager();

    // At rest, held input may start a step. Fixed priority over the buttons
    // currently down, so exactly one (dx, dy) with |dx| + |dy| == 1 is ever
    // chosen: diagonals are unrepresentable by construction.
    //
    // No remapping is needed for the isometric view, and that is worth being
    // precise about rather than calling it lucky. Under kTileProjection the
    // cell axes project to the four screen DIAGONALS -- +cellX goes
    // down-right, +cellY down-left -- so the four buttons already cover the
    // four directions a player can see. A game with a different basis would
    // have to decide this again; the engine takes no position on it.
    int dx = 0;
    int dy = 0;
    if (input.isButtonDown(BTN_UP)) {
        dy = -1;
    } else if (input.isButtonDown(BTN_DOWN)) {
        dy = 1;
    } else if (input.isButtonDown(BTN_LEFT)) {
        dx = -1;
    } else if (input.isButtonDown(BTN_RIGHT)) {
        dx = 1;
    }

    if (dx == 0 && dy == 0) {
        return;
    }

    // Facing is updated even when the step is refused, so holding a direction
    // into a wall turns the hero to face it instead of leaving it pointing the
    // way it last managed to move.
    if (dx > 0) {
        facingAway_ = false;
        flipX_ = false;   // down-right
    } else if (dy > 0) {
        facingAway_ = false;
        flipX_ = true;    // down-left
    } else if (dx < 0) {
        facingAway_ = true;
        flipX_ = true;    // up-left
    } else {
        facingAway_ = true;
        flipX_ = false;   // up-right
    }

    const int nextX = motion_.cellX + dx;
    const int nextY = motion_.cellY + dy;

    // isSolidTile() reports out-of-room as solid, so the room's edge holds
    // without a ring of blocker tiles around the layout.
    if (!isSolidTile(nextX, nextY)) {
        gameplay::beginStep(motion_, nextX, nextY);
    }
    // else: stay put. Holding a direction into a wall is inert -- not a
    // collision event, not a retry, not an error.
}

void HeroActor::update(unsigned long deltaTime) {
    // Fixed logic clock. A `while` rather than a single step so a long frame
    // catches up instead of silently slowing the hero down; the accumulator
    // keeps the remainder so no travel is lost or invented.
    logicAccumulator_ += deltaTime;
    while (logicAccumulator_ >= kLogicStepMs) {
        logicAccumulator_ -= kLogicStepMs;
        logicStep();
    }
    updateProjectedPosition();
}

void HeroActor::updateProjectedPosition() {
    // The one line in this class that knows the view is isometric.
    position = gameplay::interpolatedWorld(motion_, kStepsPerCell, kTileProjection);

#if PIXELROOT32_ENABLE_DEPTH_SORT
    // Paint order is the projected screen Y of the hero's feet, updated every
    // frame because it changes continuously across a step -- the hero really
    // does pass behind the altar and then in front of it.
    //
    // Ordering by world Y -- what gameplay::compareByBottomY does -- would be
    // wrong here: cells (2,0) and (0,2) sit on the same screen row at
    // completely different cell-space Ys, so the hero could occlude something
    // it is standing behind. Screen Y happens to BE the isometric depth for
    // this spec, because cellToScreenY reduces to originY + 8 * (x + y) when
    // both cell axes share a vertical component. That identity is a property
    // of this basis, not of projections in general, which is exactly why the
    // engine takes the key as data instead of deriving it.
    depthKey = static_cast<int16_t>(position.y);
#endif
}

void HeroActor::draw(gfx::Renderer& renderer) {
    // The walk cycle is driven by step progress rather than by a timer of its
    // own: one full stride per tile, in sync with the movement by
    // construction. A separate animation clock would drift against the logic
    // clock and land the hero on a cell mid-stride.
    const uint8_t phase =
        static_cast<uint8_t>((motion_.progress * 2 / kStepsPerCell) & 1);
    const uint8_t frame =
        static_cast<uint8_t>((facingAway_ ? HERO_AWAY_0 : HERO_TOWARD_0) + phase);

    drawAtCell(renderer, HERO_FRAMES[frame], HERO_FOOT_Y,
               static_cast<int>(position.x), static_cast<int>(position.y), flipX_);
}

}  // namespace iso_dungeon
