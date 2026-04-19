/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#ifndef TFT_eSPI_DRAWER_H
#define TFT_eSPI_DRAWER_H

#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)

#ifndef PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK
#define PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK 60
#endif
#ifndef PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK
#define PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK 30
#endif

#include "graphics/BaseDrawSurface.h"
#include "graphics/PartialUpdateController.h"
#include "graphics/ColorDepthManager.h"
// TFT_eSPI-specific includes
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
     * @brief Direct tile write to sprite buffer (optimized for tilemap rendering).
     * 
     * Writes tile data directly to the 8bpp sprite buffer without function call overhead.
     * This is significantly faster than calling drawPixel() for each pixel.
     * 
     * @param x Tile X position in sprite coordinates
     * @param y Tile Y position in sprite coordinates
     * @param width Tile width in pixels
     * @param height Tile height in pixels
     * @param data Pointer to 8bpp tile data (one byte per pixel, index into palette)
     */
    void drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data) override;

    /**
     * @brief Get pointer to sprite buffer for direct manipulation.
     * 
     * @return Pointer to 8bpp sprite buffer, or nullptr if sprite not created
     */
    uint8_t* getSpriteBuffer() override;

    /**
     * @brief Processes system events. Always true for embedded.
     */
    bool processEvents() override;

private:
    TFT_eSPI tft;   ///< The underlying TFT_eSPI driver instance.
    TFT_eSprite spr; ///< The sprite used as a framebuffer.

    // Scaling support (batch sizes: PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK in PlatformDefaults.h)
    int activeLinesPerBlock = PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK; ///< Set during init from buildScaleLUTs()
    uint16_t* lineBuffer[2] = {nullptr, nullptr}; ///< Double buffer for DMA line transfer
    uint8_t currentBuffer = 0;                    ///< Current buffer index (0 or 1)
    uint16_t* xLUT = nullptr;        ///< Lookup table for X scaling (physical -> logical)
    uint16_t* yLUT = nullptr;        ///< Lookup table for Y scaling (physical -> logical)
    uint16_t* paletteLUT = nullptr;  ///< Pre-calculated 8bpp to 16bpp palette LUT
    bool dmaAvailable_ = true;      ///< DMA availability flag (set false on allocation failure)
    
    // ============================================================================
    // Display Bottleneck Optimization Managers
    // ============================================================================
    pixelroot32::graphics::PartialUpdateController partialController_;  ///< Partial update controller
    pixelroot32::graphics::ColorDepthManager colorDepthManager_;  ///< Color depth manager
    
    /**
     * @brief Send buffer using partial updates (only dirty regions).
     * @param regions Vector of dirty regions to send
     */
    void sendBufferPartial(const std::vector<pixelroot32::graphics::DirtyRect>& regions);
    
    /**
     * @brief Send a single region to the display.
     * @param x Region X position in sprite pixels
     * @param y Region Y position in sprite pixels
     * @param w Region width in pixels
     * @param h Region height in pixels
     */
    void sendRegion(int16_t x, int16_t y, uint16_t w, uint16_t h);
    
    // ============================================================================
    // Partial Update API - Override base class implementations
    // ============================================================================
    
    /**
     * @brief Mark a region as dirty (delegates to partial controller).
     */
    void markDirty(int x, int y, int width, int height) override;
    
    /**
     * @brief Clear dirty flags for next frame.
     */
    void clearDirtyFlags() override;
    
    /**
     * @brief Check if partial updates are beneficial.
     */
    bool hasDirtyRegions() const override;
    
    /**
     * @brief Enable or disable partial updates.
     */
    void setPartialUpdateEnabled(bool enabled) override;
    
    /**
     * @brief Check if partial updates are enabled.
     */
    bool isPartialUpdateEnabled() const override;
    
    /**
     * @brief Set color depth for display output.
     * @param depth Color depth (24, 16, 8, or 4 bits per pixel)
     */
    void setColorDepth(int depth);
    
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
    void scaleLine(const uint8_t* spriteBase, int srcY, uint16_t* dst);
};

} // namespace pixelroot32::drivers::esp32

#endif // PIXELROOT32_USE_TFT_ESPI_DRIVER

#endif // TFT_eSPI_DRAWER_H
