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
 * @enum PhysicsBodyType
 * @brief Defines the simulation behavior of a PhysicsActor.
 */
enum class PhysicsBodyType {
    STATIC,    ///< Immovable body, not affected by forces or gravity.
    KINEMATIC, ///< Body moved manually or via script, stops at obstacles.
    RIGID      ///< Fully simulated physics body, affected by forces and gravity.
};

/**
 * @enum CollisionShape
 * @brief Defines the geometric shape used for collision detection.
 */
enum class CollisionShape {
    AABB,   ///< Axis-Aligned Bounding Box (Default)
    CIRCLE  ///< Circular collider
};

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

    PhysicsBodyType bodyType = PhysicsBodyType::KINEMATIC;

    LimitRect limits;
    int worldWidth = 0;
    int worldHeight = 0;

    WorldCollisionInfo worldCollisionInfo;

    pixelroot32::math::Scalar mass          = pixelroot32::math::toScalar(1.0f);
    pixelroot32::math::Scalar gravityScale  = pixelroot32::math::toScalar(1.0f);
    pixelroot32::math::Scalar restitution = pixelroot32::math::toScalar(1.0f);
    pixelroot32::math::Scalar friction    = pixelroot32::math::toScalar(0.0f);

    CollisionShape shape = CollisionShape::AABB;
    pixelroot32::math::Scalar radius = pixelroot32::math::toScalar(0.0f);


public:
    bool bounce = false; ///< When true, velocity is reflected on static contact. When false, velocity is zeroed.
    /**
     * @brief Constructs a new PhysicsActor.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Width of the actor.
     * @param h Height of the actor.
     */
    PhysicsActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h);

    /**
     * @brief Constructs a new PhysicsActor using Vector2 position.
     * @param position Initial position.
     * @param w Width of the actor.
     * @param h Height of the actor.
     */
    PhysicsActor(pixelroot32::math::Vector2 position, int w, int h);

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
     * @brief Checks if this actor is a physics-enabled body.
     * @return true.
     */
    bool isPhysicsBody() const override { return true; }

    /**
     * @brief Resets the world collision flags for the current frame.
     */
    void resetWorldCollisionInfo();

    /**
     * @brief Gets the simulation body type.
     * @return The PhysicsBodyType of this actor.
     */
    PhysicsBodyType getBodyType() const { return bodyType; }

    /**
     * @brief Sets the simulation body type.
     * @param type The new PhysicsBodyType.
     */
    void setBodyType(PhysicsBodyType type) { bodyType = type; }

    /**
     * @brief Sets the mass of the actor.
     * @param m Mass value.
     */
    void setMass(float m) { mass = pixelroot32::math::toScalar(m); }

    /**
     * @brief Gets the mass of the actor.
     * @return Mass as Scalar.
     */
    pixelroot32::math::Scalar getMass() const { return mass; }

    /**
     * @brief Sets the gravity scale.
     * @param scale Multiplier for the world gravity.
     */
    void setGravityScale(pixelroot32::math::Scalar scale) { gravityScale = scale; }

    /**
     * @brief Gets the gravity scale.
     * @return Gravity scale as Scalar.
     */
    pixelroot32::math::Scalar getGravityScale() const { return gravityScale; }

    /**
     * @brief Integrates velocity to update position.
     * @param dt Delta time in seconds (as Scalar).
     */
    virtual void integrate(pixelroot32::math::Scalar dt);
    
    /**
     * @brief Resolves collisions with the defined world or custom bounds.
     * 
     * Constrains the actor's position to stay within limits and reverses velocity
     * based on restitution if a collision occurs.
     */
    virtual void resolveWorldBounds();

    /**
     * @brief Gets the axis-aligned bounding box (AABB) hitbox of the actor.
     * @return Rect representing the hitbox.
     */
    pixelroot32::core::Rect getHitBox() override;

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
    void setRestitution(pixelroot32::math::Scalar r) { restitution = r; }

    /**
     * @brief Gets the restitution (bounciness) of the actor.
     * @return Restitution as Scalar.
     */
    pixelroot32::math::Scalar getRestitution() const { return restitution; }

    /**
     * @brief Sets the friction coefficient.
     * @param f Friction value (0.0 means no friction).
     */
    void setFriction(pixelroot32::math::Scalar f) { friction = f; }

    /**
     * @brief Gets the collision shape type.
     * @return The CollisionShape of this actor.
     */
    CollisionShape getShape() const { return shape; }

    /**
     * @brief Sets the collision shape type.
     * @param s The new CollisionShape.
     */
    void setShape(CollisionShape s) { shape = s; }

    /**
     * @brief Gets the radius (only for Shape::CIRCLE).
     * @return Radius as Scalar.
     */
    pixelroot32::math::Scalar getRadius() const { return radius; }

    /**
     * @brief Sets the radius and updates width/height to match diameter.
     * @param r Radius value.
     */
    void setRadius(pixelroot32::math::Scalar r) { 
        pixelroot32::math::Scalar dm = pixelroot32::math::Scalar(2);

        radius = r; 
        width = static_cast<int>(r * dm);
        height = static_cast<int>(r * dm);
    }

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
