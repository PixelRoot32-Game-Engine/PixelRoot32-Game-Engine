/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 Gabriel Perez
 *
 * This file remains licensed under the MIT License.
 */
#pragma once

#ifdef PLATFORM_NATIVE
    #include "../../src/platforms/mock/MockSPI.h"
    #include <drivers/native/SDL2_Drawer.h> 
#else
    #include <drivers/esp32/TFT_eSPI_Drawer.h>
    #include <SPI.h>
#endif
#include <stdexcept>

namespace pixelroot32::graphics {

enum DisplayType {
    ST7789, // 240x240 TFT
    ST7735, // 128x128 TFT
    NONE    // for SDL2 native no driver.
}; 

/**
 * @brief Configuration settings for initializing U8g2-compatible displays.
 * 
 * Use this structure to define the hardware pins, display dimensions, rotation,
 * and communication type (I2C or SPI).
 */
struct DisplayConfig {
public:
    DisplayType type;
    int rotation = 0;
    uint16_t width;
    uint16_t height;
    int xOffset = 0;
    int yOffset = 0;

    DisplayConfig(
        DisplayType type,
        const int rot = 0,
        uint16_t w = 240,
        uint16_t h = 240,
        const int xOffset = 0,
        const int yOffset = 0
    )
        : type(type), rotation(rot), width(w), height(h), 
          xOffset(xOffset), yOffset(yOffset), drawSurface(nullptr)
    {   
        #ifdef PLATFORM_NATIVE
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

    DrawSurface& getDrawSurface() const { return *drawSurface; }

private:
    DrawSurface* drawSurface = nullptr;
};

}