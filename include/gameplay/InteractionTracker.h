/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS

#include "gameplay/InteractionComponent.h"
#include "platforms/EngineConfig.h"
#include "core/Actor.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
#include "gameplay/GameplayEventBus.h"
#endif

#include <cstdint>

namespace pixelroot32::gameplay {

/**
 * @class InteractionTracker
 * @brief Detects enter/exit edges on the per-frame physics contact set and
 *        dispatches InteractionComponent callbacks (design.md D3).
 *
 * Driven from `CollisionSystem::triggerCallbacks()`:
 *   - `beginFrame()` before the contact loop
 *   - `recordPair()` inside the loop, after the unchanged `onCollision()` calls
 *   - `endFrame()` after the loop — diffs `currPairs` against `prevPairs` and
 *     fires `onEnter`/`onExit` for every pair whose presence changed.
 *
 * `currPairs`/`prevPairs` are sized off `config::PhysicsMaxContacts` (the
 * same capacity `CollisionSystem` already uses for its contact array), so
 * this invents no new capacity concept. The registry is a small, separate,
 * fixed-size array of at most `GAMEPLAY_MAX_INTERACTIVE_ACTORS` entries.
 *
 * Zero heap allocation: every member is a fixed-size array. `registerActor()`
 * asserts `actor->entityId != 0` — an actor must be added to a
 * `CollisionSystem` (which assigns `entityId`) BEFORE it is registered here;
 * registering earlier is an ordering mistake that the assert catches
 * immediately rather than silently producing an actor that never receives
 * edges (design.md Risks section).
 *
 * **Contact edges are tracked for every physics contact**, not only
 * registered ones — `recordPair()` is called from `triggerCallbacks()` for
 * every `Contact`, so a bus-published `TriggerEnter`/`TriggerExit` fires for
 * any pair whose presence changed, independent of registration. Component
 * callback dispatch, in contrast, only fires for a pair's registered side(s):
 * the `other` actor pointer passed to a callback is resolved through the
 * registry, so it is only non-null when the counterpart actor is ALSO
 * registered. A game that needs the counterpart's identity in the callback
 * (not just via the bus event's raw entity ids) must register both sides.
 */
class InteractionTracker {
public:
    static constexpr int kMaxContacts =
        pixelroot32::platforms::config::PhysicsMaxContacts;
    static constexpr int kMaxRegistered =
        pixelroot32::platforms::config::GameplayMaxInteractiveActors;

    /**
     * @brief Registers an actor's InteractionComponent for enter/exit dispatch.
     * @param actor The actor to track. MUST already be registered with a
     *        CollisionSystem (i.e. `actor->entityId != 0`).
     * @param comp Non-owning pointer to the actor's InteractionComponent.
     *        The caller retains ownership and lifetime responsibility.
     *
     * No-op (asserts in debug) if the registry is already at
     * `kMaxRegistered` capacity or if `actor`/`comp` is null.
     */
    void registerActor(core::Actor* actor, InteractionComponent* comp);

    /**
     * @brief Unregisters a previously-registered actor. Safe to call for an
     *        actor that was never registered (no-op).
     */
    void unregisterActor(core::Actor* actor);

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
    /**
     * @brief Sets the (optional) event bus enter/exit edges are published to.
     *
     * Independent of the callback dispatch: with the bus unset, only
     * InteractionComponent::onEnter/onExit fire. With it set, a
     * `{TriggerEnter|TriggerExit, idA, idB, 0, nullptr}` GameplayEvent is
     * ALSO published for every edge (design.md D3).
     */
    void setEventBus(GameplayEventBus* bus) { eventBus_ = bus; }
#endif

    /** @brief Call once at the start of the per-frame contact loop. */
    void beginFrame();

    /**
     * @brief Records one physics contact pair for this frame's diff.
     * @param a First body in the contact (may be null-checked by the caller
     *        the same way `triggerCallbacks()` already does).
     * @param b Second body in the contact.
     *
     * Records ALL contact pairs, not only registered ones — the registry
     * lookup happens only for pairs whose presence actually changed, in
     * `endFrame()`.
     */
    void recordPair(core::Actor* a, core::Actor* b);

    /**
     * @brief Diffs `currPairs` against `prevPairs` and dispatches
     *        onEnter/onExit for every pair whose presence changed, then
     *        rotates curr into prev for the next frame.
     */
    void endFrame();

    /**
     * @brief Clears prevPairs/currPairs and the registry.
     *
     * Called from `CollisionSystem::clear()` and `Scene::resetState()` to
     * prevent stale contact pairs from dispatching through dangling `Actor*`
     * after a scene reset (design.md Risks section).
     */
    void reset();

private:
    struct RegistryEntry {
        uint16_t entityId = 0;
        core::Actor* actor = nullptr;
        InteractionComponent* comp = nullptr;
    };

    static uint32_t packPair(uint16_t idA, uint16_t idB);
    RegistryEntry* findEntry(uint16_t entityId);
    void dispatchEdge(uint32_t packedPair, bool isEnter);

    uint32_t currPairs_[kMaxContacts]{};
    int currCount_ = 0;
    uint32_t prevPairs_[kMaxContacts]{};
    int prevCount_ = 0;

    RegistryEntry registry_[kMaxRegistered]{};
    int registryCount_ = 0;

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
    GameplayEventBus* eventBus_ = nullptr;
#endif
};

}

#endif // PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
