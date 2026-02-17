/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "UILayout.h"
#include "input/InputManager.h"
#include "graphics/Color.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UIVerticalLayout
 * @brief Vertical layout container with scroll support.
 *
 * Organizes UI elements vertically, one below another. Supports scrolling
 * when content exceeds the visible viewport. Handles keyboard/D-pad
 * navigation automatically.
 */
class UIVerticalLayout : public UILayout {
public:
    /**
     * @brief Constructs a new UIVerticalLayout.
     * @param x X position of the layout container.
     * @param y Y position of the layout container.
     * @param w Width of the layout container.
     * @param h Height of the layout container (viewport height).
     */
    UIVerticalLayout(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

    virtual ~UIVerticalLayout() = default;

    /**
     * @brief Adds a UI element to the layout.
     * @param element Pointer to the element to add.
     */
    void addElement(UIElement* element) override;

    /**
     * @brief Removes a UI element from the layout.
     * @param element Pointer to the element to remove.
     */
    void removeElement(UIElement* element) override;

    /**
     * @brief Recalculates positions of all elements.
     */
    void updateLayout() override;

    /**
     * @brief Handles input for navigation and scrolling.
     * @param input Reference to the InputManager.
     */
    void handleInput(const pixelroot32::input::InputManager& input) override;

    /**
     * @brief Updates the layout (handles smooth scrolling).
     * @param deltaTime Time elapsed since last frame in milliseconds.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws the layout and its visible elements.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Enables or disables scrolling.
     * @param enable True to enable scrolling.
     */
    void setScrollEnabled(bool enable) {
        UILayout::enableScroll = enable;
        if (!enable) scrollOffset = pixelroot32::math::toScalar(0.0f);
    }
    
    /**
     * @brief Enables or disables scrolling (alias for setScrollEnabled).
     * @param enable True to enable scrolling.
     */
    void enableScroll(bool enable) {
        setScrollEnabled(enable);
    }

    /**
     * @brief Sets the viewport height (visible area).
     * @param h Viewport height in pixels.
     */
    void setViewportHeight(pixelroot32::math::Scalar h) {
        height = static_cast<int>(h);
        updateLayout();
    }

    /**
     * @brief Gets the current scroll offset.
     * @return Scroll offset in pixels.
     */
    pixelroot32::math::Scalar getScrollOffset() const { return scrollOffset; }

    /**
     * @brief Sets the scroll offset directly.
     * @param offset Scroll offset in pixels.
     */
    void setScrollOffset(pixelroot32::math::Scalar offset);

    /**
     * @brief Gets the total content height.
     * @return Content height in pixels.
     */
    pixelroot32::math::Scalar getContentHeight() const { return contentHeight; }

    /**
     * @brief Gets the currently selected element index.
     * @return Selected index, or -1 if none selected.
     */
    int getSelectedIndex() const { return selectedIndex; }

    /**
     * @brief Sets the selected element index.
     * @param index Index to select (-1 to deselect).
     */
    void setSelectedIndex(int index);

    /**
     * @brief Gets the selected element.
     * @return Pointer to selected element, or nullptr if none selected.
     */
    UIElement* getSelectedElement() const;

    /**
     * @brief Sets the scroll speed for smooth scrolling.
     * @param speed Pixels per millisecond.
     */
    void setScrollSpeed(pixelroot32::math::Scalar speed) { scrollSpeed = speed; }

    /**
     * @brief Sets the navigation button indices.
     * @param upButton Button index for UP navigation.
     * @param downButton Button index for DOWN navigation.
     */
    void setNavigationButtons(uint8_t upButton, uint8_t downButton) {
        navUpButton = upButton;
        navDownButton = downButton;
    }

    /**
     * @brief Sets the style colors for selected and unselected buttons.
     * @param selectedTextCol Text color when selected.
     * @param selectedBgCol Background color when selected.
     * @param unselectedTextCol Text color when not selected.
     * @param unselectedBgCol Background color when not selected.
     */
    void setButtonStyle(pixelroot32::graphics::Color selectedTextCol,
                       pixelroot32::graphics::Color selectedBgCol,
                       pixelroot32::graphics::Color unselectedTextCol,
                       pixelroot32::graphics::Color unselectedBgCol);

private:
    pixelroot32::math::Scalar contentHeight = pixelroot32::math::toScalar(0.0f);        ///< Total height of all content
    pixelroot32::math::Scalar targetScrollOffset = pixelroot32::math::toScalar(0.0f);   ///< Target scroll position (for smooth scrolling)
    pixelroot32::math::Scalar scrollSpeed = pixelroot32::math::toScalar(0.5f);          ///< Scroll speed in pixels per millisecond
    int selectedIndex = -1;             ///< Currently selected element index
    uint8_t navUpButton = 0;            ///< Button index for UP navigation
    uint8_t navDownButton = 1;          ///< Button index for DOWN navigation
    bool wasUpPressed = false;          ///< Previous state of UP button (for rising edge detection)
    bool wasDownPressed = false;        ///< Previous state of DOWN button (for rising edge detection)
    bool needsClear = false;            ///< Flag to indicate if layout area needs clearing (performance optimization)
    pixelroot32::math::Scalar lastScrollOffset = pixelroot32::math::toScalar(0.0f);      ///< Previous scroll offset to detect changes
    
    // Style colors for buttons
    pixelroot32::graphics::Color selectedTextColor = pixelroot32::graphics::Color::White;
    pixelroot32::graphics::Color selectedBgColor = pixelroot32::graphics::Color::Cyan;
    pixelroot32::graphics::Color unselectedTextColor = pixelroot32::graphics::Color::White;
    pixelroot32::graphics::Color unselectedBgColor = pixelroot32::graphics::Color::Black;

    /**
     * @brief Calculates the total content height.
     */
    void calculateContentHeight();

    /**
     * @brief Updates element visibility based on scroll position.
     */
    void updateElementVisibility();

    /**
     * @brief Ensures the selected element is visible by adjusting scroll.
     */
    void ensureSelectedVisible();

    /**
     * @brief Clamps scroll offset to valid range.
     */
    void clampScrollOffset();
};

}
