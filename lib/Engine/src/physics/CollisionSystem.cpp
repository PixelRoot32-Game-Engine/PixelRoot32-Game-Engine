#include "physics/CollisionSystem.h"
#include "Actor.h"

void CollisionSystem::addEntity(Entity* e) { entities.push_back(e); }
void CollisionSystem::removeEntity(Entity* e) { 
    entities.erase(std::remove(entities.begin(), entities.end(), e), entities.end());
}

void CollisionSystem::update() {
    for (size_t i = 0; i < entities.size(); i++) {
        for (size_t j = i + 1; j < entities.size(); j++) {
            
            // Si tu vector es de Entity*
            Actor* actorA = static_cast<Actor*>(entities[i]);
            Actor* actorB = static_cast<Actor*>(entities[j]);

            // Verificamos que ambos sean punteros válidos (no null)
            if (actorA && actorB) {
                if (actorA->getHitBox().intersects(actorB->getHitBox())) {
                    actorA->onCollision(actorB); // Enviamos el puntero al otro actor
                    actorB->onCollision(actorA);
                }
            }
        }
    }
}