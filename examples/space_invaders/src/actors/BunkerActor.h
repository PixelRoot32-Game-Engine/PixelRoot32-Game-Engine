#pragma once
#include <physics/StaticActor.h>
#include <graphics/Renderer.h>

namespace spaceinvaders {

/**
 * @class BunkerActor
 * @brief Defensive barrier with progressive damage visualization
 * 
 * Architecture Pattern:
 * Demonstrates StaticActor usage - immovable objects that participate in
 * collision detection but have zero physics response. StaticActors are
 * optimized for stationary geometry, consuming minimal CPU resources.
 * 
 * Damage System:
 * Bunkers degrade visually as they absorb projectile hits, providing
 * both gameplay feedback and strategic depth (damaged sections offer
 * less protection).
 * 
 * Visual Degradation:
 * - Health > 50%: Green (intact, full protection)
 * - 25% < Health <= 50%: Yellow (damaged, partial protection)
 * - Health <= 25%: Red (critical, minimal protection)
 * - Health = 0: Invisible (destroyed, no collision)
 * 
 * Collision Strategy:
 * Uses AABB collision shape. Hitbox height decreases proportionally with
 * health, meaning damaged bunkers have reduced protective area. This
 * creates emergent gameplay where players must adapt as cover degrades.
 * 
 * Optimization Note:
 * As a StaticActor, position is never integrated by physics system,
 * saving computation compared to Rigid/Kinematic actors.
 */
class BunkerActor : public pixelroot32::physics::StaticActor {
public:
    /**
     * @brief Constructs bunker with specified health and dimensions
     * @param position World position (top-left corner)
     * @param w Width in pixels
     * @param h Height in pixels  
     * @param maxHealth Starting health value
     */
    BunkerActor(pixelroot32::math::Vector2 position, int w, int h, int maxHealth);

    /**
     * @brief Static actors require no per-frame updates
     * 
     * Design Note: Override exists because base Actor class declares
     * update() as pure virtual. Static geometry doesn't change state
     * without external intervention (damage).
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Renders bunker with height indicating remaining health
     * 
     * Visual Technique: The bunker "shrinks" from bottom to top as
     * health decreases, creating intuitive damage visualization.
     * Remaining height is drawn at calculated Y offset to maintain
     * proper world alignment.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Collision callback - detection only
     * 
     * Design Pattern: Collision detected here, damage applied elsewhere.
     * ProjectileActor::onCollision() calls applyDamage(), preventing
     * double-damage from single projectile.
     */
    void onCollision(pixelroot32::core::Actor* other) override;

    /**
     * @brief Applies damage from projectile impact
     * @param amount Health points to subtract (typically 1 per hit)
     */
    void applyDamage(int amount);

    /**
     * @brief Returns true if bunker health depleted
     */
    bool isDestroyed() const;

private:
    int health;     // Current health (0 = destroyed)
    int maxHealth;  // Starting health for ratio calculations
};

}
