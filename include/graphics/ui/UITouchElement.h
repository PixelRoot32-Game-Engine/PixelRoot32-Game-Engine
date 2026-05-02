/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchElement.h - Entity wrapper for UITouchWidget
 * Enables draw() rendering for touch widgets with embedded widget data
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UIElement.h"
#include "graphics/ui/UITouchWidget.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UITouchElement
 * @brief UIElement with embedded UITouchWidget data for touch interaction.
 *
 * Inherits from UIElement.
 *
 * Embeds widget data directly (x, y, width, height, flags, state)
 * to avoid memory corruption from overlapping placement new.
 * Integrates with the UILayout system.
 *
 * Memory: Owns widget data inline - no external allocation needed.
 */
class UITouchElement : public UIElement {
public:
    /**
     * @brief Construct UITouchElement with position and size
     */
    UITouchElement(int16_t x, int16_t y, uint16_t w, uint16_t h, UIWidgetType type);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~UITouchElement() = default;
    
    /**
     * @brief Update widget logic
     * @param deltaTime Time since last frame in ms
     */
    void update(unsigned long deltaTime) override;
    
    /**
     * @brief Render the widget
     * @param renderer Reference to renderer
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Process a touch event (polymorphic dispatch from UIManager).
     * @return true if this element handled the event for UI consumption semantics
     */
    virtual bool processEvent(const pixelroot32::input::TouchEvent& event) = 0;
    
    /**
     * @brief Get widget state
     * @return Current UIWidgetState
     */
    virtual uint8_t getWidgetState() const;
    
    /**
     * @brief Check if widget is currently pressed
     * @return true if state is Pressed
     */
    virtual bool isPressed() const;
    
    /**
     * @brief Check if widget is enabled
     * @return true if widget is enabled
     */
    virtual bool isEnabled() const;
    
    /**
     * @brief Check if widget is visible
     * @return true if widget is visible
     */
    virtual bool isVisible() const;
    
    /**
     * @brief Set widget visibility
     * @param visible True to make visible
     */
    void setWidgetVisible(bool visible);
    
    /**
     * @brief Set widget enabled state
     * @param enabled True to enable
     */
    void setWidgetEnabled(bool enabled);
    
    /**
     * @brief Override setPosition to sync widgetData_ with Entity position
     * @param newX New X coordinate
     * @param newY New Y coordinate
     */
    void setPosition(pixelroot32::math::Scalar newX, pixelroot32::math::Scalar newY) override {
        UIElement::setPosition(newX, newY);
        widgetData_.x = static_cast<int16_t>(static_cast<int>(newX));
        widgetData_.y = static_cast<int16_t>(static_cast<int>(newY));
    }
    
    /**
     * @brief Get widget x position
     * @return X position
     */
    int16_t getX() const { return widgetData_.x; }
    
    /**
     * @brief Get widget y position
     * @return Y position
     */
    int16_t getY() const { return widgetData_.y; }
    
    /**
     * @brief Get widget width
     * @return Width
     */
    uint16_t getWidgetWidth() const { return widgetData_.width; }
    
    /**
     * @brief Get widget height
     * @return Height
     */
    uint16_t getWidgetHeight() const { return widgetData_.height; }

protected:
    UITouchWidget widgetData_;  ///< Embedded widget data (owned)
    
public:
    /**
     * @brief Get reference to embedded widget data
     * @return Reference to UITouchWidget data
     */
    UITouchWidget& getWidgetData() { return widgetData_; }
    
    /**
     * @brief Get const reference to embedded widget data
     * @return Const reference to UITouchWidget data
     */
    const UITouchWidget& getWidgetData() const { return widgetData_; }
};

// Inline implementation
inline UITouchElement::UITouchElement(int16_t x, int16_t y, uint16_t w, uint16_t h, UIWidgetType type)
    : UIElement(
        pixelroot32::math::toScalar(x),
        pixelroot32::math::toScalar(y),
        w,
        h,
        UIElementType::GENERIC)
    , widgetData_(type, 0, x, y, w, h) {
    // Widget data already has Visible and Enabled flags set by default
}

inline void UITouchElement::update(unsigned long deltaTime) {
    (void)deltaTime;
    // Widget data is already in sync - no external state to sync
    // Subclasses can override for additional logic
}

inline void UITouchElement::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
    // Default placeholder - subclasses override for actual rendering
    // This is a no-op by default
}

inline uint8_t UITouchElement::getWidgetState() const {
    return static_cast<uint8_t>(widgetData_.state);
}

inline bool UITouchElement::isPressed() const {
    return widgetData_.state == UIWidgetState::Pressed;
}

inline bool UITouchElement::isEnabled() const {
    return widgetData_.isEnabled();
}

inline bool UITouchElement::isVisible() const {
    return widgetData_.isVisible();
}

inline void UITouchElement::setWidgetVisible(bool visible) {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        visible ? (static_cast<uint8_t>(widgetData_.flags) | static_cast<uint8_t>(UIWidgetFlags::Visible)) :
                  (static_cast<uint8_t>(widgetData_.flags) & ~static_cast<uint8_t>(UIWidgetFlags::Visible)));
    setVisible(visible);
}

inline void UITouchElement::setWidgetEnabled(bool enabled) {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        enabled ? (static_cast<uint8_t>(widgetData_.flags) | static_cast<uint8_t>(UIWidgetFlags::Enabled)) :
                  (static_cast<uint8_t>(widgetData_.flags) & ~static_cast<uint8_t>(UIWidgetFlags::Enabled)));
    setEnabled(enabled);
}

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM