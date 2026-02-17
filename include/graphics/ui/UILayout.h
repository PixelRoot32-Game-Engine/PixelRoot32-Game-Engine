/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "UIElement.h"
#include "input/InputManager.h"
#include <vector>

namespace pixelroot32::graphics::ui {

/**
 * @enum ScrollBehavior
 * @brief Defines how scrolling behaves in layouts.
 */
enum class ScrollBehavior {
    NONE,    ///< No scrolling allowed
    SCROLL,  ///< Scroll freely within bounds
    CLAMP    ///< Scroll but clamp to content bounds
};

/**
 * @class UILayout
 * @brief Base class for UI layout containers.
 *
 * Layouts organize UI elements automatically, handling positioning,
 * spacing, and optional scrolling. Layouts are themselves UI elements
 * that can be added to scenes.
 */
class UILayout : public UIElement {
public:
    /**
     * @brief Constructs a new UILayout.
     * @param x X position of the layout container.
     * @param y Y position of the layout container.
     * @param w Width of the layout container.
     * @param h Height of the layout container.
     */
    UILayout(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h)
        : UIElement(x, y, w, h, UIElementType::LAYOUT) {}

    virtual ~UILayout() = default;

    /**
     * @brief Adds a UI element to the layout.
     * @param element Pointer to the element to add.
     */
    virtual void addElement(UIElement* element) = 0;

    /**
     * @brief Removes a UI element from the layout.
     * @param element Pointer to the element to remove.
     */
    virtual void removeElement(UIElement* element) = 0;

    /**
     * @brief Recalculates positions of all elements in the layout.
     * Should be called automatically when elements are added/removed.
     */
    virtual void updateLayout() = 0;

    /**
     * @brief Handles input for layout navigation (scroll, selection, etc.).
     * @param input Reference to the InputManager.
     */
    virtual void handleInput(const pixelroot32::input::InputManager& input) = 0;

    /**
     * @brief Sets the padding (internal spacing) of the layout.
     * @param p Padding value in pixels.
     */
    void setPadding(pixelroot32::math::Scalar p) { padding = p; updateLayout(); }

    /**
     * @brief Gets the current padding.
     * @return Padding value in pixels.
     */
    pixelroot32::math::Scalar getPadding() const { return padding; }

    /**
     * @brief Sets the spacing between elements.
     * @param s Spacing value in pixels.
     */
    void setSpacing(pixelroot32::math::Scalar s) { spacing = s; updateLayout(); }

    /**
     * @brief Gets the current spacing.
     * @return Spacing value in pixels.
     */
    pixelroot32::math::Scalar getSpacing() const { return spacing; }

    /**
     * @brief Gets the number of elements in the layout.
     * @return Element count.
     */
    size_t getElementCount() const { return elements.size(); }

    /**
     * @brief Gets the element at a specific index.
     * @param index Element index.
     * @return Pointer to the element, or nullptr if index is invalid.
     */
    UIElement* getElement(size_t index) const {
        if (index >= elements.size()) return nullptr;
        return elements[index];
    }

    /**
     * @brief Clears all elements from the layout.
     */
    void clearElements();

protected:
    std::vector<UIElement*> elements;  ///< List of child elements
    pixelroot32::math::Scalar padding = pixelroot32::math::toScalar(0.0f);              ///< Internal padding
    pixelroot32::math::Scalar spacing = pixelroot32::math::toScalar(4.0f);              ///< Spacing between elements
    pixelroot32::math::Scalar scrollOffset = pixelroot32::math::toScalar(0.0f);         ///< Current scroll offset
    bool enableScroll = false;         ///< Whether scrolling is enabled
    ScrollBehavior scrollBehavior = ScrollBehavior::CLAMP; ///< Scroll behavior mode
};

}
