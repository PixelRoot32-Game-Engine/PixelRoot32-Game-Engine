#pragma once
#include "Entity.h"

class Actor : public Entity {
public:
    Actor(float x, float y, int w, int h) : Entity(x, y, w, h) {}

    virtual ~Actor() = default;
    virtual Rect getHitBox() = 0;
    virtual void onCollision(Entity* other) = 0;
};