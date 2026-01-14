#pragma once
#ifdef PLATFORM_NATIVE
    #include "MockArduinoQueue.h"
#else
    #include <ArduinoQueue.h>
#endif

#include "physics/CollisionSystem.h"
#include "Entity.h"

#define MAX_ENTITIES 10  // Adjustable max entities per scene

/**
 * @class Scene
 * @brief Represents a game level or screen containing entities.
 *
 * A Scene manages a collection of Entities and a CollisionSystem. It is responsible
 * for updating and drawing all entities it contains.
 */
class Scene {
public:
    virtual ~Scene() {}

    /**
     * @brief Initializes the scene. Called when entering the scene.
     */
    virtual void init() {}

    /**
     * @brief Updates all entities in the scene and handles collisions.
     * @param deltaTime Time elapsed in ms.
     */
    virtual void update(unsigned long deltaTime);

    /**
     * @brief Draws all visible entities in the scene.
     * @param renderer The renderer to use.
     */
    virtual void draw(Renderer& renderer);

    /**
     * @brief Adds an entity to the scene.
     * @param entity Pointer to the Entity to add.
     */
    void addEntity(Entity* entity);

    /**
     * @brief Removes an entity from the scene.
     * @param entity Pointer to the Entity to remove.
     */
    void removeEntity(Entity* entity);

    /**
     * @brief Removes all entities from the scene.
     */
    void clearEntities();

protected:
    ArduinoQueue<Entity*> entities;  ///< Queue of entities in the scene.
    CollisionSystem collisionSystem; ///< System to handle collisions between actors.
};
