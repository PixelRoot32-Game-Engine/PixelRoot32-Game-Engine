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
    enterRoom(kRooms[0], startTileX, startTileY);
}

void HeroActor::enterRoom(const RoomSpec& room, int startTileX, int startTileY) {
    room_ = &room;
    gameplay::placeAt(motion_, startTileX, startTileY);

    // Turn to face into the room. The step from the door to the tile the hero
    // is standing on is exactly the direction it just walked, so reusing the
    // walking rule gets both the pose and the mirror right without a second
    // table of facings to keep in sync.
    //
    // Doors only exist on the two back walls, so this step is always +x or +y
    // and the resulting pose always faces the camera -- which is the whole
    // point. `everyArrivalFacesInward` asserts at compile time that no arrival
    // tile falls back to {0, 0} here.
    // The one caller that legitimately gets {0, 0} is the constructor: the
    // spawn tile sits in the middle of room 0, nowhere near a door. It keeps
    // the member default, which already faces down-right toward the camera --
    // so the "always arrive facing forward" rule holds on that path too,
    // without a special case.
    const CellStep inward = inwardStepFromDoor(room, startTileX, startTileY);
    if (inward.dx != 0 || inward.dy != 0) {
        setFacingFromStep(inward.dx, inward.dy);
    }

    updateProjectedPosition();

    // The hero has jumped across the screen, so whatever drawn-appearance
    // record it was carrying describes a frame from another room. Raising the
    // flag by hand rather than letting refreshVisualDirty() notice is not
    // belt-and-braces: a door can land the hero on a cell that projects to the
    // SAME pixel and pose it left from, and then nothing would look different
    // to compare -- on a frame where the entire background changed.
    visualDirty_ = true;
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
    setFacingFromStep(dx, dy);

    const int nextX = motion_.cellX + dx;
    const int nextY = motion_.cellY + dy;

    // isSolidTile() reports out-of-room as solid, so the room's edge holds
    // without a ring of blocker tiles around the layout.
    if (!isSolidTile(*room_, nextX, nextY)) {
        gameplay::beginStep(motion_, nextX, nextY);
    }
    // else: stay put. Holding a direction into a wall is inert -- not a
    // collision event, not a retry, not an error.
}

void HeroActor::setFacingFromStep(int dx, int dy) {
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
    refreshVisualDirty();
}

uint8_t HeroActor::spriteFrame() const {
    // The walk cycle is driven by step progress rather than by a timer of its
    // own: one full stride per tile, in sync with the movement by
    // construction. A separate animation clock would drift against the logic
    // clock and land the hero on a cell mid-stride.
    const uint8_t phase =
        static_cast<uint8_t>((motion_.progress * 2 / kStepsPerCell) & 1);
    return static_cast<uint8_t>((facingAway_ ? HERO_AWAY_0 : HERO_TOWARD_0) + phase);
}

void HeroActor::refreshVisualDirty() {
    // Compared in the same integer screen coordinates drawAtCell() receives,
    // not in Scalar: two positions that round to the same pixel produce the
    // same frame, and re-sending it would cost a full SPI push to change
    // nothing.
    if (visualDirty_) {
        return;
    }
    if (static_cast<int>(position.x) != drawnX_ ||
        static_cast<int>(position.y) != drawnY_ ||
        spriteFrame() != drawnFrame_ ||
        flipX_ != drawnFlipX_) {
        visualDirty_ = true;
    }
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
    const uint8_t frame = spriteFrame();
    const int screenX = static_cast<int>(position.x);
    const int screenY = static_cast<int>(position.y);

    drawAtCell(renderer, HERO_FRAMES[frame], HERO_FOOT_Y, screenX, screenY, flipX_);

    // Record what was actually put on screen. Clearing the flag here rather
    // than in update() is what makes a skipped frame safe: if the engine never
    // called this, the flag is still standing next frame.
    drawnX_ = screenX;
    drawnY_ = screenY;
    drawnFrame_ = frame;
    drawnFlipX_ = flipX_;
    visualDirty_ = false;
}

}  // namespace iso_dungeon
