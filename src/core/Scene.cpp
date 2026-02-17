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
#include "graphics/Color.h"

namespace pixelroot32::core {

    using namespace pixelroot32::graphics;
    using namespace pixelroot32::physics;

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
        for (int i = 0; i < entityCount; i++) {
            if (entities[i]->isEnabled) {
                entities[i]->update(deltaTime);
            }
        }

        // update collision system
        collisionSystem.update();
    }

    void Scene::sortEntities() {
        // Simple bubble sort for layers (usually small number of entities)
        // If entityCount is large, we could use std::sort or something else.
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
        // Calculate viewport boundaries in world space
        // renderer.getXOffset() and getYOffset() are negative camera positions
        int viewX = -renderer.getXOffset();
        int viewY = -renderer.getYOffset();
        int viewW = renderer.getLogicalWidth();
        int viewH = renderer.getLogicalHeight();

        // Check intersection
        return !(entity->x + entity->width < viewX || 
                 entity->x > viewX + viewW ||
                 entity->y + entity->height < viewY || 
                 entity->y > viewY + viewH);
    }

    void Scene::draw(Renderer& renderer) {
        if (needsSorting) {
            sortEntities();
        }

        // Context for palette selection based on render layer
        PaletteContext backgroundContext = PaletteContext::Background;
        PaletteContext spriteContext = PaletteContext::Sprite;
        unsigned char currentLayer = 255;

        for (int i = 0; i < entityCount; ++i) {
            Entity* entity = entities[i];

            if (!entity->isVisible) continue;

            // Update render context only when layer changes
            if (entity->getRenderLayer() != currentLayer) {
                currentLayer = entity->getRenderLayer();
                if (currentLayer == 0) {
                    renderer.setRenderContext(&backgroundContext);
                } else {
                    renderer.setRenderContext(&spriteContext);
                }
            }

            // Entity Culling: Only draw if within viewport
            if (isVisibleInViewport(entity, renderer)) {
                entity->draw(renderer);
            }
        }

        // Reset context to nullptr after drawing
        renderer.setRenderContext(nullptr);
    }

    void Scene::addEntity(Entity* entity) {
        if (entityCount < pixelroot32::platforms::config::MaxEntities) {
            entities[entityCount++] = entity;
            needsSorting = true;
            collisionSystem.addEntity(entity);
        }
    }

    void Scene::removeEntity(Entity* entity) {
        for (int i = 0; i < entityCount; i++) {
            if (entities[i] == entity) {
                collisionSystem.removeEntity(entity);
                // Shift remaining entities
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
