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

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    void SceneArena::init(void* memory, std::size_t size) {
        buffer = static_cast<unsigned char*>(memory);
        capacity = size;
        offset = 0;
    }

    void SceneArena::reset() {
        offset = 0;
    }

    void* SceneArena::allocate(std::size_t size, std::size_t alignment) {
        std::size_t current = reinterpret_cast<std::size_t>(buffer) + offset;
        std::size_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
        std::size_t newOffset = aligned - reinterpret_cast<std::size_t>(buffer) + size;
        if (newOffset > capacity) {
            return nullptr;
        }
        offset = newOffset;
        return buffer + (aligned - reinterpret_cast<std::size_t>(buffer));
    }
#endif

    void Scene::update(unsigned long deltaTime) {
        int count = entities.itemCount();
        for (int i = 0; i < count; i++) {
            Entity* entity = entities.dequeue();  // Get entity from queue
            entity->update(deltaTime);
            entities.enqueue(entity);  // Re-add entity to maintain order
        }

        //  update collision system after remoeving all entities
        collisionSystem.update();
    }

    void Scene::draw(Renderer& renderer) {
        const unsigned char maxLayers = MAX_LAYERS;
        int count = entities.itemCount();

        // Context for palette selection based on render layer
        PaletteContext backgroundContext = PaletteContext::Background;
        PaletteContext spriteContext = PaletteContext::Sprite;

        for (unsigned char layer = 0; layer < maxLayers; ++layer) {
            // Set render context based on layer:
            // Layer 0 = Background, Layer 1+ = Sprite
            if (layer == 0) {
                renderer.setRenderContext(&backgroundContext);
            } else {
                renderer.setRenderContext(&spriteContext);
            }

            for (int i = 0; i < count; ++i) {
                Entity* entity = entities.dequeue();

                if (entity->isVisible && entity->getRenderLayer() == layer) {
                    entity->draw(renderer);
                }

                entities.enqueue(entity);
            }
        }

        // Reset context to nullptr after drawing all layers
        renderer.setRenderContext(nullptr);
    }

    void Scene::addEntity(Entity* entity) {
        entities.enqueue(entity);
        collisionSystem.addEntity(entity); // sync with collision system
    }

    void Scene::removeEntity(Entity* entity) {
        int count = entities.itemCount();
        for (int i = 0; i < count; i++) {
            Entity* e = entities.dequeue();
            if (e == entity) {
                collisionSystem.removeEntity(e);
                continue;
            }
            entities.enqueue(e);
        }
    }

    void Scene::clearEntities() {
        while (!entities.isEmpty()) {
            Entity* e = entities.dequeue();
            collisionSystem.removeEntity(e); // sync with collision system
        }
    }   
}
