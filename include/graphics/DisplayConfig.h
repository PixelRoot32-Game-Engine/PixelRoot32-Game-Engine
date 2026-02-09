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
    #include "../../src/platforms/mock/MockSPI.h"
    #include <drivers/native/SDL2_Drawer.h> 
    #include "DrawSurface.h"
#else
    #include <drivers/esp32/TFT_eSPI_Drawer.h>
    #include <SPI.h>
#endif
#include <stdexcept>

namespace pixelroot32::graphics {

enum DisplayType {
    ST7789, // 240x240 TFT
    ST7735, // 128x128 TFT
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
        const int rot = 0,
        uint16_t physW = 240,
        uint16_t physH = 240,
        uint16_t logW = 0,
        uint16_t logH = 0,
        const int xOff = 0,
        const int yOff = 0,
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
        } else if (drawSurface == nullptr) {
            #ifdef PLATFORM_NATIVE
                throw std::runtime_error("DisplayType::CUSTOM requires a valid DrawSurface instance.");
            #endif
        }
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
    
    /// @deprecated Use logicalWidth instead.
    uint16_t width() const { return logicalWidth; }
    
    /// @deprecated Use logicalHeight instead.
    uint16_t height() const { return logicalHeight; }

    DrawSurface& getDrawSurface() const { return *drawSurface; }

private:
    DrawSurface* drawSurface = nullptr;

    void initDrawSurface() {
        #ifdef TEST_MOCK_GRAPHICS
            class MockDrawer : public DrawSurface {
            public:
                void init() override {}
                void setRotation(uint16_t) override {}
                void clearBuffer() override {}
                void sendBuffer() override {}
                void drawText(const char*, int16_t, int16_t, uint16_t, uint8_t) override {}
                void drawTextCentered(const char*, int16_t, uint16_t, uint8_t) override {}
                void drawFilledCircle(int, int, int, uint16_t) override {}
                void drawCircle(int, int, int, uint16_t) override {}
                void drawRectangle(int, int, int, int, uint16_t) override {}
                void drawFilledRectangle(int, int, int, int, uint16_t) override {}
                void drawLine(int, int, int, int, uint16_t) override {}
                void drawBitmap(int, int, int, int, const uint8_t*, uint16_t) override {}
                void drawPixel(int, int, uint16_t) override {}
                void setContrast(uint8_t) override {}
                void setTextColor(uint16_t) override {}
                void setTextSize(uint8_t) override {}
                void setCursor(int16_t, int16_t) override {}
                uint16_t color565(uint8_t, uint8_t, uint8_t) override { return 0; }
                void setDisplaySize(int, int) override {}
                void setPhysicalSize(int, int) override {}
                void present() override {}
            };
            drawSurface = new MockDrawer();
        #elif defined(PLATFORM_NATIVE)
            drawSurface = new pixelroot32::drivers::native::SDL2_Drawer();
        #else
            switch (type)
            {
            case DisplayType::ST7789:
            case DisplayType::ST7735:
            default:
                drawSurface = new pixelroot32::drivers::esp32::TFT_eSPI_Drawer();
                break;
            }
        #endif
        
        if (drawSurface == nullptr) {
            #ifdef PLATFORM_NATIVE
                throw std::runtime_error("Failed to initialize Display Driver: No valid driver selected or supported for this platform.");
            #else
                // In ESP32, exceptions may not be enabled by default
                // while(1); 
            #endif
        }
    }
};

}
