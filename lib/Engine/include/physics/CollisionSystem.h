#pragma once
#include <vector>
#include <algorithm>
#include "core/Entity.h"

/**
 * @class CollisionSystem
 * @brief Manages collision detection between entities.
 *
 * This system iterates through registered entities, checks if they are Actors,
 * and performs AABB collision checks based on their collision layers and masks.
 */
class CollisionSystem {
public:
    /**
     * @brief Adds an entity to the collision system.
     * @param e Pointer to the entity to add.
     */
    void addEntity(Entity* e);

    /**
     * @brief Removes an entity from the collision system.
     * @param e Pointer to the entity to remove.
     */
    void removeEntity(Entity* e);

    /**
     * @brief Performs collision detection for all registered entities.
     * 
     * Iterates through all pairs of entities. If both are Actors and their
     * collision masks overlap, it checks for AABB intersection. If they intersect,
     * the onCollision() callback is triggered on both actors.
     */
    void update();

private:
    std::vector<Entity*> entities; ///< List of entities to check for collisions.
};
