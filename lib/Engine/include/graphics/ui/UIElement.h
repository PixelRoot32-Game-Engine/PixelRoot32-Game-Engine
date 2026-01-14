#pragma once
#include "Entity.h"

/**
 * @class UIElement
 * @brief Base class for all user interface elements (buttons, labels, etc.).
 *
 * Inherits from Entity to integrate with the scene graph.
 * Sets the EntityType to UI_ELEMENT.
 */
class UIElement : public Entity {
public:
    /**
     * @brief Constructs a new UIElement.
     * @param x X position.
     * @param y Y position.
     * @param w Width.
     * @param h Height.
     */
    UIElement(float x, float y, float w, float h) : Entity(x, y, w, h, EntityType::UI_ELEMENT) {}
    
    virtual ~UIElement() = default;
};