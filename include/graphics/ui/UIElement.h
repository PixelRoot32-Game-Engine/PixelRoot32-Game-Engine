/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
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

    /**
     * @brief Sets the position of the element.
     * @param newX New X coordinate.
     * @param newY New Y coordinate.
     */
    void setPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }

    /**
     * @brief Gets the preferred size of the element.
     * Used by layouts to determine how much space the element needs.
     * @param preferredWidth Output parameter for preferred width (or -1 if flexible).
     * @param preferredHeight Output parameter for preferred height (or -1 if flexible).
     */
    virtual void getPreferredSize(float& preferredWidth, float& preferredHeight) const {
        preferredWidth = static_cast<float>(width);
        preferredHeight = static_cast<float>(height);
    }
};

}
