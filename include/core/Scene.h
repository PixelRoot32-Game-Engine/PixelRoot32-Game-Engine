/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 Gabriel Perez
 *
 * This file remains licensed under the MIT License.
 */
#pragma once
#ifdef PLATFORM_NATIVE
    #include "../../src/platforms/mock/MockArduinoQueue.h"
#else
    #include <ArduinoQueue.h>
#endif

#include <cstddef>
#include "physics/CollisionSystem.h"
#include "Entity.h"

#define MAX_ENTITIES 32

namespace pixelroot32::core {

    using namespace pixelroot32::core;
    using namespace pixelroot32::physics;
    using namespace pixelroot32::graphics;
    
    
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
struct SceneArena {
    unsigned char* buffer;
    std::size_t capacity;
    std::size_t offset;

    SceneArena() : buffer(nullptr), capacity(0), offset(0) {}

    void init(void* memory, std::size_t size);
    void reset();
    void* allocate(std::size_t size, std::size_t alignment);
};
#endif

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
    virtual void draw(pixelroot32::graphics::Renderer& renderer);

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
    pixelroot32::physics::CollisionSystem collisionSystem; ///< System to handle collisions between actors.
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    SceneArena arena;
#endif
};

}
