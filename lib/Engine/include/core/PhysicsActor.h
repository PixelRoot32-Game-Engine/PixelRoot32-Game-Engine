#pragma once
#include "core/Actor.h"

namespace pixelroot32::core {

/**
 * @struct LimitRect
 * @brief Bounding rectangle for world-collision resolution.
 *
 * This struct defines the limits of the play area, constraining actors
 * to stay within these bounds.
 */
struct LimitRect {
    int left = -1;      ///< Left boundary (-1 means no limit).
    int top = -1;       ///< Top boundary (-1 means no limit).
    int right = -1;     ///< Right boundary (-1 means no limit).
    int bottom = -1;    ///< Bottom boundary (-1 means no limit).

    LimitRect() = default;
    
    /**
     * @brief Constructs a LimitRect with specific bounds.
     * @param l Left boundary.
     * @param t Top boundary.
     * @param r Right boundary.
     * @param b Bottom boundary.
     */
    LimitRect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}

    /**
     * @brief Calculates the width of the limit area.
     * @return Width of the rectangle (right - left).
     */
    int width() const { return right - left; }

    /**
     * @brief Calculates the height of the limit area.
     * @return Height of the rectangle (bottom - top).
     */
    int height() const { return bottom - top; }
};

/**
 * @struct WorldCollisionInfo
 * @brief Information about world collisions in the current frame.
 *
 * This struct holds flags indicating which sides of the play area
 * the actor has collided with during the last update.
 */
struct WorldCollisionInfo {
    bool left = false;      ///< True if collided with the left boundary.
    bool right = false;     ///< True if collided with the right boundary.
    bool top = false;       ///< True if collided with the top boundary.
    bool bottom = false;    ///< True if collided with the bottom boundary.

    WorldCollisionInfo() = default;

    /**
     * @brief Constructs a WorldCollisionInfo with specific flags.
     * @param l Left collision flag.
     * @param r Right collision flag.
     * @param t Top collision flag.
     * @param b Bottom collision flag.
     */
    WorldCollisionInfo(bool l, bool r, bool t, bool b) : left(l), right(r), top(t), bottom(b) {}
};

/**
 * @class PhysicsActor
 * @brief An actor with basic 2D physics properties.
 *
 * PhysicsActor extends the base Actor class by adding velocity, acceleration,
 * friction, restitution (bounciness), and world boundary collision resolution.
 * It is designed for objects that need to move and bounce within a defined area.
 */
class PhysicsActor : public Actor {
protected:
    float vx = 0.0f;        ///< Horizontal velocity component.
    float vy = 0.0f;        ///< Vertical velocity component.

    LimitRect limits;       ///< Custom boundaries for the actor.
    int worldWidth = 0;     ///< Width of the game world (fallback limit).
    int worldHeight = 0;    ///< Height of the game world (fallback limit).

    WorldCollisionInfo worldCollisionInfo; ///< Stores collision state with world bounds.

    float restitution = 1.0f; ///< Bounciness factor (1.0 = perfect conservation, < 1.0 = energy loss).
    float friction    = 0.0f; ///< Friction factor applied to velocity (0.0 = no friction).

public:
    /**
     * @brief Constructs a PhysicsActor.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Actor width.
     * @param h Actor height.
     */
    PhysicsActor(float x, float y, float w, float h);

    /**
     * @brief Updates the actor state.
     * 
     * Applies physics integration using velocity and checks for world boundary collisions.
     * @param deltaTime Time elapsed since the last frame in milliseconds.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Gets information about collisions with the world boundaries.
     * @return A WorldCollisionInfo struct containing collision flags.
     */
    WorldCollisionInfo getWorldCollisionInfo() const { return worldCollisionInfo; }

    /**
     * @brief Callback triggered when this actor collides with another actor.
     * @param other Pointer to the actor involved in the collision.
     */
    virtual void onCollision(Actor* other);

    /**
     * @brief Callback triggered when this actor collides with world boundaries.
     * 
     * Override this method to implement custom behavior when hitting walls (e.g., sound effects).
     */
    virtual void onWorldCollision();
  
    /**
     * @brief Sets the linear velocity of the actor.
     * @param x Horizontal velocity.
     * @param y Vertical velocity.
     */
    void setVelocity(float x, float y);

    /**
     * @brief Sets the restitution (bounciness) of the actor.
     * @param r Restitution value (0.0 to 1.0+). 1.0 means no energy is lost on bounce.
     */
    void setRestitution(float r);

    /**
     * @brief Sets the friction coefficient.
     * @param f Friction value (0.0 means no friction).
     */
    void setFriction(float f);

    /**
     * @brief Sets custom movement limits for the actor.
     * @param limits A LimitRect defining the allowed area.
     */
    void setLimits(LimitRect limits);

    /**
     * @brief Defines the world size for boundary checking.
     * 
     * Used as default limits if no custom LimitRect is provided.
     * @param width Width of the world.
     * @param height Height of the world.
     */
    void setWorldSize(int width, int height);

protected:
    /**
     * @brief Integrates velocity to update position.
     * @param dt Delta time in seconds.
     */
    void integrate(float dt);

    /**
     * @brief Resolves collisions with the defined world or custom bounds.
     * 
     * Constrains the actor's position to stay within limits and reverses velocity
     * based on restitution if a collision occurs.
     */
    void resolveWorldBounds();

private:
   /**
    * @brief Resets the world collision flags for the current frame.
    */
   inline void resetWorldCollisionInfo()  {
        worldCollisionInfo = WorldCollisionInfo();
   }
};

}
