/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/Actor.h"
#include "math/Scalar.h"
#include "math/Vector2.h"
#include <type_traits>

namespace pixelroot32::core {

/**
 * @struct LimitRect
 * @brief Defines a rectangular boundary for actor movement.
 * 
 * Used to constrain an actor within a specific area of the world.
 * Values of -1 indicate no limit on that side.
 */
struct LimitRect {
    int left = -1;
    int top = -1;
    int right = -1;
    int bottom = -1;

    LimitRect() = default;

    /**
     * @brief Constructs a new LimitRect.
     * @param l Left limit.
     * @param t Top limit.
     * @param r Right limit.
     * @param b Bottom limit.
     */
    LimitRect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
    
    int width() const { return right - left; }
    int height() const { return bottom - top; }
};

/**
 * @struct WorldCollisionInfo
 * @brief Stores flags indicating which world boundaries were hit in the current frame.
 */
struct WorldCollisionInfo {
    bool left = false;
    bool right = false;
    bool top = false;
    bool bottom = false;

    WorldCollisionInfo() = default;
    WorldCollisionInfo(bool l, bool r, bool t, bool b) : left(l), right(r), top(t), bottom(b) {}
};

/**
 * @class PhysicsActor
 * @brief An actor with basic 2D physics properties using adaptable Scalar type.
 * 
 * Handles velocity, acceleration (via integration), and collision with world boundaries.
 * Automatically adapts to use float or Fixed16 based on the platform configuration.
 */
class PhysicsActor : public Actor {
protected:
    pixelroot32::math::Vector2 velocity;

    LimitRect limits;
    int worldWidth = 0;
    int worldHeight = 0;

    WorldCollisionInfo worldCollisionInfo;

    pixelroot32::math::Scalar restitution = pixelroot32::math::toScalar(1.0f);
    pixelroot32::math::Scalar friction    = pixelroot32::math::toScalar(0.0f);

public:
    /**
     * @brief Constructs a new PhysicsActor.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Width of the actor.
     * @param h Height of the actor.
     */
    PhysicsActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

    /**
     * @brief Updates the actor state.
     * 
     * Applies physics integration using velocity and checks for world boundary collisions.
     * @param deltaTime Time elapsed since the last frame in milliseconds.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Sets custom movement limits for the actor.
     * @param limitRect The LimitRect structure defining the boundaries.
     */
    void setLimits(const LimitRect& limitRect) { limits = limitRect; }

    /**
     * @brief Sets custom movement limits for the actor.
     * @param left Left limit.
     * @param top Top limit.
     * @param right Right limit.
     * @param bottom Bottom limit.
     */
    void setLimits(int left, int top, int right, int bottom);

    /**
     * @brief Defines the world size for boundary checking.
     * 
     * Used as default limits if no custom LimitRect is provided.
     * @param w Width of the world.
     * @param h Height of the world.
     */
    void setWorldBounds(int w, int h);
    
    /**
     * @brief Legacy alias for setWorldBounds.
     * @param w Width of the world.
     * @param h Height of the world.
     */
    void setWorldSize(int w, int h) { setWorldBounds(w, h); }

    /**
     * @brief Gets information about collisions with the world boundaries.
     * @return A WorldCollisionInfo struct containing collision flags.
     */
    WorldCollisionInfo getWorldCollisionInfo() const;

    /**
     * @brief Resets the world collision flags for the current frame.
     */
    void resetWorldCollisionInfo();

    /**
     * @brief Integrates velocity to update position.
     * @param dt Delta time in seconds (as Scalar).
     */
    void integrate(pixelroot32::math::Scalar dt);
    
    /**
     * @brief Resolves collisions with the defined world or custom bounds.
     * 
     * Constrains the actor's position to stay within limits and reverses velocity
     * based on restitution if a collision occurs.
     */
    virtual void resolveWorldBounds();

    /**
     * @brief Sets the linear velocity of the actor using floats.
     * @param x Horizontal velocity.
     * @param y Vertical velocity.
     */
    template <typename T = float, typename std::enable_if<!std::is_same<T, pixelroot32::math::Scalar>::value, int>::type = 0>
    void setVelocity(T x, T y) {
        velocity.x = pixelroot32::math::toScalar(x);
        velocity.y = pixelroot32::math::toScalar(y);
    }

    /**
     * @brief Sets the linear velocity of the actor using Scalars.
     * @param x Horizontal velocity.
     * @param y Vertical velocity.
     */
    void setVelocity(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y) {
        velocity.x = x;
        velocity.y = y;
    }

    /**
     * @brief Sets the linear velocity of the actor using a Vector2.
     * @param v Velocity vector.
     */
    void setVelocity(const pixelroot32::math::Vector2& v) {
        velocity = v;
    }

    /**
     * @brief Gets the horizontal velocity.
     * @return Horizontal velocity as Scalar.
     */
    pixelroot32::math::Scalar getVelocityX() const { return velocity.x; }

    /**
     * @brief Gets the vertical velocity.
     * @return Vertical velocity as Scalar.
     */
    pixelroot32::math::Scalar getVelocityY() const { return velocity.y; }

    /**
     * @brief Gets the velocity vector.
     * @return Reference to the velocity Vector2.
     */
    const pixelroot32::math::Vector2& getVelocity() const { return velocity; }

    /**
     * @brief Sets the restitution (bounciness) of the actor.
     * @param r Restitution value (0.0 to 1.0+). 1.0 means no energy is lost on bounce.
     */
    void setRestitution(float r) { restitution = pixelroot32::math::toScalar(r); }

    /**
     * @brief Sets the friction coefficient.
     * @param f Friction value (0.0 means no friction).
     */
    void setFriction(float f) { friction = pixelroot32::math::toScalar(f); }

    /**
     * @brief Callback triggered when this actor collides with another actor.
     * @param other Pointer to the actor involved in the collision.
     */
    void onCollision(Actor* other) override;

    /**
     * @brief Callback triggered when this actor collides with world boundaries.
     * 
     * Override this method to implement custom behavior when hitting walls (e.g., sound effects).
     */
    virtual void onWorldCollision();
};

} // namespace pixelroot32::core
