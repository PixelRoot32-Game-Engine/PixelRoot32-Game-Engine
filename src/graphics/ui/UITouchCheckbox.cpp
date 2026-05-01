/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchCheckbox.cpp - Touch-optimized checkbox widget implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UIElement.h"
#include "graphics/ui/UITouchCheckbox.h"
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

UITouchCheckbox::UITouchCheckbox(
    std::string_view label,
    pixelroot32::math::Vector2 position,
    pixelroot32::math::Vector2 size,
    bool initialChecked,
    UIElementBoolCallback callback,
    int fontSizeVal)
    : UITouchElement(static_cast<int16_t>(static_cast<int>(position.x)),
                     static_cast<int16_t>(static_cast<int>(position.y)),
                     static_cast<uint16_t>(static_cast<int>(size.x)),
                     static_cast<uint16_t>(static_cast<int>(size.y)), UIWidgetType::Checkbox)
    , onChangedCallback(callback)
    , checked(initialChecked)
    , label(label)
    , normalColor(Color::White)
    , checkedColor(Color::Cyan)
    , disabledColor(Color::DarkGray)
    , borderColor(Color::Gray)
    , disabledBorderColor(Color::DarkGray)
    , fontSize(fontSizeVal)
    , pressStartPosition(Vector2::ZERO())
{
}

void UITouchCheckbox::setLabel(std::string_view newLabel) {
    label = newLabel;
}

void UITouchCheckbox::setChecked(bool newChecked) {
    if (checked != newChecked) {
        checked = newChecked;
        if (onChangedCallback) {
            onChangedCallback(checked);
        }
    }
}

void UITouchCheckbox::toggle() {
    if (isEnabled()) {
        setChecked(!checked);
    }
}

void UITouchCheckbox::setColors(Color normal, Color checked, Color disabled) {
    normalColor = normal;
    checkedColor = checked;
    disabledColor = disabled;
}

void UITouchCheckbox::setOnChanged(UITouchCheckbox::UIElementBoolCallback callback) {
    onChangedCallback = callback;
}

UITouchCheckbox::UIElementBoolCallback UITouchCheckbox::getOnChanged() const {
    return onChangedCallback;
}

void UITouchCheckbox::setFontSize(int size) {
    fontSize = size;
}

int UITouchCheckbox::getFontSize() const {
    return fontSize;
}

bool UITouchCheckbox::processEvent(const pixelroot32::input::TouchEvent& event) {
    if (!isEnabled() || !isVisible()) {
        return false;
    }
    
    // Only process touch events (ignore drag events)
    if (event.getType() != TouchEventType::TouchDown &&
        event.getType() != TouchEventType::TouchUp) {
        return false;
    }
    
    // Check if event is within our bounds
    if (!widgetData_.contains(event.x, event.y)) {
        // If we're in pressed state and touch moved outside, reset
        if (widgetData_.state == UIWidgetState::Pressed) {
            widgetData_.state = UIWidgetState::Idle;
            clearActive();
        }
        return false;
    }
    
    switch (event.getType()) {
        case pixelroot32::input::TouchEventType::TouchDown:
            handleTouchDown(event);
            return true;
            
        case pixelroot32::input::TouchEventType::TouchUp:
            handleTouchUp(event);
            return true;
            
        default:
            return false;
    }
}

void UITouchCheckbox::reset() {
    widgetData_.state = UIWidgetState::Idle;
    clearActive();
}

void UITouchCheckbox::handleTouchDown(const TouchEvent& event) {
    pressStartPosition = {toScalar(event.x), toScalar(event.y)};
    widgetData_.state = UIWidgetState::Pressed;
    setActive();
}

void UITouchCheckbox::handleTouchUp(const TouchEvent& event) {
    // Check if we dragged too far
    Scalar dx = toScalar(event.x) - pressStartPosition.x;
    Scalar dy = toScalar(event.y) - pressStartPosition.y;
    
    // Toggle only if we didn't drag too far
    if (widgetData_.state == UIWidgetState::Pressed && 
        dx * dx + dy * dy <= toScalar(DRAG_THRESHOLD * DRAG_THRESHOLD)) {
        toggle();
    }
    
    widgetData_.state = UIWidgetState::Idle;
    clearActive();
}

void UITouchCheckbox::setActive() {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(widgetData_.flags) | static_cast<uint8_t>(UIWidgetFlags::Active));
}

void UITouchCheckbox::clearActive() {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(widgetData_.flags) & ~static_cast<uint8_t>(UIWidgetFlags::Active));
}

Color UITouchCheckbox::getCurrentColor() const {
    if (!isEnabled()) {
        return disabledColor;
    } else if (checked) {
        return checkedColor;
    }
    return normalColor;
}

void UITouchCheckbox::draw(Renderer& renderer) {
    // Skip rendering if not visible
    if (!isVisible()) {
        return;
    }
    
    // Get bounds from Entity (synced from widget in update())
    int16_t x = static_cast<int16_t>(static_cast<int>(position.x));
    int16_t y = static_cast<int16_t>(static_cast<int>(position.y));
    uint16_t h = static_cast<uint16_t>(height);
    
    // Calculate checkbox box size (square based on font height)
    int boxSize = fontSize * 8;
    if (boxSize > static_cast<int>(h) - 4) {
        boxSize = static_cast<int>(h) - 4;
    }
    if (boxSize < 8) {
        boxSize = 8;
    }
    
    int boxX = x + 2;
    int boxY = y + (static_cast<int>(h) - boxSize) / 2;
    
    // Get color based on state
    Color color = getCurrentColor();
    
    // Draw checkbox border
    renderer.drawRectangle(boxX, boxY, boxSize, boxSize, color);
    
    // Draw checkbox background (filled only if checked - UICheckBox style)
    if (checked) {
        // Fill the box with same color as border (UICheckBox style: +2 offset, -4 size)
        renderer.drawFilledRectangle(boxX + 2, boxY + 2, boxSize - 4, boxSize - 4, color);
    }
    
    // Draw label text if present
    if (label.length() > 0) {
        int textHeight = fontSize * 8;
        if (textHeight > static_cast<int>(h)) {
            textHeight = static_cast<int>(h);
        }
        int textY = static_cast<int>(y) + (static_cast<int>(h) - textHeight) / 2;
        
        // Position text to the right of checkbox
        int textX = boxX + boxSize + 5;
        
        // Use appropriate text color
        Color textColor = isEnabled() ? Color::White : Color::Gray;
        
        renderer.drawText(label, textX, textY, textColor, fontSize);
    }
}

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
