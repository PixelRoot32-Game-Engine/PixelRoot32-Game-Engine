/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#ifndef U8G2_DRAEWER_H
#define U8G2_DRAEWER_H

#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_USE_U8G2_DRIVER)

#include "graphics/BaseDrawSurface.h"
#include <U8g2lib.h>

namespace pixelroot32::drivers::esp32 {

/**
 * @class U8G2_Drawer
 * @brief Implementation of DrawSurface using the U8G2 library for monochromatic OLED displays.
 */
class U8G2_Drawer : public pixelroot32::graphics::BaseDrawSurface {
public:
    /**
     * @brief Construct a new U8G2_Drawer with a custom U8G2 instance.
     * @param u8g2 Pointer to a pre-configured U8G2 instance.
     * @param ownsInstance If true, the drawer will delete the u8g2 instance in its destructor.
     */
    U8G2_Drawer(U8G2* u8g2, bool ownsInstance = true);
    
    ~U8G2_Drawer() override;

    // Core DrawSurface implementation
    bool init() override;
    void setRotation(uint16_t rotation) override;
    void drawPixel(int x, int y, uint16_t color) override;
    void clearBuffer() override;
    void sendBuffer() override;
    
    // Optimized overrides for U8G2
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override;
    void drawRectangle(int x, int y, int w, int h, uint16_t color) override;
    void drawFilledRectangle(int x, int y, int w, int h, uint16_t color) override;
    void drawCircle(int x0, int y0, int r, uint16_t color) override;
    void drawFilledCircle(int x0, int y0, int r, uint16_t color) override;
    
    /**
     * @brief Support for U8G2 native XBM bitmaps.
     * Note: This assumes the bitmap is in XBM format if color is 1, 
     * otherwise it uses the base implementation.
     */
    void drawBitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint16_t color) override;

    // Size management overrides
    void setDisplaySize(int w, int h) override;
    void setPhysicalSize(int w, int h) override;

    // Getters
    U8G2* getU8g2() const { return _u8g2; }

private:
    U8G2* _u8g2;
    bool _ownsInstance;
    uint8_t* _internalBuffer = nullptr; ///< Internal buffer for logical resolution (1 bit per pixel)
    uint16_t* _xLUT = nullptr;          ///< Lookup table for X scaling (physical -> logical)
    uint16_t* _yLUT = nullptr;          ///< Lookup table for Y scaling (physical -> logical)

    /**
     * @brief Checks if scaling is needed.
     * @return true if logical != physical resolution.
     */
    bool needsScaling() const {
        return logicalWidth != physicalWidth || logicalHeight != physicalHeight;
    }

    /**
     * @brief Builds the X and Y scaling lookup tables.
     */
    void buildScaleLUTs();

    /**
     * @brief Frees scaling-related memory.
     */
    void freeScalingBuffers();

    /**
     * @brief Sends the buffer using software scaling.
     */
    void sendBufferScaled();

    /**
     * @brief Internal helper to convert RGB565 to 1-bit monochromatic.
     * @return 1 for "on", 0 for "off".
     */
    inline uint8_t rgb565To1Bit(uint16_t color) {
        // Luminance-based threshold (simple version: if any bit is on)
        return (color > 0) ? 1 : 0;
    }
};

} // namespace pixelroot32::drivers::esp32

#endif // PIXELROOT32_USE_U8G2_DRIVER

#endif // U8G2_DRAEWER_H
