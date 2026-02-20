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
#include "core/Scene.h"
#include "core/Actor.h"
#include "graphics/Color.h"

namespace pixelroot32::core {

    using namespace pixelroot32::graphics;
    using namespace pixelroot32::physics;

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

    void Scene::update(unsigned long deltaTime) {
        // Flat Solver Pipeline
        // Physics integration and collision resolution now handled entirely by CollisionSystem
        
        // 1. Logic update — entities update game logic only (no physics integration)
        for (int i = 0; i < entityCount; i++) {
            if (entities[i]->isEnabled) {
                entities[i]->update(deltaTime);
            }
        }

        // 2. Physics update (Flat Solver)
        // Pipeline: Detect → Solve Velocity → Integrate Position → Solve Penetration → Callbacks
        unsigned long t0 = 0;
        if constexpr (pixelroot32::platforms::config::EnableProfiling) {
            t0 = pixelroot32::platforms::config::profilerMicros();
        }
        
        collisionSystem.update();

        if constexpr (pixelroot32::platforms::config::EnableProfiling) {
            gProfilerCollisionTime += pixelroot32::platforms::config::profilerMicros() - t0;
        }
    }

    void Scene::sortEntities() {
        for (int i = 0; i < entityCount - 1; i++) {
            for (int j = 0; j < entityCount - i - 1; j++) {
                if (entities[j]->getRenderLayer() > entities[j + 1]->getRenderLayer()) {
                    Entity* temp = entities[j];
                    entities[j] = entities[j + 1];
                    entities[j + 1] = temp;
                }
            }
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
            }
        }

        renderer.setRenderContext(nullptr);
    }

    void Scene::addEntity(Entity* entity) {
        if (entityCount < pixelroot32::platforms::config::MaxEntities) {
            entities[entityCount++] = entity;
            needsSorting = true;
            collisionSystem.addEntity(entity);
            if (entity->type == EntityType::ACTOR) {
                static_cast<Actor*>(entity)->collisionSystem = &collisionSystem;
            }
        }
    }

    void Scene::removeEntity(Entity* entity) {
        for (int i = 0; i < entityCount; i++) {
            if (entities[i] == entity) {
                collisionSystem.removeEntity(entity);
                for (int j = i; j < entityCount - 1; j++) {
                    entities[j] = entities[j + 1];
                }
                entityCount--;
                return;
            }
        }
    }

    void Scene::clearEntities() {
        for (int i = 0; i < entityCount; i++) {
            collisionSystem.removeEntity(entities[i]);
        }
        entityCount = 0;
    }   
}
