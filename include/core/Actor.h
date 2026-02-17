/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "Entity.h"
#include "physics/CollisionTypes.h"
#include "math/Scalar.h"

namespace pixelroot32::core {

/**
 * @class Actor
 * @brief An Entity capable of physical interaction and collision.
 *
 * Actors extend Entity with collision layers and masks. They are used for
 * dynamic game objects like players, enemies, and projectiles.
 */
class Actor : public Entity {
public:
    /**
     * @brief Constructor using Scalar coordinates.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Width.
     * @param h Height.
     */
    Actor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h) 
        : Entity(x, y, w, h, EntityType::ACTOR) {}

    /**
     * @brief Constructor using float coordinates.
     * Only enabled if Scalar is NOT float to avoid ambiguity.
     */
    template <typename T = float, typename std::enable_if<!std::is_same<pixelroot32::math::Scalar, T>::value, int>::type = 0>
    Actor(float x, float y, int w, int h) 
        : Entity(x, y, w, h, EntityType::ACTOR) {}
        
    virtual ~Actor() = default;

    pixelroot32::physics::CollisionLayer layer = pixelroot32::physics::DefaultLayers::kNone; ///< The collision layer this actor belongs to.
    pixelroot32::physics::CollisionLayer mask  = pixelroot32::physics::DefaultLayers::kNone; ///< The collision layers this actor interacts with.

    /**
     * @brief Sets the collision layer for this actor.
     * @param l The layer to set.
     */
    void setCollisionLayer(pixelroot32::physics::CollisionLayer l) { layer = l; }

    /**
     * @brief Sets the collision mask for this actor.
     * @param m The mask to set.
     */
    void setCollisionMask(pixelroot32::physics::CollisionLayer m)  { mask = m; }

    /**
     * @brief Updates the actor logic.
     * @param deltaTime Time elapsed in ms.
     */
    void update(unsigned long deltaTime) override {
        (void)deltaTime;
    }

    /**
     * @brief Checks if the Actor belongs to a specific collision layer.
     * @param targetLayer The bit(s) to check (e.g., Layers::BALL).
     * @return true if the bit is set in the actor's layer.
     */
    bool isInLayer(uint16_t targetLayer) const {
        // Bitwise AND operation: If result is not 0, the bit exists.
        return (layer & targetLayer) != 0;
    }

    /**
     * @brief Gets the hitbox for collision detection.
     * @return A Rect representing the collision bounds.
     */
    virtual Rect getHitBox() = 0;

    /**
     * @brief Callback invoked when a collision occurs.
     * @param other The actor that this actor collided with.
     */
    virtual void onCollision(Actor* other) = 0;
};

}
