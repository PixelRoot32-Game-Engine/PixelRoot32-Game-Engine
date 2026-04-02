/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchButton.cpp - Touch-optimized button widget implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UITouchButton.h"
#include "graphics/FontManager.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

    namespace input = pixelroot32::input;
    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using math::toScalar;
    using input::TouchEvent;
    using input::TouchEventType;

UITouchButton::UITouchButton(std::string_view t, int16_t x, int16_t y, uint16_t w, uint16_t h)
    : UITouchElement(x, y, w, h, UIWidgetType::Button)
    , onDownCallback(nullptr)
    , onUpCallback(nullptr)
    , onClickCallback(nullptr)
    , pressStartPosition(Vector2::ZERO())
    , label(t)
    , normalColor(Color::White)
    , pressedColor(Color::Gray)
    , disabledColor(Color::DarkGray)
    , borderColor(Color::Gray)
    , disabledBorderColor(Color::DarkGray)
{
}

void UITouchButton::setLabel(std::string_view newLabel) {
    label = newLabel;
}

void UITouchButton::setColors(Color normal, Color pressed, Color disabled) {
    normalColor = normal;
    pressedColor = pressed;
    disabledColor = disabled;
}

void UITouchButton::setOnDown(UITouchButton::ButtonCallback callback) {
    onDownCallback = callback;
}

void UITouchButton::setOnUp(UITouchButton::ButtonCallback callback) {
    onUpCallback = callback;
}

void UITouchButton::setOnClick(UITouchButton::ButtonCallback callback) {
    onClickCallback = callback;
}

UITouchButton::ButtonCallback UITouchButton::getOnDown() const {
    return onDownCallback;
}

UITouchButton::ButtonCallback UITouchButton::getOnUp() const {
    return onUpCallback;
}

UITouchButton::ButtonCallback UITouchButton::getOnClick() const {
    return onClickCallback;
}

bool UITouchButton::processEvent(const pixelroot32::input::TouchEvent& event) {
    if (!isEnabled() || !isVisible()) {
        return false;
    }
    
    // Only process touch events
    if (event.type != TouchEventType::TouchDown &&
        event.type != TouchEventType::TouchUp &&
        event.type != TouchEventType::Click) {
        return false;
    }
    
    // Check if event is within our bounds
    if (!widgetData_.contains(event.x, event.y)) {
        // If we're in pressed/dragging state and touch moved outside, reset
        if (widgetData_.state == UIWidgetState::Pressed || widgetData_.state == UIWidgetState::Dragging) {
            widgetData_.state = UIWidgetState::Idle;
            clearActive();
        }
        return false;
    }
    
    switch (event.type) {
        case pixelroot32::input::TouchEventType::TouchDown:
            handleTouchDown(event);
            return true;
            
        case pixelroot32::input::TouchEventType::TouchUp:
            handleTouchUp(event);
            return true;
            
        case pixelroot32::input::TouchEventType::Click:
            handleClick(event);
            return true;
            
        default:
            return false;
    }
}

void UITouchButton::reset() {
    widgetData_.state = UIWidgetState::Idle;
    clearActive();
}

void UITouchButton::handleTouchDown(const TouchEvent& event) {
    widgetData_.state = UIWidgetState::Pressed;
    setActive();
    pressStartPosition = {toScalar(event.x), toScalar(event.y)};
    
    if (onDownCallback) {
        onDownCallback();
    }
}

void UITouchButton::handleTouchUp(const TouchEvent& event) {
    // Check if we dragged too far
    Scalar dx = toScalar(event.x) - pressStartPosition.x;
    Scalar dy = toScalar(event.y) - pressStartPosition.y;
    
    if (dx * dx + dy * dy > toScalar(DRAG_THRESHOLD * DRAG_THRESHOLD)) {
        // Dragged too far - no click
        widgetData_.state = UIWidgetState::Idle;
    } else {
        widgetData_.state = UIWidgetState::Idle;
    }
    
    clearActive();
    
    if (onUpCallback) {
        onUpCallback();
    }
}

void UITouchButton::handleClick(const TouchEvent& event) {
    (void)event;
    if (onClickCallback) {
        onClickCallback();
    }
}

void UITouchButton::setActive() {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(widgetData_.flags) | static_cast<uint8_t>(UIWidgetFlags::Active));
}

void UITouchButton::clearActive() {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(widgetData_.flags) & ~static_cast<uint8_t>(UIWidgetFlags::Active));
}

Color UITouchButton::getCurrentColor() const {
    Color c = normalColor;

    if (!isEnabled()) {
        c = disabledColor;
    } else if (isPressed()) {
        c = pressedColor;
    }

    return c;
}

void UITouchButton::draw(Renderer& renderer) {
    // Skip rendering if not visible
    if (!isVisible()) {
        return;
    }
    
    // Get bounds from Entity (synced from widget in update())
    int16_t x = static_cast<int16_t>(position.x);
    int16_t y = static_cast<int16_t>(position.y);
    uint16_t w = static_cast<uint16_t>(width);
    uint16_t h = static_cast<uint16_t>(height);
    
    // Get color based on state
    Color color = getCurrentColor();
    
    // Draw button background (filled rectangle)
    renderer.drawFilledRectangle(x, y, w, h, color);
    
    // Draw button border (darker color)
    Color border = isEnabled() ? borderColor : disabledBorderColor;
    renderer.drawRectangle(x, y, w, h, border);
    
    // Draw label text if present
    if (label.length() > 0) {
        int fontSize = 2;  // Default font size
        int textHeight = fontSize * 8;
        if (textHeight > static_cast<int>(h)) {
            textHeight = static_cast<int>(h);
        }
        int textY = static_cast<int>(y) + (static_cast<int>(h) - textHeight) / 2;
        
        // Calculate text width using FontManager
        int textWidth = FontManager::textWidth(nullptr, label, fontSize);
        
        // Center the text horizontally
        int textX = static_cast<int>(x) + (static_cast<int>(w) - textWidth) / 2;
        
        // Use white text color
        Color textColor = isEnabled() ? Color::White : Color::Gray;
        
        renderer.drawText(label, textX, textY, textColor, fontSize);
    }
}

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
