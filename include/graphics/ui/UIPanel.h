/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "UIElement.h"
#include "graphics/Color.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UIPanel
 * @brief Visual container that draws a background and border around a child element.
 *
 * This container provides a retro-style window/panel appearance with a background
 * color and border. Typically contains a UILayout or other UI elements. Useful for
 * dialogs, menus, and information panels.
 */
class UIPanel : public UIElement {
public:
    /**
     * @brief Constructs a new UIPanel.
     * @param x X position of the panel.
     * @param y Y position of the panel.
     * @param w Width of the panel.
     * @param h Height of the panel.
     */
    UIPanel(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

    virtual ~UIPanel() = default;

    /**
     * @brief Sets the child element.
     * @param element Pointer to the UI element to wrap (typically a UILayout).
     */
    void setChild(UIElement* element);

    /**
     * @brief Gets the child element.
     * @return Pointer to the child element, or nullptr if none set.
     */
    UIElement* getChild() const { return child; }

    /**
     * @brief Sets the background color.
     * @param color Background color.
     */
    void setBackgroundColor(pixelroot32::graphics::Color color) {
        backgroundColor = color;
    }

    /**
     * @brief Gets the background color.
     * @return Background color.
     */
    pixelroot32::graphics::Color getBackgroundColor() const {
        return backgroundColor;
    }

    /**
     * @brief Sets the border color.
     * @param color Border color.
     */
    void setBorderColor(pixelroot32::graphics::Color color) {
        borderColor = color;
    }

    /**
     * @brief Gets the border color.
     * @return Border color.
     */
    pixelroot32::graphics::Color getBorderColor() const {
        return borderColor;
    }

    /**
     * @brief Sets the border width.
     * @param width Border width in pixels.
     */
    void setBorderWidth(uint8_t width) {
        borderWidth = width;
    }

    /**
     * @brief Gets the border width.
     * @return Border width in pixels.
     */
    uint8_t getBorderWidth() const {
        return borderWidth;
    }

    /**
     * @brief Sets the position of the panel.
     * Also updates the child element's position.
     * @param newX New X coordinate.
     * @param newY New Y coordinate.
     */
    void setPosition(pixelroot32::math::Scalar newX, pixelroot32::math::Scalar newY) override;

    /**
     * @brief Updates the panel and child element.
     * @param deltaTime Time elapsed since last frame in milliseconds.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws the panel (background, border) and child element.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    UIElement* child = nullptr;                          ///< Child element
    pixelroot32::graphics::Color backgroundColor = pixelroot32::graphics::Color::Black;  ///< Background color
    pixelroot32::graphics::Color borderColor = pixelroot32::graphics::Color::White;        ///< Border color
    uint8_t borderWidth = 1;                             ///< Border width in pixels

    /**
     * @brief Updates the child element's position to match the panel.
     */
    void updateChildPosition();
};

}
