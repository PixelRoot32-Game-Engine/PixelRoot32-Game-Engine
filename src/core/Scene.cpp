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

#include "core/EngineModules.h"
#include "core/Scene.h"
#include "core/Actor.h"
#include "graphics/Color.h"
#include <cassert>

namespace pixelroot32::core {

    namespace modules = pixelroot32::modules;
    namespace gfx = pixelroot32::graphics;
    namespace phy = pixelroot32::physics;

    using gfx::Renderer;
    using gfx::PaletteContext;

    extern unsigned long gProfilerCollisionTime;

    void SceneArena::init(void* memory, std::size_t size) {
        if constexpr (pixelroot32::platforms::config::EnableSceneArena) {
            buffer = static_cast<unsigned char*>(memory);
            capacity = size;
            offset = 0;
        }
    }

    void SceneArena::reset() {
        if constexpr (pixelroot32::platforms::config::EnableSceneArena) {
            offset = 0;
        }
    }

    void* SceneArena::allocate(std::size_t size, std::size_t alignment) {
        if constexpr (pixelroot32::platforms::config::EnableSceneArena) {
            std::size_t current = reinterpret_cast<std::size_t>(buffer) + offset;
            std::size_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
            std::size_t newOffset = aligned - reinterpret_cast<std::size_t>(buffer) + size;
            if (newOffset > capacity) {
                return nullptr;
            }
            offset = newOffset;
            return buffer + (aligned - reinterpret_cast<std::size_t>(buffer));
        } else {
            return nullptr;
        }
    }

    void Scene::processTouchEvents(pixelroot32::input::TouchEvent* events, uint8_t count) {
        if (events == nullptr || count == 0) {
            return;
        }

        #if PIXELROOT32_ENABLE_UI_SYSTEM
            uiManager.processEvents(events, count);
        #endif

        for (uint8_t i = 0; i < count; ++i) {
            if (!events[i].isConsumed()) {
                onUnconsumedTouchEvent(events[i]);
            }
        }
    }

    void Scene::update(unsigned long deltaTime) {
        // Flat Solver Pipeline
        // Physics integration and collision resolution now handled entirely by CollisionSystem
        
        // 1. Logic update — entities update game logic only (no physics integration)
        for (int i = 0; i < entityCount; i++) {
            if (entities[i]->isEnabled) {
                entities[i]->update(deltaTime);
            }
        }

        // 2. Physics update with fixed timestep scheduler
        unsigned long t0 = 0;
        if constexpr (pixelroot32::platforms::config::EnableProfiling) {
            t0 = pixelroot32::platforms::config::profilerMicros();
        }
        
        #if PIXELROOT32_ENABLE_PHYSICS
            // Use fixed timestep scheduler for physics (converts deltaTime ms to micros)
            physicsScheduler.update(deltaTime * 1000, collisionSystem);
        #endif

        if constexpr (pixelroot32::platforms::config::EnableProfiling) {
            gProfilerCollisionTime += pixelroot32::platforms::config::profilerMicros() - t0;
        }
    }

    void Scene::sortEntities() {
        for (int i = 1; i < entityCount; i++) {
            Entity* key = entities[i];
            int j = i - 1;

            while (j >= 0 && entities[j]->getRenderLayer() > key->getRenderLayer()) {
                entities[j + 1] = entities[j];
                j--;
            }

            entities[j + 1] = key;
        }

        needsSorting = false;
    }

    bool Scene::isVisibleInViewport(Entity* entity, Renderer& renderer) {
        int viewX = -renderer.getXOffset();
        int viewY = -renderer.getYOffset();
        int viewW = renderer.getLogicalWidth();
        int viewH = renderer.getLogicalHeight();

        return !(entity->position.x + entity->width < viewX || 
                 entity->position.x > viewX + viewW ||
                 entity->position.y + entity->height < viewY || 
                 entity->position.y > viewY + viewH);
    }

    void Scene::draw(Renderer& renderer) {
        if (needsSorting) {
            sortEntities();
        }

        PaletteContext backgroundContext = PaletteContext::Background;
        PaletteContext spriteContext = PaletteContext::Sprite;
        unsigned char currentLayer = 255;

        for (int i = 0; i < entityCount; ++i) {
            Entity* entity = entities[i];

            if (!entity->isVisible) continue;

            if (entity->getRenderLayer() != currentLayer) {
                currentLayer = entity->getRenderLayer();
                if (currentLayer == 0) {
                    renderer.setRenderContext(&backgroundContext);
                } else {
                    renderer.setRenderContext(&spriteContext);
                }
            }

            if (isVisibleInViewport(entity, renderer)) {
                entity->draw(renderer);
                // Auto-mark dirty bounds after entity draws (if enabled)
                if (entity->isAutoMarkDirty()) {
                    auto bounds = entity->getDirtyBounds();
                    renderer.getDrawSurface().markDirty(
                        static_cast<int>(entity->position.x) + static_cast<int>(bounds.position.x),
                        static_cast<int>(entity->position.y) + static_cast<int>(bounds.position.y),
                        bounds.width,
                        bounds.height
                    );
                }
            }
        }

        renderer.setRenderContext(nullptr);
    }

    void Scene::addEntity(Entity* entity) {
        assert(entity != nullptr && "Cannot add null entity to scene");
        if (entityCount < pixelroot32::platforms::config::MaxEntities) {
            entities[entityCount++] = entity;
            needsSorting = true;

            #if PIXELROOT32_ENABLE_PHYSICS
                collisionSystem.addEntity(entity);
            
                if (entity->type == EntityType::ACTOR) {
                    static_cast<Actor*>(entity)->collisionSystem = &collisionSystem;
                }
            #endif
        }
    }

    void Scene::removeEntity(Entity* entity) {
        assert(entity != nullptr && "Cannot remove null entity from scene");
        for (int i = 0; i < entityCount; i++) {
            if (entities[i] == entity) {
                #if PIXELROOT32_ENABLE_PHYSICS
                    collisionSystem.removeEntity(entity);
                #endif
                
                for (int j = i; j < entityCount - 1; j++) {
                    entities[j] = entities[j + 1];
                }
                entityCount--;
                return;
            }
        }
    }

    void Scene::clearEntities() {
        #if PIXELROOT32_ENABLE_PHYSICS
            for (int i = 0; i < entityCount; i++) {
                collisionSystem.removeEntity(entities[i]);
            }
        #endif
        entityCount = 0;
    }   
}
