/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#pragma once
#include <cstddef>
#include "physics/CollisionSystem.h"
#include "Entity.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::core {

    using namespace pixelroot32::core;
    using namespace pixelroot32::physics;
    using namespace pixelroot32::graphics;
    

#include <new> // for placement new
struct SceneArena {
    unsigned char* buffer;
    std::size_t capacity;
    std::size_t offset;

    SceneArena() : buffer(nullptr), capacity(0), offset(0) {}

    void init(void* memory, std::size_t size);
    void reset();
    void* allocate(std::size_t size, std::size_t alignment);
};

template<typename T, typename... Args>
T* arenaNew(SceneArena& arena, Args&&... args) {
    void* mem = arena.allocate(sizeof(T), alignof(T));
    if (!mem) {
        return nullptr;
    }
    return new (mem) T(static_cast<Args&&>(args)...);
}

/**
 * @class Scene
 * @brief Represents a game level or screen containing entities.
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
    Entity* entities[pixelroot32::platforms::config::MaxEntities]; ///< Array of entities in the scene.
    int entityCount = 0;            ///< Current number of entities.
    bool needsSorting = false;      ///< Flag to trigger sorting by layer.

    void sortEntities();            ///< Sorts entities by render layer.
    bool isVisibleInViewport(Entity* entity, pixelroot32::graphics::Renderer& renderer);

    pixelroot32::physics::CollisionSystem collisionSystem; ///< System to handle collisions between actors.
    SceneArena arena;
};

}
