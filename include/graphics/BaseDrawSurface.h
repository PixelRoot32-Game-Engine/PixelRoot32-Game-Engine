/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "DrawSurface.h"
#include <cmath>

namespace pixelroot32::graphics {

/**
 * @class BaseDrawSurface
 * @brief Optional base class for DrawSurface implementations that provides default primitive rendering.
 * 
 * Users can inherit from this class to avoid implementing every single primitive.
 * At minimum, a subclass should implement:
 * - init()
 * - drawPixel()
 * - sendBuffer()
 * - clearBuffer()
 */
class BaseDrawSurface : public DrawSurface {
public:
    virtual ~BaseDrawSurface() = default;

    // State management defaults
    void setTextColor(uint16_t color) override { textColor = color; }
    void setTextSize(uint8_t size) override { textSize = size; }
    void setCursor(int16_t x, int16_t y) override { cursorX = x; cursorY = y; }
    void setContrast(uint8_t level) override { contrast = level; }
    void setRotation(uint16_t rot) override { rotation = rot; }
    
    // Size management defaults
    void setDisplaySize(int w, int h) override { logicalWidth = w; logicalHeight = h; }
    void setPhysicalSize(int w, int h) override { physicalWidth = w; physicalHeight = h; }
    void setOffset(int x, int y) override { xOffset = x; yOffset = y; }
    void present() override { sendBuffer(); }

    // Color conversion default (RGB888 to RGB565)
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    // Default primitive implementations using drawPixel (slow but functional)
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override {
        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy, e2;
        while (true) {
            drawPixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x1 += sx; }
            if (e2 <= dx) { err += dx; y1 += sy; }
        }
    }

    void drawRectangle(int x, int y, int w, int h, uint16_t color) override {
        for (int i = 0; i < w; i++) {
            drawPixel(x + i, y, color);
            drawPixel(x + i, y + h - 1, color);
        }
        for (int i = 0; i < h; i++) {
            drawPixel(x, y + i, color);
            drawPixel(x + w - 1, y + i, color);
        }
    }

    void drawFilledRectangle(int x, int y, int w, int h, uint16_t color) override {
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                drawPixel(x + i, y + j, color);
            }
        }
    }

    void drawCircle(int x0, int y0, int r, uint16_t color) override {
        int x = -r, y = 0, err = 2 - 2 * r;
        do {
            drawPixel(x0 - x, y0 + y, color);
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 + x, color);
            r = err;
            if (r <= y) err += ++y * 2 + 1;
            if (r > x || err > y) err += ++x * 2 + 1;
        } while (x < 0);
    }

    void drawFilledCircle(int x0, int y0, int r, uint16_t color) override {
        int x = -r, y = 0, err = 2 - 2 * r;
        do {
            for (int i = x0 + x; i <= x0 - x; i++) {
                drawPixel(i, y0 + y, color);
                drawPixel(i, y0 - y, color);
            }
            r = err;
            if (r <= y) err += ++y * 2 + 1;
            if (r > x || err > y) err += ++x * 2 + 1;
        } while (x < 0);
    }

    void drawBitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint16_t color) override {
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                if (bitmap[j * w + i]) {
                    drawPixel(x + i, y + j, color);
                }
            }
        }
    }

    // ============================================================================
    // Partial Update API (Default implementations - override for optimization)
    // ============================================================================

    /**
     * @brief Mark a region as dirty for partial screen updates.
     * 
     * Default implementation does nothing (base class can work without optimization).
     * Override in drivers that support partial updates (e.g., TFT_eSPI_Drawer).
     * 
     * @param x X coordinate in sprite pixels
     * @param y Y coordinate in sprite pixels
     * @param width Width in pixels
     * @param height Height in pixels
     */
    virtual void markDirty(int x, int y, int width, int height) {
        (void)x; (void)y; (void)width; (void)height;
        // Default: no-op, base class doesn't track dirty regions
    }

    /**
     * @brief Clear all dirty tracking flags for next frame.
     * 
     * Default implementation does nothing.
     * Override in drivers that support partial updates.
     */
    virtual void clearDirtyFlags() {
        // Default: no-op
    }

    /**
     * @brief Check if partial updates are enabled and beneficial.
     * 
     * @return true if should use partial updates
     */
    virtual bool hasDirtyRegions() const {
        return false;
    }

    /**
     * @brief Set partial update mode.
     * @param enabled true to enable partial updates
     */
    virtual void setPartialUpdateEnabled(bool enabled) {
        (void)enabled;
        // Default: no-op
    }

    /**
     * @brief Check if partial updates are enabled.
     * @return true if partial updates are enabled
     */
    virtual bool isPartialUpdateEnabled() const {
        return false;
    }

    /**
     * @brief Called at the beginning of each frame to prepare for partial update tracking.
     * Default implementation does nothing.
     */
    virtual void beginFrame() override {
        // Default: no-op
    }

    /**
     * @brief Called at the end of each frame to finalize dirty region tracking.
     * Default implementation does nothing.
     */
    virtual void endFrame() override {
        // Default: no-op
    }

    /**
     * @brief Set the color depth for display output.
     * Default implementation does nothing.
     */
    virtual void setColorDepth(int depth) override {
        (void)depth;
        // Default: no-op
    }

    // ============================================================================
    // Auto-Mark Dirty API
    // ============================================================================

    /**
     * @brief Enable or disable automatic dirty region marking.
     *
     * When enabled (default), the surface automatically calls markDirty() after
     * drawing operations. This provides zero-config partial updates for most games.
     *
     * When disabled, games must manually call markDirty() for regions they want
     * to update. This is useful for custom rendering pipelines that need precise
     * control over which regions are updated.
     *
     * @param enabled true to enable auto-marking (default), false for manual marking
     */
    virtual void setAutoMarkDirty(bool enabled) {
        autoMarkDirty_ = enabled;
    }

    /**
     * @brief Check if automatic dirty marking is enabled.
     * @return true if auto-marking is enabled
     */
    virtual bool isAutoMarkDirty() const {
        return autoMarkDirty_;
    }

    // ============================================================================
    // Debug Dirty Regions API
    // ============================================================================

    /**
     * @brief Enable or disable debug overlay showing sent dirty regions.
     *
     * When enabled, a 2px red border is drawn around each dirty region sent to
     * the display. This is useful for debugging and tuning the partial update
     * system. Can be toggled at runtime without recompilation.
     *
     * Note: Requires PIXELROOT32_DEBUG_DIRTY_REGIONS compile flag to include
     * the debug drawing code in the build.
     *
     * @param enabled true to enable debug overlay
     */
    virtual void setDebugDirtyRegions(bool enabled) {
        debugDirtyRegions_ = enabled;
    }

    /**
     * @brief Check if debug dirty regions overlay is enabled.
     * @return true if debug overlay is enabled
     */
    virtual bool isDebugDirtyRegions() const {
        return debugDirtyRegions_;
    }

protected:
    uint16_t textColor = 0xFFFF;
    uint8_t textSize = 1;
    int16_t cursorX = 0, cursorY = 0;
    uint8_t contrast = 255;
    uint16_t rotation = 0;
    int logicalWidth = 240, logicalHeight = 240;
    int physicalWidth = 240, physicalHeight = 240;
    int xOffset = 0, yOffset = 0;

    // Auto-marking for dirty regions (default: enabled)
    // When true, markDirty() is called automatically after draw operations
    bool autoMarkDirty_ = true;

    // Debug overlay for dirty regions (default: disabled)
    bool debugDirtyRegions_ = false;
};

} // namespace pixelroot32::graphics
