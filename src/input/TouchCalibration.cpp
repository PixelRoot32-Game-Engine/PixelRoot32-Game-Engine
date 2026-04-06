/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchCalibration - Implementation of calibration and factory presets
 */
#include "input/TouchAdapter.h"
#include "math/MathUtil.h"

namespace pixelroot32::input {

TouchCalibration::TouchCalibration()
    : scaleX(1.0f)
    , scaleY(1.0f)
    , offsetX(0)
    , offsetY(0)
    , displayWidth(320)
    , displayHeight(240)
    , rotation(TouchRotation::Rotation0) {
}

TouchCalibration TouchCalibration::fromPreset(DisplayPreset preset) {
    return createForDisplay(preset);
}

TouchCalibration TouchCalibration::forResolution(int16_t width, int16_t height) {
    TouchCalibration calib;
    calib.displayWidth = width;
    calib.displayHeight = height;
    
    // Auto-calculate scales based on typical 12-bit ADC range (0-4095)
    // These are heuristic values that can be overridden by actual calibration
    if (width > 0) {
        calib.scaleX = static_cast<float>(width) / 4096.0f;
    }
    if (height > 0) {
        calib.scaleY = static_cast<float>(height) / 4096.0f;
    }
    
    return calib;
}

TouchPoint TouchCalibration::transform(int16_t rawX, int16_t rawY, bool pressed, uint8_t id, uint32_t ts) const {
    // Apply scale and offset first
    int16_t scaledX = static_cast<int16_t>((rawX * scaleX) + offsetX);
    int16_t scaledY = static_cast<int16_t>((rawY * scaleY) + offsetY);
    
    // Apply rotation transformation
    int16_t rotatedX, rotatedY;
    applyRotation(scaledX, scaledY, rotatedX, rotatedY);
    
    // Clamp to display bounds
    int16_t screenX = rotatedX;
    int16_t screenY = rotatedY;
    
    if (screenX < 0) screenX = 0;
    if (screenX > displayWidth) screenX = displayWidth;
    if (screenY < 0) screenY = 0;
    if (screenY > displayHeight) screenY = displayHeight;
    
    return TouchPoint(screenX, screenY, pressed, id, ts);
}

void TouchCalibration::setRotation(TouchRotation rot) {
    rotation = rot;
}

void TouchCalibration::applyRotation(int16_t x, int16_t y, int16_t& outX, int16_t& outY) const {
    switch (rotation) {
        case TouchRotation::Rotation0:
            outX = x;
            outY = y;
            break;
        case TouchRotation::Rotation90:
            // 90 degrees clockwise: (x, y) -> (height - y, x)
            outX = displayHeight - y;
            outY = x;
            break;
        case TouchRotation::Rotation180:
            // 180 degrees: (x, y) -> (width - x, height - y)
            outX = displayWidth - x;
            outY = displayHeight - y;
            break;
        case TouchRotation::Rotation270:
            // 270 degrees clockwise: (x, y) -> (y, width - x)
            outX = y;
            outY = displayWidth - x;
            break;
        default:
            outX = x;
            outY = y;
            break;
    }
}

TouchRotation TouchCalibration::inverted() const {
    // Return opposite rotation
    switch (rotation) {
        case TouchRotation::Rotation0:   return TouchRotation::Rotation0;
        case TouchRotation::Rotation90:  return TouchRotation::Rotation270;
        case TouchRotation::Rotation180: return TouchRotation::Rotation180;
        case TouchRotation::Rotation270: return TouchRotation::Rotation90;
        default: return TouchRotation::Rotation0;
    }
}

TouchCalibration TouchCalibration::createForDisplay(DisplayPreset preset) {
    TouchCalibration calib;
    
    switch (preset) {
        case DisplayPreset::ILI9341_320x240:
            calib.displayWidth = 320;
            calib.displayHeight = 240;
            calib.scaleX = 1.05f;   // ILI9341 typical
            calib.scaleY = 1.10f;
            calib.offsetX = -15;
            calib.offsetY = -20;
            break;
            
        case DisplayPreset::ST7789_240x320:
            calib.displayWidth = 240;
            calib.displayHeight = 320;
            calib.scaleX = 0.9f;    // ST7789 portrait typical
            calib.scaleY = 0.9f;
            calib.offsetX = 0;
            calib.offsetY = 0;
            break;
            
        case DisplayPreset::ST7789_240x240:
            calib.displayWidth = 240;
            calib.displayHeight = 240;
            calib.scaleX = 0.88f;
            calib.scaleY = 0.88f;
            calib.offsetX = 10;
            calib.offsetY = 10;
            break;
            
        case DisplayPreset::ST7735_128x160:
            calib.displayWidth = 128;
            calib.displayHeight = 160;
            calib.scaleX = 0.65f;    // ST7735 128x160 typical
            calib.scaleY = 0.65f;
            calib.offsetX = 10;
            calib.offsetY = 25;
            break;
            
        case DisplayPreset::ST7735_128x128:
            calib.displayWidth = 128;
            calib.displayHeight = 128;
            calib.scaleX = 0.62f;    // ST7735 128x128 typical
            calib.scaleY = 0.62f;
            calib.offsetX = 0;
            calib.offsetY = 0;
            break;
            
        case DisplayPreset::ILI9488_320x480:
            calib.displayWidth = 320;
            calib.displayHeight = 480;
            calib.scaleX = 1.08f;    // ILI9488 3.5" typical
            calib.scaleY = 1.08f;
            calib.offsetX = 0;
            calib.offsetY = 0;
            break;
            
        case DisplayPreset::GC9A01_240x240:
            calib.displayWidth = 240;
            calib.displayHeight = 240;
            calib.scaleX = 0.90f;    // GC9A01 round typical
            calib.scaleY = 0.90f;
            calib.offsetX = 0;
            calib.offsetY = 0;
            break;
            
        case DisplayPreset::Custom:
        case DisplayPreset::None:
        default:
            // Default values
            calib.displayWidth = 320;
            calib.displayHeight = 240;
            calib.scaleX = 1.0f;
            calib.scaleY = 1.0f;
            break;
    }
    
    return calib;
}

} // namespace pixelroot32::input