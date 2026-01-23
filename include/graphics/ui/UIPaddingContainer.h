/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once
#include "UIElement.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UIPaddingContainer
 * @brief Container that wraps a single UI element and applies padding.
 *
 * This container adds padding/margin around a single child element without
 * organizing multiple elements. Useful for adding spacing to individual
 * elements or nesting layouts with custom padding.
 */
class UIPaddingContainer : public UIElement {
public:
    /**
     * @brief Constructs a new UIPaddingContainer.
     * @param x X position of the container.
     * @param y Y position of the container.
     * @param w Width of the container.
     * @param h Height of the container.
     */
    UIPaddingContainer(float x, float y, float w, float h);

    virtual ~UIPaddingContainer() = default;

    /**
     * @brief Sets the child element.
     * @param element Pointer to the UI element to wrap.
     */
    void setChild(UIElement* element);

    /**
     * @brief Gets the child element.
     * @return Pointer to the child element, or nullptr if none set.
     */
    UIElement* getChild() const { return child; }

    /**
     * @brief Sets uniform padding on all sides.
     * @param p Padding value in pixels.
     */
    void setPadding(float p);

    /**
     * @brief Sets asymmetric padding.
     * @param left Left padding in pixels.
     * @param right Right padding in pixels.
     * @param top Top padding in pixels.
     * @param bottom Bottom padding in pixels.
     */
    void setPadding(float left, float right, float top, float bottom);

    /**
     * @brief Gets the left padding.
     * @return Left padding in pixels.
     */
    float getPaddingLeft() const { return paddingLeft; }

    /**
     * @brief Gets the right padding.
     * @return Right padding in pixels.
     */
    float getPaddingRight() const { return paddingRight; }

    /**
     * @brief Gets the top padding.
     * @return Top padding in pixels.
     */
    float getPaddingTop() const { return paddingTop; }

    /**
     * @brief Gets the bottom padding.
     * @return Bottom padding in pixels.
     */
    float getPaddingBottom() const { return paddingBottom; }

    /**
     * @brief Sets the position of the container.
     * Also updates the child element's position.
     * @param newX New X coordinate.
     * @param newY New Y coordinate.
     */
    void setPosition(float newX, float newY);

    /**
     * @brief Updates the container and child element.
     * @param deltaTime Time elapsed since last frame in milliseconds.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws the child element.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    UIElement* child = nullptr;  ///< Child element
    float paddingLeft = 0.0f;     ///< Left padding
    float paddingRight = 0.0f;   ///< Right padding
    float paddingTop = 0.0f;     ///< Top padding
    float paddingBottom = 0.0f;  ///< Bottom padding

    /**
     * @brief Updates the child element's position based on padding.
     */
    void updateChildPosition();
};

}
