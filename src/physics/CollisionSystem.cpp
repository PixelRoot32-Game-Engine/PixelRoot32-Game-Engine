#include "physics/CollisionSystem.h"
#include "core/Actor.h"

namespace pixelroot32::physics {

    using namespace pixelroot32::core;
    using namespace pixelroot32::physics;

    void CollisionSystem::addEntity(Entity* e) { entities.push_back(e); }
    void CollisionSystem::removeEntity(Entity* e) { 
        entities.erase(std::remove(entities.begin(), entities.end(), e), entities.end());
    }

    void CollisionSystem::update() {
        for (size_t i = 0; i < entities.size(); i++) {
            // 1. We ask the Entity (thanks to the virtual method in the database)
            if (entities[i]->type != EntityType::ACTOR) continue; 

            // 2. If we get here, it's safe to use static_cast (much faster than dynamic_cast)
            Actor* actorA = static_cast<Actor*>(entities[i]);

            for (size_t j = i + 1; j < entities.size(); j++) {
                if (entities[j]->type != EntityType::ACTOR) continue;
                
                Actor* actorB = static_cast<Actor*>(entities[j]);

                // 3. We verify bitmasks (our "Collision Matrix")
                if ((actorA->mask & actorB->layer) || (actorB->mask & actorA->layer)) {
                    if (actorA->getHitBox().intersects(actorB->getHitBox())) {
                        actorA->onCollision(actorB);
                        actorB->onCollision(actorA);
                    }
                }
            }
        }
    }
}