/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "Entity.h"
#include "physics/CollisionTypes.h"
#include "math/Scalar.h"
#include <cstdint>

namespace pixelroot32::physics { class CollisionSystem; }

namespace pixelroot32::core {

/**
 * @class Actor
 * @brief An Entity capable of physical interaction and collision.
 *
 * Inherits from Entity.
 *
 * Adds collision layers and masks for dynamic game objects like players,
 * enemies, and projectiles.
 */
class Actor : public Entity {
public:
    /**
     * @brief Constructor using Scalar coordinates.
     */
    Actor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h) 
        : Entity(x, y, w, h, EntityType::ACTOR) {}

    /**
     * @brief Constructor using Vector2 position.
     */
    Actor(pixelroot32::math::Vector2 pos, int w, int h)
        : Entity(pos, w, h, EntityType::ACTOR) {}

    /**
     * @brief Constructor using float coordinates.
     * Only enabled if Scalar is NOT float to avoid ambiguity.
     */
    template <typename T = float, typename std::enable_if<!std::is_same<pixelroot32::math::Scalar, T>::value, int>::type = 0>
    Actor(float x, float y, int w, int h) 
        : Entity(x, y, w, h, EntityType::ACTOR) {}
        
    virtual ~Actor() = default;

    uint16_t entityId = 0;   ///< Unique id per CollisionSystem registration; used for pair deduplication.
    int queryId = 0;                         ///< Used for optimized grid queries.

    pixelroot32::physics::CollisionLayer layer = pixelroot32::physics::DefaultLayers::kNone; ///< The collision layer this actor belongs to.
    pixelroot32::physics::CollisionLayer mask  = pixelroot32::physics::DefaultLayers::kNone; ///< The collision layers this actor interacts with.

    pixelroot32::physics::CollisionSystem* collisionSystem = nullptr; ///< Reference to the collision system.

    /**
     * @brief Sets the collision layer this actor belongs to.
     * @param l The collision layer.
     */
    void setCollisionLayer(pixelroot32::physics::CollisionLayer l) { layer = l; }

    /**
     * @brief Sets the collision mask defining which layers this actor interacts with.
     * @param m The collision mask.
     */
    void setCollisionMask(pixelroot32::physics::CollisionLayer m)  { mask = m; }

    /**
     * @brief Updates the actor's state.
     * @param deltaTime Time elapsed since last update in milliseconds.
     */
    void update(unsigned long deltaTime) override {
        (void)deltaTime;
    }

    /**
     * @brief Checks if the actor belongs to a specific collision layer.
     * @param targetLayer The layer bitmask to check.
     * @return true if the actor is in the specified layer.
     */
    bool isInLayer(uint16_t targetLayer) const {
        return (layer & targetLayer) != 0;
    }

    /**
     * @brief Gets the axis-aligned bounding box (hitbox) for this actor.
     * @return The Rect representing the hitbox.
     */
    virtual Rect getHitBox() = 0;

    /**
     * @brief Checks if this actor is a physics-driven body (e.g., RigidActor).
     * @return true if it is a physics body, false otherwise.
     */
    virtual bool isPhysicsBody() const { return false; }

    /**
     * @brief Callback invoked when a collision occurs. Notification only — no physics changes.
     * @param other The actor that this actor collided with.
     */
    virtual void onCollision(Actor* other) = 0;
};

}
