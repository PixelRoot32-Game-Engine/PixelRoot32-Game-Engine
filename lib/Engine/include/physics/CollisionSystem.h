#pragma once
#include <vector>
#include <algorithm>
#include "core/Entity.h"

class CollisionSystem {
public:
    void addEntity(Entity* e);
    void removeEntity(Entity* e);
    void update();

private:
    std::vector<Entity*> entities;
};
