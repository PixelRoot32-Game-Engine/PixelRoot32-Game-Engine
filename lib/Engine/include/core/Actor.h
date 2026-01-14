#pragma once
#include "Entity.h"
#include <CollisionTypes.h>

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
     * @brief Constructor.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Width.
     * @param h Height.
     */
    Actor(float x, float y, int w, int h) : Entity(x, y, w, h, EntityType::ACTOR) {}
    virtual ~Actor() = default;

    CollisionLayer layer = DefaultLayers::kNone; ///< The collision layer this actor belongs to.
    CollisionLayer mask  = DefaultLayers::kNone; ///< The collision layers this actor interacts with.

    /**
     * @brief Sets the collision layer for this actor.
     * @param l The layer to set.
     */
    void setCollisionLayer(CollisionLayer l) { layer = l; }

    /**
     * @brief Sets the collision mask for this actor.
     * @param m The mask to set.
     */
    void setCollisionMask(CollisionLayer m)  { mask = m; }

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