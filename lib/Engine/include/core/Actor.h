#pragma once
#include "Entity.h"
#include <CollisionTypes.h>

class Actor : public Entity {
public:
    Actor(float x, float y, int w, int h) : Entity(x, y, w, h, EntityType::ACTOR) {}
    virtual ~Actor() = default;

    CollisionLayer layer = DefaultLayers::kNone;
    CollisionLayer mask  = DefaultLayers::kNone;

    void setCollisionLayer(CollisionLayer l) { layer = l; }
    void setCollisionMask(CollisionLayer m)  { mask = m; }

    void update(unsigned long dt) override {

    }

    /**
     * Comprueba si el Actor pertenece a una o varias capas específicas.
     * @param targetLayer El bit o bits a consultar (ej: Layers::BALL)
     * @return true si el bit está encendido en la identidad del actor.
     */
    bool isInLayer(uint16_t targetLayer) const {
        // Operación bitwise AND: Si el resultado no es 0, el bit existe.
        return (layer & targetLayer) != 0;
    }

    virtual Rect getHitBox() = 0;
    virtual void onCollision(Actor* other) = 0;
};