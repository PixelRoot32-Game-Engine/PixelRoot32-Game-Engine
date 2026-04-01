/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchFactory - Factory functions for creating touch system
 * Provides presets and easy initialization
 */
#pragma once

#include "input/TouchManager.h"
#include "input/TouchAdapter.h"
#include "input/TouchPoint.h"

namespace pixelroot32::input {

/**
 * @class TouchFactory
 * @brief Factory for creating touch system configurations
 * 
 * Provides convenient factory methods to create TouchManager
 * with common display presets and configurations.
 * 
 * Usage:
 *   auto manager = TouchFactory::createForILI9341();
 *   manager->init();
 */
class TouchFactory {
public:
    /**
     * @brief Create TouchManager for ILI9341 display (320x240)
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional, 255 if not used)
     * @return Initialized TouchManager
     */
    static TouchManager createForILI9341(uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(320, 240);
        TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ILI9341_320x240);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager for ST7789 display (240x320 portrait)
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createForST7789_240x320(uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(240, 320);
        TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7789_240x320);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager for ST7789 display (240x240 round)
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createForST7789_240x240(uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(240, 240);
        TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7789_240x240);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager for ST7735 display (128x160)
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createForST7735_128x160(uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(128, 160);
        TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7735_128x160);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager for ST7735 display (128x128)
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createForST7735_128x128(uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(128, 128);
        TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ST7735_128x128);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager for ILI9488 display (320x480)
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createForILI9488(uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(320, 480);
        TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::ILI9488_320x480);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager for GC9A01 display (240x240 round)
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createForGC9A01(uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(240, 240);
        TouchCalibration calib = TouchCalibration::fromPreset(DisplayPreset::GC9A01_240x240);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager for custom resolution
     * @param width Display width
     * @param height Display height
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createForResolution(uint16_t width, uint16_t height, 
                                            uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchManager manager(width, height);
        TouchCalibration calib = TouchCalibration::forResolution(width, height);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Create TouchManager from DisplayPreset enum
     * @param preset Display preset
     * @param csPin SPI CS pin number
     * @param irqPin IRQ pin (optional)
     * @return Initialized TouchManager
     */
    static TouchManager createFromPreset(DisplayPreset preset, 
                                          uint8_t csPin = 5, uint8_t irqPin = 255) {
        TouchCalibration calib = TouchCalibration::fromPreset(preset);
        TouchManager manager(calib.displayWidth, calib.displayHeight);
        manager.setCalibration(calib);
        return manager;
    }
    
    /**
     * @brief Get display preset from width/height
     * @param width Display width
     * @param height Display height
     * @return Matching DisplayPreset or Custom if no match
     */
    static DisplayPreset detectPreset(uint16_t width, uint16_t height) {
        if (width == 320 && height == 240) return DisplayPreset::ILI9341_320x240;
        if (width == 240 && height == 320) return DisplayPreset::ST7789_240x320;
        if (width == 240 && height == 240) return DisplayPreset::ST7789_240x240;
        if (width == 128 && height == 160) return DisplayPreset::ST7735_128x160;
        if (width == 128 && height == 128) return DisplayPreset::ST7735_128x128;
        if (width == 320 && height == 480) return DisplayPreset::ILI9488_320x480;
        if (width == 240 && height == 240) return DisplayPreset::GC9A01_240x240;
        return DisplayPreset::Custom;
    }
};

} // namespace pixelroot32::input