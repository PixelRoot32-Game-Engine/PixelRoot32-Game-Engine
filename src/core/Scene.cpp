#include "core/Scene.h"

namespace pixelroot32::core {

    using namespace pixelroot32::graphics;
    using namespace pixelroot32::physics;

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
        int count = entities.itemCount();
        for (int i = 0; i < count; i++) {
            Entity* entity = entities.dequeue();  // Get entity from queue
            
            // Draw entity if it is visible
            if(entity->isVisible) {
                entity->draw(renderer);
            }
            
            entities.enqueue(entity);  // Re-add entity to maintain order
        }
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
