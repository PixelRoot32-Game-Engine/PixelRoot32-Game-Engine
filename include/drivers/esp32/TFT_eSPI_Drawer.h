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
#ifndef PIXELROOT32_TFT_12BIT_COLOR
#define PIXELROOT32_TFT_12BIT_COLOR 0
#endif

#include "graphics/BaseDrawSurface.h"
#if PIXELROOT32_TFT_12BIT_COLOR
#include "graphics/Rgb444.h"
#endif
// TFT_eSPI-specific includes
#include <TFT_eSPI.h>
#include <stdint.h>


namespace  pixelroot32::drivers::esp32 {
    
/**
 * @class TFT_eSPI_Drawer
 * @brief Concrete implementation of DrawSurface for ESP32 using the TFT_eSPI library.
 *
 * Inherits from BaseDrawSurface.
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

    /**
     * @brief Blocks until the DMA transfer deferred by sendBufferScaled() completes.
     *
     * sendBufferScaled() leaves the last block of the frame in flight so its SPI
     * time overlaps the next frame's update/draw work. Every other user of the SPI
     * bus - and anything that frees or reallocates lineBuffer[] - MUST call this
     * first, otherwise it either corrupts the transaction or triggers a
     * use-after-free on the buffer DMA is still reading.
     *
     * Safe to call when nothing is pending (no-op).
     */
    void waitForPendingDMA();

    /**
     * @brief Settles the deferred transfer on a frame the engine chose not to
     *        present. Implements the DrawSurface hook in terms of
     *        waitForPendingDMA().
     *
     * This driver's present() deliberately returns with the frame's last block
     * still in flight and the SPI write transaction still OPEN, so that block's
     * bus time overlaps the next frame's update/draw work. The design assumes
     * another present() follows to close it, which is true only while every
     * frame is drawn.
     *
     * A scene that reports `shouldRedrawFramebuffer() == false` breaks that
     * assumption: Engine skips both draw() and present(), so nothing closes the
     * transaction and the bus stays claimed for as long as the scene holds
     * still. Engine therefore calls this on exactly the frames it skips. The
     * pixels already on the panel are unaffected - the transfer completes
     * either way; what this reclaims is the bus and the open transaction.
     *
     * @note Do NOT move this into processEvents() to "always be safe".
     *       Engine runs processEvents() BEFORE draw(), so flushing there would
     *       wait out the DMA before the CPU does the work meant to overlap it,
     *       destroying the deferral this exists to protect.
     *
     * Safe to call when nothing is pending (no-op), so drivers that transfer
     * synchronously simply inherit the base no-op instead of overriding.
     *
     * @see waitForPendingDMA() for the shared-bus contract this reuses.
     */
    void flushPendingTransfers() override { waitForPendingDMA(); }

private:
    TFT_eSPI tft;   ///< The underlying TFT_eSPI driver instance.
    TFT_eSprite spr; ///< The sprite used as a framebuffer.

    // Scaling support (batch sizes: PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK in PlatformDefaults.h)
    int activeLinesPerBlock = PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK; ///< Set during init from buildScaleLUTs()
    uint16_t* lineBuffer[2] = {nullptr, nullptr}; ///< Double buffer for DMA line transfer
    uint8_t currentBuffer = 0;                    ///< Current buffer index (0 or 1); alternates ACROSS frames, never reset
    bool dmaPending = false;                      ///< True while the last block of a frame is still in flight (transaction still open)
    uint16_t* xLUT = nullptr;        ///< Lookup table for X scaling (physical -> logical)
    uint16_t* yLUT = nullptr;        ///< Lookup table for Y scaling (physical -> logical)
    uint16_t* paletteLUT = nullptr;  ///< Pre-calculated 8bpp palette LUT (byte-swapped RGB565, or 0x0RGB in 12-bit mode)

#if PIXELROOT32_TFT_12BIT_COLOR
    /// True once the panel has been switched to the 12-bit (RGB444) pixel format.
    ///
    /// Decided in buildScaleLUTs() and left false when the physical width makes
    /// the packed stream unsafe (see the guard there), in which case the driver
    /// keeps the byte-swapped RGB565 path unchanged.
    bool use12BitColor = false;

    /// 3-byte RGB444 stream for a horizontally duplicated pixel, one entry per
    /// palette index. Used by the 2x fast path, which only ever emits pairs of
    /// identical colours, so the triple is a pure function of the source index.
    uint8_t (*pairLUT)[3] = nullptr;
#endif

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

#if PIXELROOT32_TFT_12BIT_COLOR
    /**
     * @brief Converts one block of physical lines into the packed RGB444 stream.
     *
     * Mirrors the three RGB565 conversion paths (1:1, 2x, generic scale) but
     * emits three bytes per pixel pair instead of two bytes per pixel.
     *
     * @param spriteBase Base of the 8bpp sprite framebuffer.
     * @param startY     First physical line of the block.
     * @param endY       One past the last physical line of the block.
     * @param is2x       True when the frame is an exact 2x integer upscale.
     * @param dst        Destination line buffer, viewed as raw bytes.
     */
    void convertBlockRgb444(const uint8_t* spriteBase, int startY, int endY, bool is2x, uint8_t* dst);

    /**
     * @brief Scales a single line from 8bpp logical to packed RGB444 physical.
     */
    void scaleLine444(const uint8_t* spriteBase, int srcY, uint8_t* dst);

    /**
     * @brief Bytes one physical line occupies in the packed RGB444 stream.
     *
     * Always even, because 12-bit mode is only enabled for physical widths that
     * are a multiple of 4 (see buildScaleLUTs).
     */
    int bytesPerLine444() const { return (physicalWidth * 3) / 2; }
#endif
};

} // namespace pixelroot32::drivers::esp32

#endif // PIXELROOT32_USE_TFT_ESPI_DRIVER

#endif // TFT_eSPI_DRAWER_H
