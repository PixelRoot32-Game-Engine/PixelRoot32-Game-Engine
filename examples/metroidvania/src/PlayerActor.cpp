#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "PlayerActor.h"
#include "assets/PlayerIdleSprites.h"
#include "assets/PlayerRunSprites.h"
#include "assets/PlayerJumSprites.h"
#include "assets/PlayerPalette.h"
#include "platforms/EngineConfig.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "platforms/PlatformMemory.h"

namespace metroidvania {

namespace pr32 = pixelroot32;
using pr32::graphics::Sprite4bpp;

static const int NUM_IDLE_FRAMES = 2;
static const int NUM_RUN_FRAMES = 4;
static const int NUM_JUM_FRAMES = 5;

static const Sprite4bpp IDLE_FRAMES[] = {
    { reinterpret_cast<const uint8_t*>(PLAYER_IDLE_SPRITE_0_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_IDLE_SPRITE_1_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
};

static const Sprite4bpp RUN_FRAMES[] = {
    { reinterpret_cast<const uint8_t*>(PLAYER_RUN_SPRITE_0_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_RUN_SPRITE_1_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_RUN_SPRITE_2_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_RUN_SPRITE_3_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
};

static const Sprite4bpp JUM_FRAMES[] = {
    { reinterpret_cast<const uint8_t*>(PLAYER_JUM_SPRITE_0_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_JUM_SPRITE_1_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_JUM_SPRITE_2_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_JUM_SPRITE_3_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
    { reinterpret_cast<const uint8_t*>(PLAYER_JUM_SPRITE_4_4BPP), metroidvania::PLAYER_PALETTE_MAPPING, PLAYER_WIDTH, PLAYER_HEIGHT, 8 },
};

PlayerActor::PlayerActor(pixelroot32::math::Vector2 position)
    : KinematicActor(position, PLAYER_WIDTH, PLAYER_HEIGHT) {
    setRenderLayer(2);
    setCollisionLayer(Layers::PLAYER);
    setCollisionMask(Layers::ENEMY | Layers::GROUND | Layers::PLATFORM); 
}

void PlayerActor::setInput(float dir, float vDir, bool jumpPressed) {
    moveDir = dir;
    verticalDir = vDir;
    if (jumpPressed) {
        // If climbing, jump releases the ladder
        if (currentState == PlayerState::CLIMBING) {
            changeState(PlayerState::JUMP);
            velocity.y = pixelroot32::math::toScalar(-PLAYER_JUMP_VELOCITY * 0.8f);
        } else {
            wantsJump = true;
        }
    }
}

void PlayerActor::setStairs(const uint8_t* indices, int width, int height, int tileSize) {
    stairsIndices = indices;
    stairsWidth = width;
    stairsHeight = height;
    stairsTileSize = tileSize;
}

void PlayerActor::buildStairsCache() {
    if (!stairsIndices || stairsWidth <= 0 || stairsHeight <= 0) return;
    const int totalCells = stairsWidth * stairsHeight;
    if (totalCells > STAIRS_CACHE_MAX_BYTES * 8) return;

    for (int i = 0; i < STAIRS_CACHE_MAX_BYTES; ++i) stairsMask[i] = 0;

    for (int r = 0; r < stairsHeight; ++r) {
        for (int c = 0; c < stairsWidth; ++c) {
            const int idx = c + r * stairsWidth;
            uint8_t v = PIXELROOT32_READ_BYTE_P(stairsIndices + idx);
            if (v != 0) stairsMask[idx >> 3] |= (1u << (idx & 7));
        }
    }
    stairsMaskReady = true;
}

/**
 * @brief Checks if the player's center overlaps a stairs tile.
 * Multiple vertical points are checked to allow entering the stairs from top or bottom.
 */
bool PlayerActor::isOverlappingStairs() const {
    if (!stairsIndices && !stairsMaskReady) return false;
    
    float centerX = static_cast<float>(position.x) + width / 2.0f;
    int col = static_cast<int>(centerX) / stairsTileSize;
    
    if (col < 0 || col >= stairsWidth) return false;

    // Check points: head, center, feet and slightly below feet.
    float posY = static_cast<float>(position.y);
    float yPoints[] = { posY, posY + height / 2.0f, posY + height - 1.0f, posY + height + 2.0f };
    for (float py : yPoints) {
        int row = static_cast<int>(py) / stairsTileSize;
        if (row >= 0 && row < stairsHeight) {
            const int idx = col + row * stairsWidth;
            if (stairsMaskReady) {
                if (stairsMask[idx >> 3] & (1u << (idx & 7))) return true;
            } else {
                if (stairsIndices[idx] != 0) return true;
            }
        }
    }
    
    return false;
}

void PlayerActor::update(unsigned long deltaTime) {
    float dt = deltaTime * 0.001f;
    if (dt > 0.05f) dt = 0.05f; // Cap to avoid large jumps due to lag

    bool overlappingStairs = isOverlappingStairs();

    // 1. STATE MACHINE TRANSITIONS (Before applying movements/masks)
    if (currentState == PlayerState::CLIMBING) {
        // Drop off the ladder if we moved outside the stairs area
        if (!overlappingStairs) {
            changeState(PlayerState::IDLE);
        }
        // Drop off the ladder if we hit the floor and are not climbing up
        else if (is_on_floor() && verticalDir >= 0.0f) {
            changeState(PlayerState::IDLE);
        }
    } else {
        // Try to enter climbing mode
        if (overlappingStairs && verticalDir != 0.0f) {
            bool canClimbDown = false;
            
            // Allow climbing down only if we are on the floor AND there are stairs physically below us
            if (verticalDir > 0 && is_on_floor()) {
                float checkY = static_cast<float>(position.y) + height + 2.0f;
                int checkCol = static_cast<int>(position.x + width / 2.0f) / stairsTileSize;
                int checkRow = static_cast<int>(checkY) / stairsTileSize;
                if (checkCol >= 0 && checkCol < stairsWidth && checkRow >= 0 && checkRow < stairsHeight) {
                    const int idx = checkCol + checkRow * stairsWidth;
                    canClimbDown = stairsMaskReady ? ((stairsMask[idx >> 3] & (1u << (idx & 7))) != 0) : (stairsIndices[idx] != 0);
                }
            }
            
            bool canClimbUp = (verticalDir < 0); // Climb up from ground or air

            if (canClimbUp || canClimbDown) {
                changeState(PlayerState::CLIMBING);
                
                // Auto-center player on the ladder
                int col = static_cast<int>(position.x + width / pixelroot32::math::toScalar(2.0f)) / stairsTileSize;
                position.x = pixelroot32::math::toScalar(col * stairsTileSize + (stairsTileSize - width) / 2.0f);
                
                // If climbing down through a platform, give a small nudge to bypass the initial floor collision
                if (canClimbDown) {
                    position.y += pixelroot32::math::toScalar(4.0f); 
                }
            }
        }
    }

    // 2. MOVEMENT & COLLISION MASK LOGIC
    if (currentState == PlayerState::CLIMBING) {
        // Lock lateral movement to keep the character firmly on the ladder rail
        velocity.x = pixelroot32::math::toScalar(0); 
        velocity.y = pixelroot32::math::toScalar(verticalDir * PLAYER_CLIMB_SPEED);
        
        pixelroot32::physics::CollisionLayer climbMask = Layers::ENEMY; 
        
        // Are we moving down and about to hit the bottom of the ladder?
        if (velocity.y > pixelroot32::math::toScalar(0)) {
            // Check if there are stairs right below our feet
            float py = static_cast<float>(position.y) + height + 2.0f; 
            float px = static_cast<float>(position.x) + width / 2.0f;
            int col = static_cast<int>(px) / stairsTileSize;
            int row = static_cast<int>(py) / stairsTileSize;
            
            bool stairsBelow = false;
            if (col >= 0 && col < stairsWidth && row >= 0 && row < stairsHeight) {
                const int idx = col + row * stairsWidth;
                stairsBelow = stairsMaskReady ? ((stairsMask[idx >> 3] & (1u << (idx & 7))) != 0) : (stairsIndices[idx] != 0);
            }
            
            // If there are no stairs below, it means the ladder ends here.
            if (!stairsBelow) {
                climbMask |= Layers::GROUND | Layers::PLATFORM;
            }
        }
        
        setCollisionMask(climbMask);
    } else {
        velocity.x = pixelroot32::math::toScalar(moveDir * PLAYER_MOVE_SPEED);
        velocity.y += pixelroot32::math::toScalar(PLAYER_GRAVITY * dt); // Apply gravity
        
        // Dynamic Mask Logic:
        // Ignore platforms (ceilings) when moving up, so we can jump through them
        if (velocity.y < pixelroot32::math::toScalar(0)) {
            if (overlappingStairs) {
                // If moving up and overlapping stairs, ignore completely so we don't hit our head
                setCollisionMask(Layers::ENEMY);
            } else {
                // Moving up normally: ignore PLATFORM (One-Way), but collide with GROUND (solid ceilings)
                setCollisionMask(Layers::ENEMY | Layers::GROUND);
            }
        } else {
            // Falling down: collide with everything
            setCollisionMask(Layers::ENEMY | Layers::GROUND | Layers::PLATFORM);
        }

        if (wantsJump) {
            if (is_on_floor()) {
                velocity.y = pixelroot32::math::toScalar(-PLAYER_JUMP_VELOCITY);
            }
            wantsJump = false; // Always consume input
        }
    }

    // Sprite orientation
    if (moveDir > 0.0f) facingLeft = false;
    else if (moveDir < 0.0f) facingLeft = true;

    // Execute Move and Slide against the physics system
    // Using KinematicActor standard API for environmental collisions
    moveAndSlide(velocity * pixelroot32::math::toScalar(dt), pixelroot32::math::Vector2(0, -1));

    // Reset velocity on axis that collided
    if (is_on_floor() || is_on_ceiling()) {
        velocity.y = pixelroot32::math::toScalar(0);
    }
    if (is_on_wall()) {
        velocity.x = pixelroot32::math::toScalar(0);
    }

    // World bounds (screen edges)
    // Lazy initialization of world size (if not defined)
    if (worldHeight == 0) {
        setWorldSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    if (worldWidth > 0) {
        if (position.x < pixelroot32::math::toScalar(0)) {
            position.x = pixelroot32::math::toScalar(0);
            velocity.x = pixelroot32::math::toScalar(0);
        } else if (position.x + pixelroot32::math::toScalar(width) > pixelroot32::math::toScalar(worldWidth)) {
            position.x = pixelroot32::math::toScalar(worldWidth - width);
            velocity.x = pixelroot32::math::toScalar(0);
        }
    }
    
    if (worldHeight > 0) {
        if (position.y > pixelroot32::math::toScalar(worldHeight)) {
            // Die and respawn
            position.x = pixelroot32::math::toScalar(PLAYER_START_X);
            position.y = pixelroot32::math::toScalar(PLAYER_START_Y);
            velocity.x = pixelroot32::math::toScalar(0);
            velocity.y = pixelroot32::math::toScalar(0);
            changeState(PlayerState::IDLE);
        }
    }

    // State machine for animations
    PlayerState nextState = currentState;
    switch (currentState) {
        case PlayerState::IDLE:
            if (!is_on_floor()) nextState = PlayerState::JUMP;
            else if (moveDir != 0.0f) nextState = PlayerState::RUN;
            break;
        case PlayerState::RUN:
            if (!is_on_floor()) nextState = PlayerState::JUMP;
            else if (moveDir == 0.0f) nextState = PlayerState::IDLE;
            break;
        case PlayerState::JUMP:
            if (is_on_floor()) {
                nextState = (moveDir != 0.0f) ? PlayerState::RUN : PlayerState::IDLE;
            }
            break;
        case PlayerState::CLIMBING:
            // CLIMBING state is managed by ladder presence and vertical input
            break;
    }
    if (nextState != currentState) changeState(nextState);

    // Animation frame update
    timeAccumulator += deltaTime;
    while (timeAccumulator >= ANIMATION_FRAME_TIME_MS) {
        timeAccumulator -= ANIMATION_FRAME_TIME_MS;
        ++currentFrame;
        int numFrames = getNumberOfFramesByState();
        if (currentFrame >= numFrames) currentFrame = 0;
    }
}

void PlayerActor::draw(pr32::graphics::Renderer& renderer) {
    const auto& sprite = getSpriteByState();
    renderer.drawSprite(sprite, static_cast<int>(position.x), static_cast<int>(position.y), facingLeft);
}

pr32::core::Rect PlayerActor::getHitBox() {
    return { position, width, height };
}

void PlayerActor::onCollision(pr32::core::Actor* other) {
    (void)other;
}

void PlayerActor::changeState(PlayerState newState) {
    if (currentState != newState) {
        currentState = newState;
        currentFrame = 0;
        timeAccumulator = 0;
    }
}

int PlayerActor::getNumberOfFramesByState() const {
    switch (currentState) {
        case PlayerState::IDLE:     return NUM_IDLE_FRAMES;
        case PlayerState::RUN:      return NUM_RUN_FRAMES;
        case PlayerState::JUMP:     return NUM_JUM_FRAMES;
        case PlayerState::CLIMBING: return NUM_RUN_FRAMES;
    }
    return NUM_IDLE_FRAMES;
}

Sprite4bpp PlayerActor::getSpriteByState() const {
    switch (currentState) {
        case PlayerState::IDLE:
            return IDLE_FRAMES[currentFrame < NUM_IDLE_FRAMES ? currentFrame : 0];
        case PlayerState::RUN:
            return RUN_FRAMES[currentFrame < NUM_RUN_FRAMES ? currentFrame : 0];
        case PlayerState::JUMP:
            return JUM_FRAMES[currentFrame < NUM_JUM_FRAMES ? currentFrame : 0];
        case PlayerState::CLIMBING:
            return RUN_FRAMES[currentFrame < NUM_RUN_FRAMES ? currentFrame : 0];
    }
    return IDLE_FRAMES[0];
}

} // namespace metroidvania

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
