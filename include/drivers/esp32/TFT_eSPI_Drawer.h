
#pragma once
#ifndef TFT_eSPI_DRAEWER_H
#define TFT_eSPI_DRAEWER_H

#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)

#include "graphics/BaseDrawSurface.h"
// TFT_esPI pecific includes would go here
#include <TFT_eSPI.h>
#include <stdint.h>


namespace  pixelroot32::drivers::esp32 {
    
/**
 * @class TFT_eSPI_Drawer
 * @brief Concrete implementation of DrawSurface for ESP32 using the TFT_eSPI library.
 *
 * This class handles low-level interaction with the display hardware via SPI.
 * It uses a sprite (framebuffer) to minimize flickering and tearing.
 */
class TFT_eSPI_Drawer : public pixelroot32::graphics::BaseDrawSurface {
public:
    TFT_eSPI_Drawer();
    virtual ~TFT_eSPI_Drawer();

    /**
     * @brief Initializes the TFT_eSPI library and the sprite buffer.
     * Sets up the SPI communication and allocates memory for the framebuffer.
     */
    void init() override;

    /**
     * @brief Sets the screen rotation.
     * @param rotation 0-3 corresponding to 0, 90, 180, 270 degrees.
     */
    void setRotation(uint16_t rotation) override;

    /**
     * @brief Fills the sprite buffer with black color.
     */
    void clearBuffer() override;

    /**
     * @brief Pushes the sprite buffer to the physical display.
     * This is the "flip" operation in double buffering.
     */
    void sendBuffer() override;

    void drawFilledCircle(int x, int y, int radius, uint16_t color) override;
    void drawCircle(int x, int y, int radius, uint16_t color) override;
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override;
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) override;
    void drawPixel(int x, int y, uint16_t color) override;

    /**
     * @brief Processes system events. Always true for embedded.
     */
    bool processEvents() override;

private:
    TFT_eSPI tft;   ///< The underlying TFT_eSPI driver instance.
    TFT_eSprite spr; ///< The sprite used as a framebuffer.

    // Scaling support
    uint16_t* lineBuffer[2] = {nullptr, nullptr}; ///< Double buffer for DMA line transfer
    uint8_t currentBuffer = 0;                    ///< Current buffer index (0 or 1)
    uint16_t* xLUT = nullptr;        ///< Lookup table for X scaling (physical -> logical)
    uint16_t* yLUT = nullptr;        ///< Lookup table for Y scaling (physical -> logical)
    uint16_t* paletteLUT = nullptr;  ///< Pre-calculated 8bpp to 16bpp palette LUT
    
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
     * @brief Sends the buffer using hardware DMA and software scaling.
     */
    void sendBufferScaled();

    /**
     * @brief Scales a single line from 8bpp logical to 16bpp physical.
     */
    void scaleLine(int srcY, uint16_t* dst);
};

} // namespace pixelroot32::drivers::esp32

#endif // ESP32

#endif // TFT_eSPI_DRAEWER_H