/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "UILayout.h"
#include <vector>
#include <utility>

#include "platforms/EngineConfig.h"

namespace pixelroot32::graphics::ui {

/**
 * @enum Anchor
 * @brief Defines anchor points for positioning UI elements.
 */
enum class Anchor {
    TOP_LEFT,      ///< Top-left corner
    TOP_RIGHT,     ///< Top-right corner
    BOTTOM_LEFT,   ///< Bottom-left corner
    BOTTOM_RIGHT,  ///< Bottom-right corner
    CENTER,        ///< Center of screen
    TOP_CENTER,    ///< Top center
    BOTTOM_CENTER, ///< Bottom center
    LEFT_CENTER,   ///< Left center
    RIGHT_CENTER   ///< Right center
};

/**
 * @class UIAnchorLayout
 * @brief Layout that positions elements at fixed anchor points on the screen.
 *
 * This layout positions UI elements at fixed anchor points (corners, center, etc.)
 * without reflow. Very efficient for HUDs, debug UI, and fixed-position elements.
 * Positions are calculated once or when screen size changes.
 */
class UIAnchorLayout : public UILayout {
public:
    /**
     * @brief Constructs a new UIAnchorLayout.
     * @param x X position of the layout container (usually 0).
     * @param y Y position of the layout container (usually 0).
     * @param w Width of the layout container (usually screen width).
     * @param h Height of the layout container (usually screen height).
     */
    UIAnchorLayout(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

    virtual ~UIAnchorLayout() = default;

    /**
     * @brief Adds a UI element with a specific anchor point.
     * @param element Pointer to the element to add.
     * @param anchor Anchor point for positioning.
     */
    void addElement(UIElement* element, Anchor anchor);

    /**
     * @brief Adds a UI element to the layout (defaults to TOP_LEFT anchor).
     * @param element Pointer to the element to add.
     */
    void addElement(UIElement* element) override;

    /**
     * @brief Removes a UI element from the layout.
     * @param element Pointer to the element to remove.
     */
    void removeElement(UIElement* element) override;

    /**
     * @brief Recalculates positions of all elements based on anchors.
     */
    void updateLayout() override;

    /**
     * @brief Handles input (no-op for anchor layout, elements handle their own input).
     * @param input Reference to the InputManager.
     */
    void handleInput(const pixelroot32::input::InputManager& input) override;

    /**
     * @brief Updates the layout and child elements.
     * @param deltaTime Time elapsed since last frame in milliseconds.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws all elements.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Sets the screen size for anchor calculations.
     * @param screenWidth Screen width in pixels.
     * @param screenHeight Screen height in pixels.
     */
    void setScreenSize(pixelroot32::math::Scalar screenWidth, pixelroot32::math::Scalar screenHeight);

    /**
     * @brief Gets the screen width.
     * @return Screen width in pixels.
     */
    pixelroot32::math::Scalar getScreenWidth() const { return screenWidth; }

    /**
     * @brief Gets the screen height.
     * @return Screen height in pixels.
     */
    pixelroot32::math::Scalar getScreenHeight() const { return screenHeight; }

private:
    std::vector<std::pair<UIElement*, Anchor>> anchoredElements;  ///< Elements with their anchor points
    pixelroot32::math::Scalar screenWidth = pixelroot32::math::Scalar(pixelroot32::platforms::config::LogicalWidth);   ///< Screen width for anchor calculations (logical resolution)
    pixelroot32::math::Scalar screenHeight = pixelroot32::math::Scalar(pixelroot32::platforms::config::LogicalHeight);  ///< Screen height for anchor calculations (logical resolution)

    /**
     * @brief Calculates position for an element based on its anchor.
     * @param element The element to position.
     * @param anchor The anchor point.
     * @param outX Output parameter for calculated X position.
     * @param outY Output parameter for calculated Y position.
     */
    void calculateAnchorPosition(UIElement* element, Anchor anchor, pixelroot32::math::Scalar& outX, pixelroot32::math::Scalar& outY) const;
};

}
