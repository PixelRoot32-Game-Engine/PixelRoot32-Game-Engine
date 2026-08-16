#include "Player.h"
#include "core/Engine.h"
#include "physics/TileAttributes.h"
#include "physics/TilePixelCollision.h"
#include "assets/roomscreen_main_scene.h"
#include "assets/PlayerSprites.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace room_screen {

namespace gfx = pr32::graphics;
namespace math = pr32::math;
namespace scene = roomscreen::main_scene;

Player::Player(int startX, int startY)
    : Entity(math::Vector2(startX, startY), kPlayerSize, kPlayerSize,
             pr32::core::EntityType::GENERIC)
    , pixelX_(startX)
    , pixelY_(startY) {
}

bool Player::canOccupy(int x, int y) const {
    if constexpr (kCollisionMode == CollisionMode::WholeTile) {
        // Original whole-tile check: any overlapped tile flagged TILE_SOLID
        // blocks the box, its bitmap ignored. The far edge is (x + size - 1)
        // so a box flush against a wall does not test the tile it stops short
        // of.
        const int leftCol   = x / scene::TILE_SIZE;
        const int rightCol  = (x + kPlayerSize - 1) / scene::TILE_SIZE;
        const int topRow    = y / scene::TILE_SIZE;
        const int bottomRow = (y + kPlayerSize - 1) / scene::TILE_SIZE;

        // The edge of the map is a wall, independent of the tile data.
        // getTileFlags returns TILE_NONE out-of-bounds, so without this check
        // the player would walk off the world.
        if (leftCol < 0 || rightCol >= scene::MAP_WIDTH ||
            topRow < 0 || bottomRow >= scene::MAP_HEIGHT) {
            return false;
        }

        for (int row = topRow; row <= bottomRow; ++row) {
            for (int col = leftCol; col <= rightCol; ++col) {
                if (scene::getTileFlags(kItemsBehaviorLayer, col, row) & pr32::physics::TILE_SOLID) {
                    return false;
                }
            }
        }
        return true;
    }

    // Per-pixel path. Erosion is applied to the *object*, not the player, so
    // the full 16x16 hitbox slides past thin branches and stray pixels instead
    // of penetrating into the object and snagging on irregular silhouettes
    // (plants, tree canopies).
    constexpr int erosion = (kCollisionMode == CollisionMode::PerPixelEroded)
                                ? kTileSolidErosionPx
                                : 0;

    // The edge of the map is a wall, independent of the tile data. Without
    // this explicit guard isWorldPixelSolid would treat OOB as non-solid
    // (walkable) and the player could walk off the world.
    if (x < 0 || y < 0 ||
        x + kPlayerSize > scene::MAP_WIDTH  * scene::TILE_SIZE ||
        y + kPlayerSize > scene::MAP_HEIGHT * scene::TILE_SIZE) {
        return false;
    }

    // isWorldPixelSolid short-circuits on TILE_NONE so non-solid tiles pay
    // only a flag lookup.
    for (int py = 0; py < kPlayerSize; ++py) {
        for (int px = 0; px < kPlayerSize; ++px) {
            const int worldX = x + px;
            const int worldY = y + py;
            const int tileX  = worldX / scene::TILE_SIZE;
            const int tileY  = worldY / scene::TILE_SIZE;
            const int localX = worldX % scene::TILE_SIZE;
            const int localY = worldY % scene::TILE_SIZE;
            if (pr32::physics::isWorldPixelSolid(
                    &scene::items,
                    scene::behavior_layers[kItemsBehaviorLayer],
                    tileX, tileY,
                    localX, localY,
                    pr32::physics::TILE_SOLID,
                    erosion)) {
                return false;
            }
        }
    }
    return true;
}

void Player::stepAxis(int steps, int deltaX, int deltaY) {
    // One pixel at a time so the player ends up flush against a wall instead of
    // stopping a step short of it.
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

void Player::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();

    const bool up    = input.isButtonDown(BTN_UP);
    const bool down  = input.isButtonDown(BTN_DOWN);
    const bool left  = input.isButtonDown(BTN_LEFT);
    const bool right = input.isButtonDown(BTN_RIGHT);

    int deltaX = 0;
    int deltaY = 0;

    // Single axis, horizontal wins.
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

    moving_ = (deltaX != 0 || deltaY != 0);

    if (!moving_) {
        // Standing still must not bank travel or keep the walk cycle ticking,
        // or the first step after a pause would fire off several pixels at once.
        travelAccumulator_ = 0;
        walkTimer_ = 0;
        walkFrame_ = 0;
        return;
    }

    walkTimer_ += deltaTime;
    while (walkTimer_ >= kWalkFrameMs) {
        walkTimer_ -= kWalkFrameMs;
        walkFrame_ ^= 1;
    }

    travelAccumulator_ += static_cast<int>(deltaTime) * kPlayerSpeedPxPerSec;
    const int steps = travelAccumulator_ / 1000;
    travelAccumulator_ -= steps * 1000;

    if (steps > 0) {
        stepAxis(steps, deltaX, deltaY);
        syncEntityPosition();
    }
}

void Player::draw(gfx::Renderer& renderer) {
    uint8_t frame = PLAYER_DOWN_IDLE;
    bool flipX = false;

    if (moving_) {
        switch (facing_) {
            case Facing::Down:
                // One bitmap; the mirror is the second walk frame.
                frame = PLAYER_DOWN_WALK;
                flipX = (walkFrame_ != 0);
                break;
            case Facing::Up:
                frame = PLAYER_UP_WALK;
                flipX = (walkFrame_ != 0);
                break;
            case Facing::Right:
                frame = walkFrame_ ? PLAYER_SIDE_WALK_B : PLAYER_SIDE_WALK_A;
                break;
            case Facing::Left:
                // The flip here is the facing, so the walk frame comes from two
                // real bitmaps: mirroring a side-on pose turns the hero around.
                frame = walkFrame_ ? PLAYER_SIDE_WALK_B : PLAYER_SIDE_WALK_A;
                flipX = true;
                break;
        }
    } else {
        switch (facing_) {
            case Facing::Down:  frame = PLAYER_DOWN_IDLE; break;
            case Facing::Up:    frame = PLAYER_UP_IDLE; break;
            case Facing::Right: frame = PLAYER_SIDE_IDLE; break;
            case Facing::Left:  frame = PLAYER_SIDE_IDLE; flipX = true; break;
        }
    }

    renderer.drawSprite(PLAYER_FRAMES[frame], pixelX_, pixelY_, flipX);
}

void Player::setPixelPosition(int x, int y) {
    pixelX_ = x;
    pixelY_ = y;
    travelAccumulator_ = 0;
    syncEntityPosition();
}

void Player::syncEntityPosition() {
    position = math::Vector2(pixelX_, pixelY_);
}

} // namespace room_screen
