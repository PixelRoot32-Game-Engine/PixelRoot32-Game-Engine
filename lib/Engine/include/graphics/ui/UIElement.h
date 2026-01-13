#pragma once
#include "Entity.h"

class UIElement : public Entity {
public:
    UIElement(float x, float y, float w, float h) : Entity(x, y, w, h, EntityType::UI_ELEMENT) {}
    virtual ~UIElement() = default;
    
    bool isVisible = true;
    virtual void setVisible(bool v) { isVisible = v; }

    bool isEnabled = true;
    virtual void setEnabled(bool e) { isEnabled = e; }
};