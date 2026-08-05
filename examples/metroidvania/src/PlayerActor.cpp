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
using Scalar = pixelroot32::math::Scalar;
namespace math = pixelroot32::math;
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

// Player state table for pixelroot32::gameplay::StateMachine. onUpdate holds
// the actual per-state transition logic that used to be a hand-written
// switch in update() (IDLE/RUN/JUMP); onEnter is shared across every row and
// resets animation timing on every real transition, reproducing the old
// changeState()'s side effect. CLIMBING's onUpdate is deliberately null: it
// is managed externally by ladder presence and vertical input in section 1
// of update(), never by a per-frame condition here. Defined class-static
// (not namespace-scope) so its rows can take the address of the private
// callback member functions — still `static const`, so it lands in
// flash/.rodata per StateMachine.h's documented convention.
const pixelroot32::gameplay::StateMachine::State PlayerActor::kPlayerStates[4] = {
    { &PlayerActor::onEnterAnyState, &PlayerActor::onUpdateIdle, nullptr, static_cast<pixelroot32::gameplay::StateId>(PlayerState::IDLE) },
    { &PlayerActor::onEnterAnyState, &PlayerActor::onUpdateRun,  nullptr, static_cast<pixelroot32::gameplay::StateId>(PlayerState::RUN) },
    { &PlayerActor::onEnterAnyState, &PlayerActor::onUpdateJump, nullptr, static_cast<pixelroot32::gameplay::StateId>(PlayerState::JUMP) },
    { &PlayerActor::onEnterAnyState, nullptr,                    nullptr, static_cast<pixelroot32::gameplay::StateId>(PlayerState::CLIMBING) },
};

PlayerActor::PlayerActor(pixelroot32::math::Vector2 position)
    : KinematicActor(position, PLAYER_WIDTH, PLAYER_HEIGHT) {
    setRenderLayer(2);
    setCollisionLayer(Layers::PLAYER);
    setCollisionMask(Layers::ENEMY | Layers::GROUND | Layers::PLATFORM);

    stateMachine.configure(this, kPlayerStates, sizeof(kPlayerStates) / sizeof(kPlayerStates[0]));
    stateMachine.start(static_cast<pixelroot32::gameplay::StateId>(PlayerState::IDLE));
}

void PlayerActor::setInput(math::Scalar dir, math::Scalar vDir, bool jumpPressed) {
    moveDir = dir;
    verticalDir = vDir;
    if (jumpPressed) {
        // If climbing, jump releases the ladder
        if (currentState() == PlayerState::CLIMBING) {
            changeState(PlayerState::JUMP);
            velocity.y = math::toScalar(-PLAYER_JUMP_VELOCITY * 0.8f);  // NOLINT(readability-magic-numbers)
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
    
    auto centerX = position.x + math::toScalar(width / 2);
    int col = static_cast<int>(centerX) / stairsTileSize;
    
    if (col < 0 || col >= stairsWidth) return false;

    // Check points: head, center, feet and slightly below feet.
    auto posY = position.y;
    math::Scalar yPoints[] = { 
        posY, 
        posY + math::toScalar(height / 2), 
        posY + math::toScalar(height - 1), 
        posY + math::toScalar(height + 2) 
    };
    for (auto py : yPoints) {
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
    math::Scalar dt = math::toScalar(deltaTime) * math::toScalar(0.001f);
    if (dt > math::toScalar(0.05f)) dt = math::toScalar(0.05f); // Cap to avoid large jumps due to lag

    bool overlappingStairs = isOverlappingStairs();

    // 1. STATE MACHINE TRANSITIONS (Before applying movements/masks)
    if (currentState() == PlayerState::CLIMBING) {
        // Drop off the ladder if we moved outside the stairs area
        if (!overlappingStairs) {
            changeState(PlayerState::IDLE);
        }
        // Drop off the ladder if we hit the floor and are not climbing up
        else if (is_on_floor() && verticalDir >= math::toScalar(0.0f)) {
            changeState(PlayerState::IDLE);
        }
    } else {
        // Try to enter climbing mode
        if (overlappingStairs && verticalDir != math::toScalar(0.0f)) {
            bool canClimbDown = false;
            
            // Allow climbing down only if we are on the floor AND there are stairs physically below us
            if (verticalDir > math::toScalar(0) && is_on_floor()) {
                auto checkY = position.y + math::toScalar(height + 2);
                int checkCol = static_cast<int>(position.x + math::toScalar(width / 2)) / stairsTileSize;
                int checkRow = static_cast<int>(checkY) / stairsTileSize;
                if (checkCol >= 0 && checkCol < stairsWidth && checkRow >= 0 && checkRow < stairsHeight) {
                    const int idx = checkCol + checkRow * stairsWidth;
                    canClimbDown = stairsMaskReady ? ((stairsMask[idx >> 3] & (1u << (idx & 7))) != 0) : (stairsIndices[idx] != 0);
                }
            }
            
            bool canClimbUp = (verticalDir < math::toScalar(0)); // Climb up from ground or air

            if (canClimbUp || canClimbDown) {
                changeState(PlayerState::CLIMBING);
                
                // Auto-center player on the ladder
                int col = static_cast<int>(position.x + math::toScalar(width / 2)) / stairsTileSize;
                position.x = math::toScalar(col * stairsTileSize + (stairsTileSize - width) / 2);
                
                // If climbing down through a platform, give a small nudge to bypass the initial floor collision
                if (canClimbDown) {
                    position.y += math::toScalar(4); 
                }
            }
        }
    }

    // 2. MOVEMENT & COLLISION MASK LOGIC

    // Capture jump intent before it is consumed by the wantsJump check below
    bool jumpThisFrame = wantsJump;

    if (currentState() == PlayerState::CLIMBING) {
        // Lock lateral movement to keep the character firmly on the ladder rail
        velocity.x = math::toScalar(0); 
        velocity.y = math::toScalar(verticalDir * PLAYER_CLIMB_SPEED);
        
        pixelroot32::physics::CollisionLayer climbMask = Layers::ENEMY; 
        
        // Are we moving down and about to hit the bottom of the ladder?
        if (velocity.y > math::toScalar(0)) {
            // Check if there are stairs right below our feet
            auto py = position.y + math::toScalar(height + 2); 
            auto px = position.x + math::toScalar(width / 2);
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
        velocity.x = math::toScalar(moveDir * PLAYER_MOVE_SPEED);
        velocity.y += math::toScalar(PLAYER_GRAVITY * dt); // Apply gravity
        
        // Dynamic Mask Logic:
        // Ignore platforms (ceilings) when moving up, so we can jump through them
        if (velocity.y < math::toScalar(0)) {
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
                velocity.y = math::toScalar(-PLAYER_JUMP_VELOCITY);
            }
            wantsJump = false; // Always consume input
        }
    }

    // Sprite orientation
    if (moveDir > math::toScalar(0.0f)) facingLeft = false;
    else if (moveDir < math::toScalar(0.0f)) facingLeft = true;

    // Compute snap: disabled during CLIMBING (incompatible with ladder logic)
    // and on jump frame. Active otherwise for floor adhesion.
    math::Vector2 snap = (currentState() == PlayerState::CLIMBING || jumpThisFrame)
        ? math::Vector2{}
        : math::Vector2(math::toScalar(0), KinematicActor::MIN_SNAP);

    velocity = moveAndSlide(velocity, dt, math::Vector2(0, -1), pixelroot32::physics::SnapPolicy::Step, snap);

    // Keep wall-stop zeroing — moveAndSlide doesn't zero wall velocity in return
    if (is_on_wall()) {
        velocity.x = math::toScalar(0);
    }

    // World bounds (screen edges)
    // Lazy initialization of world size (if not defined)
    if (worldHeight == 0) {
        setWorldSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    if (worldWidth > 0) {
        if (position.x < math::toScalar(0)) {
            position.x = math::toScalar(0);
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
            justRespawned = true;
        }
    }

    // State machine dispatch: replaces the old hand-written switch. The
    // current state's onUpdate callback (kPlayerStates, above) issues the
    // same requestState() calls the switch used to — at most one transition
    // is dispatched here, and per StateMachine.h Rule 5 a transition
    // requested inline from onUpdate does not also run the newly entered
    // state's onUpdate this same call, exactly matching the old switch's
    // single-transition-per-frame behavior. CLIMBING's onUpdate is null, so
    // this is a no-op while climbing, same as the switch's empty case.
    stateMachine.update(deltaTime);

    // Animation frame update, unchanged from the original code and run in
    // the same relative position: any transition requested by onUpdate just
    // above already reset currentFrame/timeAccumulator via onEnterAnyState
    // (fired synchronously and inline by requestState()), so accumulating
    // deltaTime here reproduces "reset, then add" exactly as `changeState()`
    // followed by this same increment used to.
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
    // requestState() is itself a no-op when newState matches the current
    // state (StateMachine.h Rule 1), so the guard the old code had here is
    // no longer needed.
    stateMachine.requestState(static_cast<pixelroot32::gameplay::StateId>(newState));
}

PlayerState PlayerActor::currentState() const {
    return static_cast<PlayerState>(stateMachine.getCurrentState());
}

void PlayerActor::onEnterAnyState(void* owner, pixelroot32::gameplay::StateId fromState) {
    (void)fromState;
    auto* self = static_cast<PlayerActor*>(owner);
    self->currentFrame = 0;
    self->timeAccumulator = 0;
}

void PlayerActor::onUpdateIdle(void* owner, unsigned long deltaTime, uint32_t timeInStateMs) {
    (void)deltaTime;
    (void)timeInStateMs;
    auto* self = static_cast<PlayerActor*>(owner);
    if (!self->is_on_floor()) {
        self->changeState(PlayerState::JUMP);
    } else if (self->moveDir != math::toScalar(0.0f)) {
        self->changeState(PlayerState::RUN);
    }
}

void PlayerActor::onUpdateRun(void* owner, unsigned long deltaTime, uint32_t timeInStateMs) {
    (void)deltaTime;
    (void)timeInStateMs;
    auto* self = static_cast<PlayerActor*>(owner);
    if (!self->is_on_floor()) {
        self->changeState(PlayerState::JUMP);
    } else if (self->moveDir == math::toScalar(0.0f)) {
        self->changeState(PlayerState::IDLE);
    }
}

void PlayerActor::onUpdateJump(void* owner, unsigned long deltaTime, uint32_t timeInStateMs) {
    (void)deltaTime;
    (void)timeInStateMs;
    auto* self = static_cast<PlayerActor*>(owner);
    if (self->is_on_floor()) {
        self->changeState(self->moveDir != math::toScalar(0.0f) ? PlayerState::RUN : PlayerState::IDLE);
    }
}

int PlayerActor::getNumberOfFramesByState() const {
    switch (currentState()) {
        case PlayerState::IDLE:     return NUM_IDLE_FRAMES;
        case PlayerState::RUN:      return NUM_RUN_FRAMES;
        case PlayerState::JUMP:     return NUM_JUM_FRAMES;
        case PlayerState::CLIMBING: return NUM_RUN_FRAMES;
    }
    return NUM_IDLE_FRAMES;
}

Sprite4bpp PlayerActor::getSpriteByState() const {
    switch (currentState()) {
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
