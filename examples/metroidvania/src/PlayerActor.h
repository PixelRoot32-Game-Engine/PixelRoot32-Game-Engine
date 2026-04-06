#pragma once
#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "physics/KinematicActor.h"
#include "GameConstants.h"
#include "GameLayers.h"

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
    void setInput(float dir, float vDir, bool jumpPressed);

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

private:
    unsigned long timeAccumulator = 0;
    uint8_t currentFrame = 0;
    PlayerState currentState = PlayerState::IDLE;

    float moveDir = 0.0f;       ///< Horizontal direction (-1, 0, 1)
    float verticalDir = 0.0f;   ///< Vertical direction for ladders
    bool wantsJump = false;     ///< Jump intent flag
    bool onGround = false;      ///< Ground contact flag
    bool facingLeft = false;    ///< Sprite orientation

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
};

} // namespace metroidvania

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES // namespace metroidvania
