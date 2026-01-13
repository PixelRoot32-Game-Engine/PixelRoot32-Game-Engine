#pragma once
#include "graphics/Renderer.h"

struct Rect {
    float x, y;
    int width, height;

    bool intersects(const Rect& other) const {
        return !(x + width < other.x || x > other.x + other.width ||
                 y + height < other.y || y > other.y + other.height);
    }
};

enum class EntityType { GENERIC, ACTOR, UI_ELEMENT };

class Entity {
public:
    float x, y;
    int width, height;
    EntityType type;

    Entity(float x, float y, int w, int h, EntityType t) 
        : x(x), y(y), width(w), height(h), type(t) {}
        
    virtual ~Entity() {}
    
    virtual void update(unsigned long deltaTimet) = 0;
    virtual void draw(Renderer& renderer) = 0;
};
