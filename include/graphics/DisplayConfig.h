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
#else
    #include <SPI.h>
#endif

namespace pixelroot32::graphics {

/**
 * @brief Configuration settings for initializing U8g2-compatible displays.
 * 
 * Use this structure to define the hardware pins, display dimensions, rotation,
 * and communication type (I2C or SPI).
 */
struct DisplayConfig {
    DrawSurface* drawSurface = nullptr; ///< Pointer to the draw surface
    int rotation = 0;                   ///< Display rotation
    uint16_t width;                     ///< Display width in pixels
    uint16_t height;                    ///< Display height in pixels
    int xOffset = 0;                    ///< X offset for display rendering
    int yOffset = 0;                    ///< Y offset for display rendering

    /**
     * @brief Constructor to initialize the display configuration.
     * 
     * @param driverType Type of the driver (ST7789_DRIVER or SDL_DRIVER).
     * @param rot Display rotation.
     * @param w Display width in pixels.
     * @param h Display height in pixels.
     */
    DisplayConfig(
        DrawSurface* drawSurface,
        const int rot = 0,
        uint16_t w = 240,
        uint16_t h = 240
    )
        : drawSurface(drawSurface), rotation(rot), width(w), height(h)
    {
        const int DEFAULT_DISPLAY_WIDTH = 240;
        const int DEFAULT_DISPLAY_HEIGHT = 240;

        xOffset = (DEFAULT_DISPLAY_WIDTH - width) / 2;
        yOffset = (DEFAULT_DISPLAY_HEIGHT - height) / 2;            
    }
};

}
