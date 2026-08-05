#pragma once
#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "physics/KinematicActor.h"
#include "GameConstants.h"
#include "GameLayers.h"
#include "gameplay/StateMachine.h"

namespace metroidvania {

/**
 * @enum PlayerState
 * @brief Player animation and movement state.
 */
enum class PlayerState {
    IDLE,
    RUN,
    JUMP,
    CLIMBING
};

/**
 * @brief PlayerActor class representing the main protagonist.
 * Inherits from PhysicsActor to utilize basic physics properties.
 */
class PlayerActor : public pixelroot32::physics::KinematicActor {
public:
    PlayerActor(pixelroot32::math::Vector2 position);

    /** @brief Updates player logic every frame. */
    void update(unsigned long deltaTime) override;

    /** @brief Draws the current state sprite. */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /** @brief Defines the player's collision area. */
    pixelroot32::core::Rect getHitBox() override;

    /** @brief Handles collisions with other actors (enemies, items, etc). */
    void onCollision(pixelroot32::core::Actor* other) override;

    /** @brief Updates input state received from the scene. */
    void setInput(pixelroot32::math::Scalar dir, pixelroot32::math::Scalar vDir, bool jumpPressed);

    /**
     * @brief Assigns stairs layer data from the map.
     * @param indices Tile indices (PROGMEM on ESP32).
     * @param width Map width in tiles.
     * @param height Map height in tiles.
     * @param tileSize Tile size in pixels.
     */
    void setStairs(const uint8_t* indices, int width, int height, int tileSize);

    /** @brief Builds RAM cache of stairs mask. Call once after setStairs. */
    void buildStairsCache();

    /**
     * @brief Returns true if the player just respawned from a void fall, and clears the flag.
     * @return true if a respawn just occurred this frame, false otherwise.
     */
    bool consumeRespawnFlag() {
        bool val = justRespawned;
        justRespawned = false;
        return val;
    }

private:
    /// Bound to a caller-owned, static const table (see .cpp) whose onUpdate
    /// callbacks hold the actual IDLE/RUN/JUMP transition logic. Configured
    /// and started in the constructor.
    pixelroot32::gameplay::StateMachine stateMachine;

    // Animation timing stays explicit rather than derived from
    // stateMachine.getTimeInState(): once transitions are requested from
    // inside onUpdate (below), StateMachine::update() has already added this
    // frame's deltaTime to time-in-state BEFORE onUpdate runs (see
    // StateMachine.h's `update()` contract and Rule 5), so a same-frame
    // transition resets time-in-state to exactly 0, discarding that delta —
    // unlike the original code, which reset then added the delta, ending the
    // transition frame at `deltaTime`, not `0`. Keeping these fields and
    // resetting them from onEnterAnyState (fired synchronously by
    // requestState() regardless of call site) reproduces the original
    // ordering exactly; see PlayerActor.cpp for the full trace.
    unsigned long timeAccumulator = 0;
    uint8_t currentFrame = 0;

    pixelroot32::math::Scalar moveDir = pixelroot32::math::toScalar(0.0f);       ///< Horizontal direction (-1, 0, 1)
    pixelroot32::math::Scalar verticalDir = pixelroot32::math::toScalar(0.0f);   ///< Vertical direction for ladders
    bool wantsJump = false;     ///< Jump intent flag
    bool onGround = false;      ///< Ground contact flag
    bool facingLeft = false;    ///< Sprite orientation
    bool justRespawned = false; ///< Set on void-fall respawn, consumed by scene in same frame

    pixelroot32::math::Vector2 velocity;

    const uint8_t* stairsIndices = nullptr;
    int stairsWidth = 0;
    int stairsHeight = 0;
    int stairsTileSize = 0;

    static constexpr int STAIRS_CACHE_MAX_BYTES = (32 * 32 + 7) / 8;
    uint8_t stairsMask[STAIRS_CACHE_MAX_BYTES] = {};
    bool stairsMaskReady = false;

    /** @brief Checks if the player is overlapping a stairs area. */
    bool isOverlappingStairs() const;

    /** @brief Returns the number of frames for the current state. */
    int getNumberOfFramesByState() const;

    /** @brief Returns the sprite for the current state and frame. */
    pixelroot32::graphics::Sprite4bpp getSpriteByState() const;

    /** @brief Changes player state and resets animation if needed. */
    void changeState(PlayerState newState);

    /** @brief Current player state, read back from the state machine. */
    PlayerState currentState() const;

    // StateMachine::State callbacks (see the kPlayerStates table in the
    // .cpp). `owner` is always `this` — configure() binds it once in the
    // constructor. Static so they match EnterFn/UpdateFn's C function
    // pointer signatures; each recovers the instance via
    // `static_cast<PlayerActor*>(owner)`.

    /** @brief Shared onEnter for every row: resets animation timing, exactly
     *  what the old `changeState()` did on every real transition. */
    static void onEnterAnyState(void* owner, pixelroot32::gameplay::StateId fromState);

    /** @brief IDLE -> JUMP (airborne) or RUN (moving), mirrors the old switch. */
    static void onUpdateIdle(void* owner, unsigned long deltaTime, uint32_t timeInStateMs);

    /** @brief RUN -> JUMP (airborne) or IDLE (stopped), mirrors the old switch. */
    static void onUpdateRun(void* owner, unsigned long deltaTime, uint32_t timeInStateMs);

    /** @brief JUMP -> RUN or IDLE on landing, mirrors the old switch. */
    static void onUpdateJump(void* owner, unsigned long deltaTime, uint32_t timeInStateMs);

    /// State table bound in the constructor. Defined out-of-line in the .cpp
    /// (class-static, not namespace-scope, so its rows can take the address
    /// of the private callbacks above) — still `static const`, so it lands
    /// in flash/.rodata per StateMachine.h's documented convention.
    static const pixelroot32::gameplay::StateMachine::State kPlayerStates[4];
};

} // namespace metroidvania

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES // namespace metroidvania
