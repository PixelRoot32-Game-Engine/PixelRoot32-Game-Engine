#pragma once
#include <physics/RigidActor.h>
#include <graphics/Renderer.h>
#include <math/Scalar.h>

namespace spaceinvaders {

/**
 * @enum ProjectileType
 * @brief Distinguishes between player and enemy projectiles
 * 
 * Used for collision filtering and visual differentiation.
 * PLAYER_BULLET moves upward (negative Y), ENEMY_BULLET moves downward (positive Y).
 */
enum class ProjectileType {
    PLAYER_BULLET,
    ENEMY_BULLET
};

/**
 * @class ProjectileActor
 * @brief High-speed projectile with Continuous Collision Detection (CCD)
 * 
 * Architecture Pattern:
 * Demonstrates the RigidActor usage pattern for physics-simulated entities.
 * Unlike KinematicActor, RigidActor integrates position automatically through
 * the physics system, making it ideal for fast-moving objects where frame-rate
 * independent movement is critical.
 * 
 * Continuous Collision Detection (CCD):
 * Projectiles move at 120px/s, which means at 60 FPS they travel 2px per frame.
 * For accurate collision against small targets (aliens), we need swept collision
 * tests. This class maintains previousPosition to enable swept-circle tests
 * against AlienActors, which provides superior accuracy to discrete collision.
 * 
 * Pooling Pattern:
 * Projectiles use object pooling (managed by SpaceInvadersScene) rather than
 * dynamic allocation. The reset() method allows reuse without reconstruction,
 * critical for performance on microcontroller targets like ESP32.
 * 
 * Physics Configuration:
 * - Restitution: 0 (no bounce, projectiles should not ricochet)
 * - Friction: 0 (no air resistance, constant velocity)
 * - Gravity: 0 (no vertical acceleration, straight-line movement)
 * - Shape: Circle (accurate for bullet-like projectiles)
 * - CCD: Automatic via Flat Solver when velocity * dt > radius * 3
 */
class ProjectileActor : public pixelroot32::physics::RigidActor {
public:
    /**
     * @brief Constructs and immediately activates projectile
     * @param position Initial spawn position (typically player/alien position)
     * @param type Determines direction and color (player = white, enemy = red)
     */
    ProjectileActor(pixelroot32::math::Vector2 position, ProjectileType type);
    
    /**
     * @brief Main update - delegates movement to physics system
     * 
     * System Flow:
     * 1. Store previous position for swept collision tests
     * 2. RigidActor::update() handles physics integration
     * 3. CCD activates automatically if moving fast enough
     * 4. Deactivate if off-screen (object pool lifecycle)
     */
    void update(unsigned long deltaTime) override;
    
    /**
     * @brief Renders projectile as colored rectangle
     * White for player bullets, red for enemy bullets
     */
    void draw(pixelroot32::graphics::Renderer& renderer);
    
    /**
     * @brief Resets projectile for reuse from object pool
     * 
     * Design Pattern: Object Pooling
     * Instead of constructing/destructing projectiles (expensive), the Scene
     * maintains a fixed pool. reset() prepares a pooled projectile for reuse
     * with new position and type.
     */
    void reset(pixelroot32::math::Vector2 position, ProjectileType type);
    
    /**
     * @brief Collision callback - marks projectile for deactivation
     * 
     * Design Note: Projectiles always deactivate on collision regardless of
     * what they hit. Specific collision logic (damage, scoring) is handled
     * by SpaceInvadersScene to maintain separation of concerns.
     */
    void onCollision(pixelroot32::core::Actor* other) override;
    
    /**
     * @brief Returns true if projectile is active and should be processed
     */
    bool isActive() const { return active; }
    
    /**
     * @brief Marks projectile for deactivation and pool recycling
     */
    void deactivate() { active = false; }
    
    /**
     * @brief Returns projectile type for collision filtering
     */
    ProjectileType getType() const { return type; }
    
    /**
     * @brief Returns X coordinate from previous frame
     * 
     * Used by SpaceInvadersScene::handleCollisions() for swept-circle
     * collision tests against AlienActors. This provides sub-frame collision
     * accuracy essential for hitting fast-moving or small targets.
     */
    pixelroot32::math::Scalar getPreviousX() const { return previousPosition.x; }
    
    /**
     * @brief Returns Y coordinate from previous frame
     * 
     * Paired with getPreviousX() for complete swept test data.
     */
    pixelroot32::math::Scalar getPreviousY() const { return previousPosition.y; }

private:
    ProjectileType type;             // Player or enemy projectile
    bool active;                     // Pool lifecycle state
    pixelroot32::math::Vector2 previousPosition;  // For swept collision tests
};

}
