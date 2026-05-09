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
#include "physics/PhysicsScheduler.h"
#include "Entity.h"
#include "platforms/EngineConfig.h"
#include "input/TouchEvent.h"

#ifdef PIXELROOT32_ENABLE_UI_SYSTEM
#include "graphics/ui/UIManager.h"
#endif

namespace pixelroot32::core {

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
    virtual void init() {
        #if PIXELROOT32_ENABLE_PHYSICS
            physicsScheduler.init();
        #endif
    }

#if PIXELROOT32_ENABLE_UI_SYSTEM
    /**
     * @brief Initialize the UI system for this scene.
     * Called during scene init. Add UI elements here.
     */
    virtual void initUI() {}

    /**
     * @brief Update the UI system.
     * @param deltaTime Time elapsed in ms.
     */
    virtual void updateUI(unsigned long deltaTime) {
        (void)deltaTime;
    }

    /**
     * @brief Get the UI manager for this scene.
     * @return Reference to the UIManager
     */
    pixelroot32::graphics::ui::UIManager& getUIManager() { return uiManager; }
#endif

    /**
     * @brief Central touch pipeline entry point (call once per frame).
     *
     * Execution order (deterministic):
     *   1. If UI is enabled: UIManager::processEvents — marks handled events consumed.
     *   2. For each unconsumed event: calls onUnconsumedTouchEvent (virtual).
     *
     * Callers (Engine / game loop) feed the buffer obtained from
     * TouchManager::getEvents or TouchEventDispatcher::getEvents.
     *
     * @param events Mutable buffer — consumed flags are set in-place.
     * @param count  Number of events in the buffer.
     */
    virtual void processTouchEvents(pixelroot32::input::TouchEvent* events, uint8_t count);

    /**
     * @brief Hook for scene-specific handling of unconsumed touch events.
     *
     * Override in subclasses to feed ActorTouchController, custom drag
     * logic, etc.  Default implementation is a no-op.
     *
     * @param event The touch event (not consumed by UI).
     */
    virtual void onUnconsumedTouchEvent(const pixelroot32::input::TouchEvent& event) {
        (void)event;
    }

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
     * @brief Advises the scene that the framebuffer is about to be drawn.
     * @param renderer The renderer to use.
     * 
     * Optional hook: run immediately before Renderer::beginFrame().
     * Scenes using StaticTilemapLayerCache should call adviseFramebufferBeforeBeginFrame with the same
     * layers/camera sampling as StaticTilemapLayerCache::draw so dirty-region clears align with framebuffer memcpy restores.
     */
    virtual void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer) {
        (void)renderer;
    }

    /**
     * @brief When false, Engine may skip `draw()` and `present()` for this iteration (after `update()`).
     *
     * Default `true` preserves legacy behavior. Override to return false only when the logical
     * framebuffer would be identical to the last presented frame (same camera, same visuals).
     * When `PIXELROOT32_ENABLE_DEBUG_OVERLAY` is on, Engine forces a full redraw regardless.
     */
    virtual bool shouldRedrawFramebuffer() const { return true; }

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

    // Physics 
    #if PIXELROOT32_ENABLE_PHYSICS
        pixelroot32::physics::CollisionSystem collisionSystem; ///< System to handle collisions between actors.
        pixelroot32::physics::PhysicsScheduler physicsScheduler; ///< Fixed timestep scheduler for physics.
    #endif

    // UI System
    #if PIXELROOT32_ENABLE_UI_SYSTEM
        pixelroot32::graphics::ui::UIManager uiManager; ///< Touch UI manager for the scene.
    #endif

    SceneArena arena;
};

}
