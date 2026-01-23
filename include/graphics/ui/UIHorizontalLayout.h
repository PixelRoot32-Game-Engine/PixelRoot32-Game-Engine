/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once
#include "UILayout.h"
#include "input/InputManager.h"
#include "graphics/Color.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UIHorizontalLayout
 * @brief Horizontal layout container with scroll support.
 *
 * Organizes UI elements horizontally, one next to another. Supports scrolling
 * when content exceeds the visible viewport. Handles keyboard/D-pad
 * navigation automatically.
 */
class UIHorizontalLayout : public UILayout {
public:
    /**
     * @brief Constructs a new UIHorizontalLayout.
     * @param x X position of the layout container.
     * @param y Y position of the layout container.
     * @param w Width of the layout container (viewport width).
     * @param h Height of the layout container.
     */
    UIHorizontalLayout(float x, float y, float w, float h);

    virtual ~UIHorizontalLayout() = default;

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
        if (!enable) scrollOffset = 0.0f;
    }
    
    /**
     * @brief Enables or disables scrolling (alias for setScrollEnabled).
     * @param enable True to enable scrolling.
     */
    void enableScroll(bool enable) {
        setScrollEnabled(enable);
    }

    /**
     * @brief Sets the viewport width (visible area).
     * @param w Viewport width in pixels.
     */
    void setViewportWidth(float w) {
        width = static_cast<int>(w);
        updateLayout();
    }

    /**
     * @brief Gets the current scroll offset.
     * @return Scroll offset in pixels.
     */
    float getScrollOffset() const { return scrollOffset; }

    /**
     * @brief Sets the scroll offset directly.
     * @param offset Scroll offset in pixels.
     */
    void setScrollOffset(float offset);

    /**
     * @brief Gets the total content width.
     * @return Content width in pixels.
     */
    float getContentWidth() const { return contentWidth; }

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
    void setScrollSpeed(float speed) { scrollSpeed = speed; }

    /**
     * @brief Sets the navigation button indices.
     * @param leftButton Button index for LEFT navigation.
     * @param rightButton Button index for RIGHT navigation.
     */
    void setNavigationButtons(uint8_t leftButton, uint8_t rightButton) {
        navLeftButton = leftButton;
        navRightButton = rightButton;
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
    float contentWidth = 0.0f;         ///< Total width of all content
    float targetScrollOffset = 0.0f;   ///< Target scroll position (for smooth scrolling)
    float scrollSpeed = 0.5f;          ///< Scroll speed in pixels per millisecond
    int selectedIndex = -1;             ///< Currently selected element index
    uint8_t navLeftButton = 2;          ///< Button index for LEFT navigation
    uint8_t navRightButton = 3;         ///< Button index for RIGHT navigation
    bool wasLeftPressed = false;        ///< Previous state of LEFT button (for rising edge detection)
    bool wasRightPressed = false;       ///< Previous state of RIGHT button (for rising edge detection)
    bool needsClear = false;            ///< Flag to indicate if layout area needs clearing (performance optimization)
    float lastScrollOffset = 0.0f;      ///< Previous scroll offset to detect changes
    
    // Style colors for buttons
    pixelroot32::graphics::Color selectedTextColor = pixelroot32::graphics::Color::White;
    pixelroot32::graphics::Color selectedBgColor = pixelroot32::graphics::Color::Cyan;
    pixelroot32::graphics::Color unselectedTextColor = pixelroot32::graphics::Color::White;
    pixelroot32::graphics::Color unselectedBgColor = pixelroot32::graphics::Color::Black;

    /**
     * @brief Calculates the total content width.
     */
    void calculateContentWidth();

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
