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
#include "graphics/CameraEffects.h"

#ifdef PIXELROOT32_ENABLE_UI_SYSTEM
#include "graphics/ui/UIManager.h"
#endif

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM
#include "gameplay/RoomGraph.h"
#endif

namespace pixelroot32::core {

#include <new> // for placement new

/**
 * @struct SceneArena
 * @brief Bump allocator for objects whose lifetime is one scene.
 *
 * Hands out aligned slices of a caller-supplied buffer by advancing an offset.
 * There is no individual free: reset() rewinds the offset to zero and releases
 * everything at once, which is why allocations must not outlive the scene.
 * Destructors are never run, so only trivially destructible types are safe.
 *
 * allocate() returns nullptr when the request does not fit, and callers must
 * check. Prefer arenaNew() over calling allocate() directly.
 *
 * @note Gated by platforms::config::EnableSceneArena. With the arena
 *       disabled every allocate() returns nullptr and init()/reset() are
 *       no-ops, so callers keep their fallback path.
 */
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
 *
 * @note init() is idempotent — may be called any number of times, each call
 *       leaves the state equivalent to a fresh init.
 */
class Scene {
public:
    virtual ~Scene() {}

    /**
     * @brief Initialises the scene. Called when entering the scene.
     *
     * Idempotent contract: init() MUST leave the scene in a state equivalent
     * to a single fresh init.  N invocations MUST NOT produce dangling
     * pointers, duplicate entities, or leaked resources.
     *
     * Calls resetState() first to clear any prior state, then re-initialises
     * the physics scheduler (if enabled).
     */
    virtual void init() {
        resetState();
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

#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM
    /**
     * @brief Hook that fires when the scene's RoomGraph enters a new room.
     *
     * Override in subclasses to react to room transitions (spawn entities,
     * play SFX, adjust custom camera bounds). The default implementation
     * is a no-op that mirrors onUnconsumedTouchEvent.
     *
     * @param fromIdx Index of the previous room (0xFFFF if first entry).
     * @param toIdx   Index of the room being entered.
     */
    virtual void onRoomEnter(int fromIdx, int toIdx) {
        (void)fromIdx;
        (void)toIdx;
    }

    /**
     * @brief Type-erased accessor for the scene's room graph.
     * @return Pointer to the RoomGraphBase, or nullptr if none set.
     */
    pixelroot32::gameplay::RoomGraphBase* getRoomGraph() const {
        return roomGraph_;
    }

    /**
     * @brief Register a RoomGraph with this scene, called once during init().
     *
     * Subclasses that own a RoomGraph<N> member call this to wire it into
     * the base class. The base invokes onRoomEnter via the graph's onEnter
     * callback whenever enterRoom transitions to a new room.
     *
     * @param g Pointer to a RoomGraphBase (typically a RoomGraph<N>
     *          owned by the subclass). Must outlive this scene.
     */
    void setRoomGraph(pixelroot32::gameplay::RoomGraphBase* g) {
        roomGraph_ = g;
    }
#endif // PIXELROOT32_ENABLE_GAMEPLAY_ROOM

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
     * @brief Get the current summed offset from CameraEffectsSystem.
     * @return math::Vector2 offset (ZERO when no effects active or feature disabled).
     */
    inline pixelroot32::math::Vector2 getCameraEffectOffset() const {
#if PIXELROOT32_ENABLE_CAMERA_EFFECTS
        return cameraEffects.getOffset();
#else
        return pixelroot32::math::Vector2::ZERO();
#endif
    }

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
    /**
     * @brief Resets the scene to a clean initial state.
     *
     * Called at the start of every init() invocation.  Default implementation
     * clears all entities, resets the arena, and (if physics is enabled)
     * clears the collision system.
     *
     * Derived scenes that own heap-allocated resources (e.g. unique_ptr
     * members) MUST override resetState() to release owned resources BEFORE
     * calling Scene::resetState().  This ensures no dangling raw pointers
     * remain in the base entity array.
     *
     * Overrides MUST call Scene::resetState() as their last operation
     * (after releasing owned resources).
     */
    virtual void resetState() noexcept;

    Entity* entities[pixelroot32::platforms::config::MaxEntities]; ///< Array of entities in the scene.
    int entityCount = 0;            ///< Current number of entities.
    bool needsSorting = false;      ///< Flag to trigger sorting by layer.

    /**
     * @brief Function-pointer comparator for secondary ordering within a render layer.
     *
     * `true` when `a` must be drawn before `b`. Consulted by shouldPrecede()
     * only when both entities share the same renderLayer; `nullptr` (the
     * default) means "no secondary ordering", which keeps sortEntities()
     * byte-identical to its pre-capability behavior (see design.md D5).
     */
    using DepthComparator = bool (*)(Entity* a, Entity* b);

    DepthComparator depthComparator = nullptr;  ///< Optional secondary comparator (nullptr = today's behavior).
    bool depthSortEnabled = false;              ///< When true, draw() re-sorts every frame regardless of needsSorting.

    void sortEntities();            ///< Sorts entities by render layer.
    bool isVisibleInViewport(Entity* entity, pixelroot32::graphics::Renderer& renderer);

    /**
     * @brief Insertion-sort predicate used by sortEntities(): whether `key`
     * must be placed before `at`.
     *
     * Primary comparison is renderLayer, identical to today's behavior. Only
     * when both entities share the same renderLayer does the comparator get
     * consulted; with no comparator set this falls through to `false`,
     * matching today's stable-insertion behavior for equal layers exactly.
     */
    inline bool shouldPrecede(Entity* key, Entity* at) const {
        const unsigned char la = at->getRenderLayer();
        const unsigned char lk = key->getRenderLayer();
        if (la != lk) return la > lk;               // identical to today
        if (depthComparator == nullptr) return false; // identical to today
        return depthComparator(key, at);
    }

    // Physics 
    #if PIXELROOT32_ENABLE_PHYSICS
        pixelroot32::physics::CollisionSystem collisionSystem; ///< System to handle collisions between actors.
        pixelroot32::physics::PhysicsScheduler physicsScheduler; ///< Fixed timestep scheduler for physics.
    #endif

    // UI System
    #if PIXELROOT32_ENABLE_UI_SYSTEM
        pixelroot32::graphics::ui::UIManager uiManager; ///< Touch UI manager for the scene.
    #endif

    // Camera Effects
    #if PIXELROOT32_ENABLE_CAMERA_EFFECTS
        pixelroot32::graphics::CameraEffectsSystem cameraEffects; ///< Camera shake/punch/offset effects.
    #endif

    // Room Graph
    #if PIXELROOT32_ENABLE_GAMEPLAY_ROOM
        pixelroot32::gameplay::RoomGraphBase* roomGraph_ = nullptr;
    #endif

    SceneArena arena;
};

}
