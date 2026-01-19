/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once
#include "core/Entity.h"

namespace pixelroot32::graphics::ui {

    enum TextAlignment {
        LEFT,
        CENTER,
        RIGHT
    };

/**
 * @class UIElement
 * @brief Base class for all user interface elements (buttons, labels, etc.).
 *
 * Inherits from Entity to integrate with the scene graph.
 * Sets the EntityType to UI_ELEMENT.
 */
class UIElement : public pixelroot32::core::Entity {
public:
    /**
     * @brief Constructs a new UIElement.
     * @param x X position.
     * @param y Y position.
     * @param w Width.
     * @param h Height.
     */
    UIElement(float x, float y, float w, float h) : pixelroot32::core::Entity(x, y, w, h, pixelroot32::core::EntityType::UI_ELEMENT) {
        setRenderLayer(2);
    }
    
    virtual ~UIElement() = default;
};

}
