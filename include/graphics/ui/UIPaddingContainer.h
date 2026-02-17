/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
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
    UIPaddingContainer(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

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
    void setPadding(pixelroot32::math::Scalar p);

    /**
     * @brief Sets asymmetric padding.
     * @param left Left padding in pixels.
     * @param right Right padding in pixels.
     * @param top Top padding in pixels.
     * @param bottom Bottom padding in pixels.
     */
    void setPadding(pixelroot32::math::Scalar left, pixelroot32::math::Scalar right, pixelroot32::math::Scalar top, pixelroot32::math::Scalar bottom);

    /**
     * @brief Gets the left padding.
     * @return Left padding in pixels.
     */
    pixelroot32::math::Scalar getPaddingLeft() const { return paddingLeft; }

    /**
     * @brief Gets the right padding.
     * @return Right padding in pixels.
     */
    pixelroot32::math::Scalar getPaddingRight() const { return paddingRight; }

    /**
     * @brief Gets the top padding.
     * @return Top padding in pixels.
     */
    pixelroot32::math::Scalar getPaddingTop() const { return paddingTop; }

    /**
     * @brief Gets the bottom padding.
     * @return Bottom padding in pixels.
     */
    pixelroot32::math::Scalar getPaddingBottom() const { return paddingBottom; }

    /**
     * @brief Sets the position of the container.
     * Also updates the child element's position.
     * @param newX New X coordinate.
     * @param newY New Y coordinate.
     */
    void setPosition(pixelroot32::math::Scalar newX, pixelroot32::math::Scalar newY) override;

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
    pixelroot32::math::Scalar paddingLeft = pixelroot32::math::toScalar(0);     ///< Left padding
    pixelroot32::math::Scalar paddingRight = pixelroot32::math::toScalar(0);   ///< Right padding
    pixelroot32::math::Scalar paddingTop = pixelroot32::math::toScalar(0);     ///< Top padding
    pixelroot32::math::Scalar paddingBottom = pixelroot32::math::toScalar(0);  ///< Bottom padding

    /**
     * @brief Updates the child element's position based on padding.
     */
    void updateChildPosition();
};

}
