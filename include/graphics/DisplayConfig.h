/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#pragma once

#ifdef PLATFORM_NATIVE
    #include <platforms/mock/MockSPI.h>
    #include <drivers/native/SDL2_Drawer.h> 
    #include "DrawSurface.h"
#else
    #include "platforms/PlatformDefaults.h"
    #if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)
        #include <drivers/esp32/TFT_eSPI_Drawer.h>
    #elif defined(PIXELROOT32_USE_U8G2_DRIVER)
        #include <drivers/esp32/U8G2_Drawer.h>
    #endif
    #include <SPI.h>
#endif
#include "platforms/EngineConfig.h"
#include <cassert>
#include <memory>

namespace pixelroot32::graphics {

/**
 * @enum DisplayType
 * @brief Identifies the type of display driver to use.
 */
enum DisplayType {
    ST7789, // TFT
    ST7735, // TFT
    ILI9341, // TFT 
    ILI9341_2, // TFT Alternative ILI9341 driver, see https://github.com/Bodmer/TFT_eSPI/issues/1172
    OLED_SSD1306, // 128x64 OLED (U8G2)
    OLED_SH1106,  // 128x64 OLED (U8G2)
    NONE,   // for SDL2 native no driver.
    CUSTOM  // User-provided DrawSurface implementation
}; 

/**
 * @brief Configuration settings for initializing displays with optional resolution scaling.
 * 
 * Supports both physical (hardware) and logical (rendering) resolutions.
 * When logical resolution is smaller than physical, the engine automatically
 * scales the output using nearest-neighbor interpolation.
 * 
 * Use this structure to define the hardware pins, display dimensions, rotation,
 * and communication type (I2C or SPI).
 */
struct DisplayConfig {
public:
    DisplayType type;
    int rotation = 0;
    
    /// Physical display resolution (hardware)
    uint16_t physicalWidth;
    uint16_t physicalHeight;
    
    /// Logical rendering resolution (what the game draws to)
    uint16_t logicalWidth;
    uint16_t logicalHeight;
    
    int xOffset = 0;
    int yOffset = 0;

    // Pin configuration (U8X8_PIN_NONE if not used)
    uint8_t clockPin = 255;  // I2C SCL / SPI SCK
    uint8_t dataPin = 255;   // I2C SDA / SPI MOSI
    uint8_t csPin = 255;     // SPI CS
    uint8_t dcPin = 255;     // SPI DC
    uint8_t resetPin = 255;  // Reset
    bool useHardwareI2C = true;

    /**
     * @brief Constructor for initializing display settings.
     * @param type Display type (ST7789, ST7735, NONE).
     * @param rot Rotation (0-3 for 0°, 90°, 180°, 270°; or degree values 90, 180, 270).
     * @param physW Physical display width (hardware).
     * @param physH Physical display height (hardware).
     * @param logW Logical rendering width (0 = same as physical).
     * @param logH Logical rendering height (0 = same as physical).
     * @param xOff X offset for display positioning.
     * @param yOff Y offset for display positioning.
     */
    DisplayConfig(
        DisplayType type,
        const int rot = pixelroot32::platforms::config::DisplayRotation,
        uint16_t physW = pixelroot32::platforms::config::PhysicalDisplayWidth,
        uint16_t physH = pixelroot32::platforms::config::PhysicalDisplayHeight,
        uint16_t logW = pixelroot32::platforms::config::LogicalWidth,
        uint16_t logH = pixelroot32::platforms::config::LogicalHeight,
        const int xOff = pixelroot32::platforms::config::XOffset,
        const int yOff = pixelroot32::platforms::config::YOffset,
        DrawSurface* customSurface = nullptr
    )
        : type(type), rotation(rot),
          physicalWidth(physW), physicalHeight(physH),
          logicalWidth(logW == 0 ? physW : logW),
          logicalHeight(logH == 0 ? physH : logH),
          xOffset(xOff), yOffset(yOff), drawSurface(customSurface)
    {   
        if (type != DisplayType::CUSTOM) {
            initDrawSurface();
        } else if (!drawSurface) {
            #ifdef PLATFORM_NATIVE
                assert(drawSurface && "DisplayType::CUSTOM requires a valid DrawSurface instance.");
            #endif
        }
    }

    /**
     * @brief Constructor with pin configuration (for OLED/U8G2).
     * @param type Display type.
     * @param rot Rotation.
     * @param clk Clock pin.
     * @param data Data pin.
     * @param cs CS pin.
     * @param dc DC pin.
     * @param rst Reset pin.
     * @param physW Physical width.
     * @param physH Physical height.
     * @param logW Logical width.
     * @param logH Logical height.
     * @param xOff X offset.
     * @param yOff Y offset.
     * @param hwI2C True to use hardware I2C.
     */
    DisplayConfig(
        DisplayType type,
        int rot,
        uint8_t clk,
        uint8_t data,
        uint8_t cs,
        uint8_t dc,
        uint8_t rst,
        uint16_t physW = 128,
        uint16_t physH = 64,
        uint16_t logW = 0,
        uint16_t logH = 0,
        int xOff = 0,
        int yOff = 0,
        bool hwI2C = true
    ) : type(type), rotation(rot),
        physicalWidth(physW), physicalHeight(physH),
        logicalWidth(logW == 0 ? physW : logW), 
        logicalHeight(logH == 0 ? physH : logH),
        xOffset(xOff), yOffset(yOff),
        clockPin(clk), dataPin(data), csPin(cs), dcPin(dc), resetPin(rst),
        useHardwareI2C(hwI2C)
    {
        if (type != DisplayType::CUSTOM) {
            initDrawSurface();
        }
    }

    /**
     * @brief Static factory to create a DisplayConfig with a custom DrawSurface.
     * @param surface Pointer to the custom DrawSurface implementation (ownership is transferred).
     * @param w Physical and logical width.
     * @param h Physical and logical height.
     * @param rot Rotation.
     */
    static DisplayConfig createCustom(DrawSurface* surface, uint16_t w, uint16_t h, int rot = 0) {
        return DisplayConfig(DisplayType::CUSTOM, rot, w, h, w, h, 0, 0, surface);
    }

    /**
     * @brief Copy constructor.
     * @param other The DisplayConfig to copy from.
     */
    DisplayConfig(const DisplayConfig& other)
        : type(other.type), rotation(other.rotation),
          physicalWidth(other.physicalWidth), physicalHeight(other.physicalHeight),
          logicalWidth(other.logicalWidth), logicalHeight(other.logicalHeight),
          xOffset(other.xOffset), yOffset(other.yOffset),
          clockPin(other.clockPin), dataPin(other.dataPin),
          csPin(other.csPin), dcPin(other.dcPin), resetPin(other.resetPin),
          useHardwareI2C(other.useHardwareI2C),
          drawSurface(nullptr) // Cannot copy the unique_ptr
    {}

    /**
     * @brief Move constructor.
     * @param other The DisplayConfig to move from.
     */
    DisplayConfig(DisplayConfig&& other) noexcept
        : type(other.type), rotation(other.rotation),
          physicalWidth(other.physicalWidth), physicalHeight(other.physicalHeight),
          logicalWidth(other.logicalWidth), logicalHeight(other.logicalHeight),
          xOffset(other.xOffset), yOffset(other.yOffset),
          clockPin(other.clockPin), dataPin(other.dataPin),
          csPin(other.csPin), dcPin(other.dcPin), resetPin(other.resetPin),
          useHardwareI2C(other.useHardwareI2C),
          drawSurface(std::move(other.drawSurface))
    {}

    /**
     * @brief Move assignment operator.
     * @param other The DisplayConfig to move from.
     * @return Reference to this DisplayConfig.
     */
    DisplayConfig& operator=(DisplayConfig&& other) noexcept {
        if (this != &other) {
            type = other.type;
            rotation = other.rotation;
            physicalWidth = other.physicalWidth;
            physicalHeight = other.physicalHeight;
            logicalWidth = other.logicalWidth;
            logicalHeight = other.logicalHeight;
            xOffset = other.xOffset;
            yOffset = other.yOffset;
            clockPin = other.clockPin;
            dataPin = other.dataPin;
            csPin = other.csPin;
            dcPin = other.dcPin;
            resetPin = other.resetPin;
            useHardwareI2C = other.useHardwareI2C;
            drawSurface = std::move(other.drawSurface);
        }
        return *this;
    }

    // =========================================================================
    // Resolution Helpers
    // =========================================================================
    
    /**
     * @brief Checks if scaling is needed (logical != physical).
     * @return true if the engine needs to scale output.
     */
    bool needsScaling() const { 
        return logicalWidth != physicalWidth || logicalHeight != physicalHeight; 
    }
    
    /**
     * @brief Gets horizontal scaling factor.
     * @return Scale factor (physical / logical).
     */
    float getScaleX() const { 
        return static_cast<float>(physicalWidth) / logicalWidth; 
    }
    
    /**
     * @brief Gets vertical scaling factor.
     * @return Scale factor (physical / logical).
     */
    float getScaleY() const { 
        return static_cast<float>(physicalHeight) / logicalHeight; 
    }

    // =========================================================================
    // Backward Compatibility Aliases (deprecated)
    // =========================================================================
    
    /**
     * @brief Deprecated, gets logical width.
     * @return The logical width.
     */
    uint16_t width() const { return logicalWidth; }
    
    /**
     * @brief Deprecated, gets logical height.
     * @return The logical height.
     */
    uint16_t height() const { return logicalHeight; }

    /**
     * @brief Gets the underlying DrawSurface implementation.
     * @return Reference to the DrawSurface.
     */
    DrawSurface& getDrawSurface() const { return *drawSurface; }

    /**
     * @brief Initializes the underlying draw surface.
     */
    void initDrawSurface();

    /**
     * @brief Transfers ownership of the DrawSurface to the caller.
     * @return A unique_ptr containing the DrawSurface.
     */
    std::unique_ptr<DrawSurface> releaseDrawSurface() {
        return std::move(drawSurface);
    }

private:
    std::unique_ptr<DrawSurface> drawSurface;
};

} // namespace pixelroot32::graphics

/**
 * @brief Helper macro to create a custom display configuration with ownership transfer.
 * 
 * Usage:
 * @code
 * Engine engine(PIXELROOT32_CUSTOM_DISPLAY(new MyDriver(), 240, 240));
 * @endcode
 */
#define PIXELROOT32_CUSTOM_DISPLAY(surface_ptr, width, height) \
    pixelroot32::graphics::DisplayConfig::createCustom(surface_ptr, width, height)
