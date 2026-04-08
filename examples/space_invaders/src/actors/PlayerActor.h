#pragma once
#include <physics/KinematicActor.h>
#include <graphics/Renderer.h>

namespace spaceinvaders {

/**
 * @class PlayerActor
 * @brief Player-controlled ship with discrete grid-based movement
 * 
 * Architecture Pattern:
 * This class demonstrates the KinematicActor usage pattern, where the entity
 * maintains direct control over its position while still participating in
 * collision detection. Unlike RigidActor which responds to physics forces,
 * KinematicActor allows precise programmatic positioning ideal for
 * player-controlled characters.
 * 
 * Grid Movement System:
 * The player moves between 9 discrete positions (0-8) distributed evenly
 * across the screen width. This provides tactical positioning control
 * reminiscent of arcade shooters, where precise lane control is essential.
 * 
 * Key Design Decisions:
 * 1. Discrete positions (not continuous) - Enables strategic positioning
 * 2. Interpolated movement - Smooth visual transition between grid cells
 * 3. Auto-repeat logic - Arcade-style input handling for rapid movement
 * 4. Easing curves - Visual polish with ease-out-cubic interpolation
 * 
 * Input Handling Strategy:
 * - Single press: Move one position in the pressed direction
 * - Hold (300ms delay): Begin auto-repeat movement
 * - Auto-repeat: 150ms interval between subsequent moves
 * 
 * Collision Integration:
 * Uses AABB collision shape with the Flat Solver physics system.
 * The kinematic nature means collisions are detected but don't push
 * the player ( Scene handles damage application via onCollision() ).
 */
class PlayerActor : public pixelroot32::physics::KinematicActor {
public:
    /**
     * @brief Constructs player at initial position, snapping to nearest grid cell
     * @param position Initial world position (will be quantized to grid)
     */
    PlayerActor(pixelroot32::math::Vector2 position);
    
    /**
     * @brief Main update tick - processes input and interpolates movement
     * 
     * System Flow:
     * 1. handleInput() - Detect button presses and manage auto-repeat
     * 2. updatePositionMovement() - Interpolate visual position between grid cells
     */
    void update(unsigned long deltaTime) override;
    
    /**
     * @brief Renders the player sprite at current interpolated position
     */
    void draw(pixelroot32::graphics::Renderer& renderer);
    
    /**
     * @brief Collision callback - Scene handles damage logic
     * 
     * Design Note: PlayerActor only detects collisions. Damage application,
     * life reduction, and game state changes are managed by SpaceInvadersScene
     * to maintain separation of concerns between actor behavior and game rules.
     */
    void onCollision(pixelroot32::core::Actor* other) override;

    /**
     * @brief Returns true if fire button is currently held
     * Used for continuous fire rate limiting
     */
    bool isFireDown() const;
    
    /**
     * @brief Returns true if fire button was pressed this frame
     * Used for single-shot actions (Edge detection)
     */
    bool wantsToShoot() const;

private:
    /**
     * @brief Processes directional input with auto-repeat logic
     * 
     * State Machine:
     * IDLE -> [Press Detected] -> SINGLE_MOVE -> [300ms held] -> AUTO_REPEAT
     * AUTO_REPEAT moves every 150ms until button released
     */
    void handleInput(unsigned long deltaTime);
    
    /**
     * @brief Interpolates visual position between grid cells
     * 
     * Uses ease-out-cubic easing for satisfying movement feel:
     * t = 1 - (1 - t)^3
     * 
     * This creates quick initial movement that settles smoothly at target,
     * giving weight and responsiveness to the arcade controls.
     */
    void updatePositionMovement(unsigned long deltaTime);
    
    /**
     * @brief Converts world X coordinate to grid position (0-8)
     * @return Grid index clamped to valid range
     */
    int calculatePositionFromX(pixelroot32::math::Scalar x) const;
    
    /**
     * @brief Calculates world X coordinate for a given grid position
     * 
     * Formula: usable_width / (NUM_POSITIONS - 1) * position
     * Distributes positions evenly while respecting screen boundaries
     */
    pixelroot32::math::Scalar calculateXFromPosition(int pos) const;
    
    /**
     * @brief Initiates movement to a new grid position
     * 
     * Validates bounds, sets target, and begins interpolation.
     * No effect if target position equals current position.
     */
    void moveToPosition(int newPosition);
    
    bool isAlive;
    
    // Grid Configuration
    static constexpr int NUM_POSITIONS = 9;                           // Total discrete positions
    static constexpr unsigned long MOVE_DURATION_MS = 100;            // Interpolation duration
    static constexpr unsigned long AUTO_REPEAT_DELAY_MS = 300;        // Delay before auto-repeat
    static constexpr unsigned long AUTO_REPEAT_INTERVAL_MS = 150;     // Auto-repeat frequency
    
    // Movement State
    int currentPosition;             // Occupied grid cell (0-8)
    int targetPosition;              // Destination grid cell
    bool isMoving;                   // True during interpolation
    unsigned long moveTimer;         // Accumulator for interpolation
    pixelroot32::math::Scalar startX;  // Position at movement start
    pixelroot32::math::Scalar targetX; // Final position after movement
    
    // Input State Tracking
    bool leftButtonPressed;          // Previous frame left button state
    bool rightButtonPressed;         // Previous frame right button state
    unsigned long autoRepeatTimer;   // Accumulator for auto-repeat timing
    bool autoRepeatActive;           // True when in auto-repeat mode
};

}
