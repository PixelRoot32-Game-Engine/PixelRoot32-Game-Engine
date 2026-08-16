/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "gameplay/InteractionTracker.h"

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS

#include <algorithm>
#include <cassert>

namespace pixelroot32::gameplay {

    namespace core = pixelroot32::core;
    using core::Actor;

    uint32_t InteractionTracker::packPair(uint16_t idA, uint16_t idB) {
        uint16_t lo = (idA < idB) ? idA : idB;
        uint16_t hi = (idA < idB) ? idB : idA;
        return (static_cast<uint32_t>(lo) << 16) | static_cast<uint32_t>(hi);
    }

    InteractionTracker::RegistryEntry* InteractionTracker::findEntry(uint16_t entityId) {
        for (int i = 0; i < registryCount_; ++i) {
            if (registry_[i].entityId == entityId) return &registry_[i];
        }
        return nullptr;
    }

    void InteractionTracker::registerActor(Actor* actor, InteractionComponent* comp) {
        assert(actor != nullptr && "registerActor: actor is null");
        assert(comp != nullptr && "registerActor: comp is null");
        assert(actor != nullptr && actor->entityId != 0 &&
               "registerActor: actor must be added to a CollisionSystem "
               "(which assigns entityId) BEFORE registering for interaction "
               "tracking");
        if (!actor || !comp || actor->entityId == 0) return;
        if (registryCount_ >= kMaxRegistered) return;  // Silently ignore, mirrors CollisionSystem::addEntity when full.

        RegistryEntry& entry = registry_[registryCount_++];
        entry.entityId = actor->entityId;
        entry.actor = actor;
        entry.comp = comp;
    }

    void InteractionTracker::unregisterActor(Actor* actor) {
        if (!actor) return;
        for (int i = 0; i < registryCount_; ++i) {
            if (registry_[i].actor == actor) {
                registry_[i] = registry_[--registryCount_];
                return;
            }
        }
    }

    void InteractionTracker::beginFrame() {
        currCount_ = 0;
    }

    void InteractionTracker::recordPair(Actor* a, Actor* b) {
        if (!a || !b) return;
        if (currCount_ >= kMaxContacts) return;  // Fixed capacity: extra pairs beyond kMaxContacts are dropped, matching contactCount's own cap.
        currPairs_[currCount_++] = packPair(a->entityId, b->entityId);
    }

    void InteractionTracker::dispatchEdge(uint32_t packedPair, bool isEnter) {
        const uint16_t idA = static_cast<uint16_t>(packedPair >> 16);
        const uint16_t idB = static_cast<uint16_t>(packedPair & 0xFFFFu);

        RegistryEntry* entryA = findEntry(idA);
        RegistryEntry* entryB = findEntry(idB);
        Actor* actorA = entryA ? entryA->actor : nullptr;
        Actor* actorB = entryB ? entryB->actor : nullptr;

        if (entryA && entryA->comp) {
            InteractionComponent::TriggerFn fn = isEnter ? entryA->comp->onEnter : entryA->comp->onExit;
            if (fn) fn(entryA->comp->owner, actorB);
        }
        if (entryB && entryB->comp) {
            InteractionComponent::TriggerFn fn = isEnter ? entryB->comp->onEnter : entryB->comp->onExit;
            if (fn) fn(entryB->comp->owner, actorA);
        }

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
        if (eventBus_) {
            GameplayEvent evt;
            evt.type = isEnter ? GameplayEventType::TriggerEnter : GameplayEventType::TriggerExit;
            evt.entityIdA = idA;
            evt.entityIdB = idB;
            eventBus_->publish(evt);
        }
#endif
    }

    void InteractionTracker::endFrame() {
        std::sort(currPairs_, currPairs_ + currCount_);
        std::sort(prevPairs_, prevPairs_ + prevCount_);

        int i = 0;
        int j = 0;
        while (i < currCount_ && j < prevCount_) {
            if (currPairs_[i] < prevPairs_[j]) {
                dispatchEdge(currPairs_[i], /*isEnter=*/true);
                ++i;
            } else if (currPairs_[i] > prevPairs_[j]) {
                dispatchEdge(prevPairs_[j], /*isEnter=*/false);
                ++j;
            } else {
                // Present in both frames: contact persists, no edge.
                ++i;
                ++j;
            }
        }
        while (i < currCount_) {
            dispatchEdge(currPairs_[i], /*isEnter=*/true);
            ++i;
        }
        while (j < prevCount_) {
            dispatchEdge(prevPairs_[j], /*isEnter=*/false);
            ++j;
        }

        // Rotate curr into prev for the next frame's diff.
        for (int k = 0; k < currCount_; ++k) {
            prevPairs_[k] = currPairs_[k];
        }
        prevCount_ = currCount_;
        currCount_ = 0;
    }

    void InteractionTracker::reset() {
        currCount_ = 0;
        prevCount_ = 0;
        registryCount_ = 0;
        for (int i = 0; i < kMaxContacts; ++i) {
            currPairs_[i] = 0;
            prevPairs_[i] = 0;
        }
        for (int i = 0; i < kMaxRegistered; ++i) {
            registry_[i] = RegistryEntry{};
        }
    }

}

#endif // PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
