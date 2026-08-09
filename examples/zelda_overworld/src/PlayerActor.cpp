#include "PlayerActor.h"

#include "core/Engine.h"
#include "TileFormat.h"
#include "assets/LinkSprites.h"
#include "assets/OverworldMap.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace zelda_overworld {

namespace gfx = pr32::graphics;
namespace math = pr32::math;

PlayerActor::PlayerActor(int startCol, int startRow)
    : Entity(math::Vector2(startCol * kTileSize, startRow * kTileSize),
             kPlayerSize, kPlayerSize,
             pr32::core::EntityType::GENERIC)
    , pixelX_(startCol * kTileSize)
    , pixelY_(startRow * kTileSize) {
}

bool PlayerActor::canOccupy(int x, int y) {
    // The box spans from its top-left to its last covered pixel, so the far
    // edge is (x + size - 1). Using (x + size) would test the tile the box
    // stops exactly short of and refuse legal positions flush against a wall.
    const int leftCol   = x / kTileSize;
    const int rightCol  = (x + kPlayerSize - 1) / kTileSize;
    const int topRow    = y / kTileSize;
    const int bottomRow = (y + kPlayerSize - 1) / kTileSize;

    for (int row = topRow; row <= bottomRow; ++row) {
        for (int col = leftCol; col <= rightCol; ++col) {
            if (isSolidCell(col, row)) {
                return false;
            }
        }
    }
    return true;
}

void PlayerActor::stepAxis(int steps, int deltaX, int deltaY) {
    // One pixel at a time so the player ends up flush against a wall instead of
    // stopping a step short of it. At 90 px/s a frame is one or two pixels, so
    // this loop costs at most a couple of tile queries.
    for (int i = 0; i < steps; ++i) {
        const int nextX = pixelX_ + deltaX;
        const int nextY = pixelY_ + deltaY;
        if (!canOccupy(nextX, nextY)) {
            return;
        }
        pixelX_ = nextX;
        pixelY_ = nextY;
    }
}

void PlayerActor::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();

    const bool up    = input.isButtonDown(BTN_UP);
    const bool down  = input.isButtonDown(BTN_DOWN);
    const bool left  = input.isButtonDown(BTN_LEFT);
    const bool right = input.isButtonDown(BTN_RIGHT);

    int deltaX = 0;
    int deltaY = 0;

    // Single axis, horizontal wins. The NES has no diagonal walk.
    if (left && !right) {
        deltaX = -1;
        facing_ = Facing::Left;
    } else if (right && !left) {
        deltaX = 1;
        facing_ = Facing::Right;
    } else if (up && !down) {
        deltaY = -1;
        facing_ = Facing::Up;
    } else if (down && !up) {
        deltaY = 1;
        facing_ = Facing::Down;
    }

    if (deltaX == 0 && deltaY == 0) {
        // Standing still must not bank travel, or the first step after a pause
        // would fire off several pixels at once.
        travelAccumulator_ = 0;
        return;
    }

    travelAccumulator_ += static_cast<int>(deltaTime) * kPlayerSpeedPxPerSec;
    const int steps = travelAccumulator_ / 1000;
    travelAccumulator_ -= steps * 1000;

    if (steps > 0) {
        stepAxis(steps, deltaX, deltaY);
        syncEntityPosition();
    }
}

void PlayerActor::draw(gfx::Renderer& renderer) {
    const SceneSprite* sprite = &PLAYER_DOWN;
    bool flipX = false;

    switch (facing_) {
        case Facing::Up:    sprite = &PLAYER_UP;   break;
        case Facing::Down:  sprite = &PLAYER_DOWN; break;
        case Facing::Right: sprite = &PLAYER_SIDE; break;
        case Facing::Left:  sprite = &PLAYER_SIDE; flipX = true; break;
    }

    drawSceneSprite(renderer, *sprite, pixelX_, pixelY_, gfx::Color::White, flipX);
}

void PlayerActor::setPixelPosition(int x, int y) {
    pixelX_ = x;
    pixelY_ = y;
    travelAccumulator_ = 0;
    syncEntityPosition();
}

void PlayerActor::syncEntityPosition() {
    position = math::Vector2(pixelX_, pixelY_);
}

} // namespace zelda_overworld
