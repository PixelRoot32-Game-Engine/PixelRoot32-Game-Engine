
#pragma once
#ifndef TFT_eSPI_DRAEWER_H
#define TFT_eSPI_DRAEWER_H

#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)

#include "graphics/DrawSurface.h"
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
class TFT_eSPI_Drawer : public pixelroot32::graphics::DrawSurface {
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

    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) override;
    void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) override;
    void drawFilledCircle(int x, int y, int radius, uint16_t color) override;
    void drawCircle(int x, int y, int radius, uint16_t color) override;
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override;
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) override;
    void drawPixel(int x, int y, uint16_t color) override;

    void setTextColor(uint16_t color) override;
    void setTextSize(uint8_t size) override;
    void setCursor(int16_t x, int16_t y) override;

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override;

    void setDisplaySize(int w, int h) override;

    /**
     * @brief Sets contrast. No-op for ST7789 usually.
     */
    void setContrast(uint8_t value) override {
        // ST7789 does not support real contrast control via this API -> noop
    }

    /**
     * @brief Processes system events. Always true for embedded.
     */
    bool processEvents() override;

    /**
     * @brief Present buffer. Calls sendBuffer().
     */
    void present() override;

    /**
     * @brief Sets the physical display size for scaling operations.
     * @param w Physical width.
     * @param h Physical height.
     */
    void setPhysicalSize(int w, int h) override;

private:
    TFT_eSPI tft;   ///< The underlying TFT_eSPI driver instance.
    TFT_eSprite spr; ///< The sprite used as a framebuffer.
    int16_t cursorX, cursorY;
    uint16_t textColor;
    uint8_t textSize;
    uint8_t rotation;

    // Resolution dimensions
    int logicalWidth = 240;   ///< Logical resolution (framebuffer size)
    int logicalHeight = 240;
    int physicalWidth = 240;  ///< Physical resolution (display hardware)
    int physicalHeight = 240;
    
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
     * @brief Sends the framebuffer scaled to physical resolution.
     */
    void IRAM_ATTR sendBufferScaled();
    
    /**
     * @brief Scales a single logical line to physical width.
     * @param srcY Source Y coordinate in logical space.
     * @param dst Destination buffer (physicalWidth pixels).
     */
    void IRAM_ATTR scaleLine(int srcY, uint16_t* dst);
};

} // namespace pixelroot32::drivers::esp32

#endif // ESP32

#endif // TFT_eSPI_DRAEWER_H