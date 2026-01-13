#pragma once
#include "Entity.h"

class UIElement : public Entity {
public:
    UIElement(float x, float y, float w, float h) : Entity(x, y, w, h) {}
    
    virtual ~UIElement() = default;
    
    bool isVisible = true;
    bool isEnabled = true;
};