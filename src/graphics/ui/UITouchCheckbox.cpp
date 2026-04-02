/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchCheckbox.cpp - Touch-optimized checkbox widget implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UITouchCheckbox.h"
#include "graphics/FontManager.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

    namespace input = pixelroot32::input;
    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using input::TouchEvent;
    using input::TouchEventType;

UITouchCheckbox::UITouchCheckbox(std::string_view label, int16_t x, int16_t y, 
                                 uint16_t w, uint16_t h, bool initialChecked)
    : UITouchElement(x, y, w, h, UIWidgetType::Checkbox)
    , onChangedCallback(nullptr)
    , checked(initialChecked)
    , label(label)
    , normalColor(Color::White)
    , checkedColor(Color::Cyan)
    , disabledColor(Color::DarkGray)
    , borderColor(Color::Gray)
    , disabledBorderColor(Color::DarkGray)
    , fontSize(2)
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

void UITouchCheckbox::setOnChanged(UITouchCheckbox::CheckboxCallback callback) {
    onChangedCallback = callback;
}

UITouchCheckbox::CheckboxCallback UITouchCheckbox::getOnChanged() const {
    return onChangedCallback;
}

bool UITouchCheckbox::processEvent(const pixelroot32::input::TouchEvent& event) {
    if (!isEnabled() || !isVisible()) {
        return false;
    }
    
    // Only process touch events
    if (event.type != TouchEventType::TouchDown &&
        event.type != TouchEventType::TouchUp) {
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
    
    switch (event.type) {
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
    (void)event;
    widgetData_.state = UIWidgetState::Pressed;
    setActive();
}

void UITouchCheckbox::handleTouchUp(const TouchEvent& event) {
    (void)event;
    
    // Toggle on release (click behavior)
    if (widgetData_.state == UIWidgetState::Pressed) {
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
    int16_t x = static_cast<int16_t>(position.x);
    int16_t y = static_cast<int16_t>(position.y);
    uint16_t w = static_cast<uint16_t>(width);
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
    Color border = isEnabled() ? borderColor : disabledBorderColor;
    
    // Draw checkbox border
    renderer.drawRectangle(boxX, boxY, boxSize, boxSize, border);
    
    // Draw checkbox background (filled if checked or pressed)
    if (checked || (isPressed() && isEnabled())) {
        // Fill the box with checked color
        renderer.drawFilledRectangle(boxX + 1, boxY + 1, boxSize - 2, boxSize - 2, color);
        
        // Draw checkmark (simple X or ✓)
        int margin = boxSize / 4;
        int x1 = boxX + margin;
        int y1 = boxY + boxSize / 2;
        int x2 = boxX + boxSize / 2;
        int y2 = boxY + boxSize - margin;
        int x3 = boxX + boxSize - margin;
        int y3 = boxY + margin;
        
        // Draw checkmark in white (or contrasting color)
        Color checkColor = Color::White;
        renderer.drawLine(x1, y1, x2, y2, checkColor);
        renderer.drawLine(x2, y2, x3, y3, checkColor);
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
