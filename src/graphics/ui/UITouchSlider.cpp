/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchSlider.cpp - Touch-optimized slider widget implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UITouchSlider.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

    namespace input = pixelroot32::input;
    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using input::TouchEvent;
    using input::TouchEventType;

UITouchSlider::UITouchSlider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t initialValue)
    : UITouchElement(x, y, w, h, UIWidgetType::Slider)
    , onValueChangedCallback(nullptr)
    , onDragStartCallback(nullptr)
    , onDragEndCallback(nullptr)
    , value(initialValue)
    , previousValue(initialValue)
    , dragStartPosition(Vector2::ZERO())
    , currentDragPosition(Vector2::ZERO())
    , trackColor(Color::Gray)
    , thumbColor(Color::White)
    , disabledColor(Color::Gray) {}

void UITouchSlider::setColors(Color track, Color thumb) {
    trackColor = track;
    thumbColor = thumb;
}

void UITouchSlider::setOnValueChanged(UITouchSlider::SliderCallback callback) {
    onValueChangedCallback = callback;
}

void UITouchSlider::setOnDragStart(UITouchSlider::SliderCallback callback) {
    onDragStartCallback = callback;
}

void UITouchSlider::setOnDragEnd(UITouchSlider::SliderCallback callback) {
    onDragEndCallback = callback;
}

UITouchSlider::SliderCallback UITouchSlider::getOnValueChanged() const {
    return onValueChangedCallback;
}

UITouchSlider::SliderCallback UITouchSlider::getOnDragStart() const {
    return onDragStartCallback;
}

UITouchSlider::SliderCallback UITouchSlider::getOnDragEnd() const {
    return onDragEndCallback;
}

uint8_t UITouchSlider::getValue() const {
    return value;
}

void UITouchSlider::setValue(uint8_t newValue) {
    previousValue = value;
    value = (newValue > MAX_VALUE) ? MAX_VALUE : newValue;
    
    if (value != previousValue && onValueChangedCallback) {
        onValueChangedCallback(value);
    }
}

uint8_t UITouchSlider::getPreviousValue() const {
    return previousValue;
}

bool UITouchSlider::hasValueChanged() const {
    return value != previousValue;
}

bool UITouchSlider::processEvent(const TouchEvent& event) {
    if (!isEnabled() || !isVisible()) {
        return false;
    }
    
    // Only process touch down/move/up events
    if (event.getType() != TouchEventType::TouchDown &&
        event.getType() != TouchEventType::DragMove &&
        event.getType() != TouchEventType::TouchUp) {
        return false;
    }
    
    // Check bounds using Entity position/size (synced from widget in update())
    int16_t ex = event.x;
    int16_t ey = event.y;
    int16_t bx = static_cast<int16_t>(position.x);
    int16_t by = static_cast<int16_t>(position.y);
    uint16_t bw = static_cast<uint16_t>(width);
    uint16_t bh = static_cast<uint16_t>(height);
    
    if (ex < bx || ex >= bx + static_cast<int16_t>(bw) ||
        ey < by || ey >= by + static_cast<int16_t>(bh)) {
        return false;
    }
    
    switch (event.getType()) {
        case TouchEventType::TouchDown:
            return handleTouchDown(event);
            
        case TouchEventType::DragMove:
            return handleDragMove(event);
            
        case TouchEventType::TouchUp:
            return handleTouchUp(event);
            
        default:
            return false;
    }
}

void UITouchSlider::reset() {
    widgetData_.state = UIWidgetState::Idle;
    clearActive();
}

bool UITouchSlider::handleTouchDown(const TouchEvent& event) {
    // Check if touch is within slider bounds - already checked in processEvent
    widgetData_.state = UIWidgetState::Dragging;
    setActive();
    dragStartPosition = {event.x, event.y};
    currentDragPosition = {event.x, event.y};
    
    // Calculate initial value based on touch position
    updateValueFromPosition(event.x);
    
    if (onDragStartCallback) {
        onDragStartCallback(value);
    }
    
    return true;
}

bool UITouchSlider::handleDragMove(const TouchEvent& event) {
    if (widgetData_.state != UIWidgetState::Dragging) {
        return false;
    }
    
    currentDragPosition = {event.x, event.y};
    updateValueFromPosition(event.x);
    
    return true;
}

bool UITouchSlider::handleTouchUp(const TouchEvent& event) {
    (void)event;
    if (widgetData_.state != UIWidgetState::Dragging) {
        return false;
    }
    
    widgetData_.state = UIWidgetState::Idle;
    clearActive();
    
    if (onDragEndCallback) {
        onDragEndCallback(value);
    }
    
    return true;
}

void UITouchSlider::updateValueFromPosition(int16_t xPos) {
    // Get slider bounds from Entity (synced from widget in update())
    int16_t sliderLeft = static_cast<int16_t>(position.x) + 4;   // 4 pixel padding
    int16_t sliderWidth = static_cast<int16_t>(width);
    int16_t sliderRight = sliderLeft + sliderWidth - 4;
    
    if (xPos <= sliderLeft) {
        setValue(MIN_VALUE);
    } else if (xPos >= sliderRight) {
        setValue(MAX_VALUE);
    } else {
        // Map position to value
        int16_t range = sliderRight - sliderLeft;
        int16_t offset = xPos - sliderLeft;
        uint8_t newValue = static_cast<uint8_t>((offset * MAX_VALUE) / range);
        setValue(newValue);
    }
}

void UITouchSlider::setActive() {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(widgetData_.flags) | static_cast<uint8_t>(UIWidgetFlags::Active));
}

void UITouchSlider::clearActive() {
    widgetData_.flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(widgetData_.flags) & ~static_cast<uint8_t>(UIWidgetFlags::Active));
}

void UITouchSlider::draw(pixelroot32::graphics::Renderer& renderer) {
    // Skip rendering if not visible
    if (!isVisible()) {
        return;
    }
    
    // Get bounds from Entity (synced from widget in update())
    int16_t x = static_cast<int16_t>(position.x);
    int16_t y = static_cast<int16_t>(position.y);
    uint16_t w = static_cast<uint16_t>(width);
    uint16_t h = static_cast<uint16_t>(height);
    
    // Draw track (horizontal line centered vertically)
    int16_t trackY = y + static_cast<int16_t>(h / 2) - 2;
    renderer.drawFilledRectangle(x + 4, trackY, w - 8, 4, trackColor);
    
    // Draw thumb at current value position
    int16_t thumbX = x + 4 + static_cast<int16_t>(((w - 8) * value) / MAX_VALUE);
    int16_t thumbY = y + static_cast<int16_t>(h / 2) - 6;
    renderer.drawFilledRectangle(thumbX - 3, thumbY, 6, 12, thumbColor);
    renderer.drawRectangle(thumbX - 3, thumbY, 6, 12, isEnabled() ? thumbColor : disabledColor);
}

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
