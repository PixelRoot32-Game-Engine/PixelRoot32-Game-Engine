/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>

namespace pixelroot32::graphics {

/**
 * @class DrawSurface
 * @brief Abstract interface for platform-specific drawing operations.
 *
 * This class defines the contract for any graphics driver (e.g., TFT_eSPI for ESP32,
 * SDL2 for Windows). It implements the Bridge pattern, allowing the Renderer to
 * remain platform-agnostic.
 */
class DrawSurface {
public:

    virtual ~DrawSurface() = default;

    /**
     * @brief Initializes the hardware or window.
     */
    virtual void init() = 0;

    /**
     * @brief Sets the display rotation.
     * @param rotation Rotation value. Can be index (0-3) or degrees (0, 90, 180, 270).
     */
    virtual void setRotation(uint16_t rotation) = 0;

    /**
     * @brief Clears the frame buffer (fills with black or background color).
     */
    virtual void clearBuffer() = 0;

    /**
     * @brief Sends the frame buffer to the physical display.
     */
    virtual void sendBuffer() = 0;

    // Drawing Primitives
    virtual void drawFilledCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) = 0;
    virtual void drawPixel(int x, int y, uint16_t color) = 0;

    /**
     * @brief Direct tile write to sprite buffer (optimized for tilemap rendering).
     * 
     * Default implementation returns without doing anything.
     * Override in drivers that support direct buffer access (e.g., TFT_eSPI).
     * 
     * This default implementation also automatically marks the region as dirty
     * for partial screen updates when the subclass supports it.
     * 
     * @param x Tile X position in sprite coordinates
     * @param y Tile Y position in sprite coordinates
     * @param width Tile width in pixels
     * @param height Tile height in pixels
     * @param data Pointer to 8bpp tile data (one byte per pixel, index into palette)
     */
    virtual void drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data) {
        // Default: auto-mark the region as dirty for partial updates
        // Subclasses can override for more optimized implementations
        markDirty(static_cast<int>(x), static_cast<int>(y), 
                  static_cast<int>(width), static_cast<int>(height));
    }

    /**
     * @brief Get pointer to sprite buffer for direct manipulation.
     * 
     * Default implementation returns nullptr.
     * Override in drivers that support direct buffer access.
     * 
     * @return Pointer to 8bpp sprite buffer, or nullptr if not supported
     */
    virtual uint8_t* getSpriteBuffer() {
        return nullptr;
    }

    /**
     * @brief Set the color depth for display output.
     * @param depth Contrast level (0-255).
     */
    virtual void setContrast(uint8_t level) = 0;

    // ============================================================================
    // Partial Update Benchmark API
    // ============================================================================

    /**
     * @brief Get number of regions sent in last frame.
     * @return Region count
     */
    virtual int getLastRegionCount() const {
        return 0;
    }

    /**
     * @brief Get total pixels sent in last frame.
     * @return Total sent pixels
     */
    virtual int getLastTotalSentPixels() const {
        return 0;
    }

    /**
     * @brief Get number of dirty pixels in last frame.
     * @return Dirty pixel count
     */
    virtual int getDirtyPixelCount() const {
        return 0;
    }

    /**
     * @brief Get last frame width.
     * @return Frame width
     */
    virtual int getLastFrameWidth() const {
        return 0;
    }

    /**
     * @brief Get last frame height.
     * @return Frame height
     */
    virtual int getLastFrameHeight() const {
        return 0;
    }

    // Text State Management
    virtual void setTextColor(uint16_t color) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    // virtual int16_t textWidth(const char* text) = 0;

    /**
     * @brief Converts RGB888 color to RGB565 format.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @return 16-bit color value.
     */
    virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b) = 0;

    /**
     * @brief Sets the logical display size (rendering resolution).
     * @param w Width of the logical framebuffer.
     * @param h Height of the logical framebuffer.
     */
    virtual void setDisplaySize(int w, int h) = 0;

    /**
     * @brief Sets the physical display size (hardware resolution).
     * 
     * Used when the logical rendering resolution differs from the
     * physical display resolution. The driver will scale output
     * from logical to physical resolution.
     * 
     * @param w Physical display width.
     * @param h Physical display height.
     */
    virtual void setPhysicalSize(int w, int h) {
        // Default implementation: assume no scaling (physical = logical)
        // Override in drivers that support resolution scaling
        (void)w; (void)h;
    }

    /**
     * @brief Sets the display offset (positioning of the active area).
     * @param x X offset.
     * @param y Y offset.
     */
    virtual void setOffset(int x, int y) {
        (void)x; (void)y;
    }

    /**
     * @brief Processes platform events (e.g., SDL window events).
     * @return false if the application should quit, true otherwise.
     */
    virtual bool processEvents() { return true; }

    /**
     * @brief Swaps buffers (for double-buffered systems like SDL).
     */
    virtual void present() = 0;

    // ============================================================================
    // Partial Update API - for display bottleneck optimization
    // ============================================================================

    /**
     * @brief Mark a region as dirty for partial screen updates.
     * 
     * Usado por DirtyRectTracker para rastrear qué regiones han cambiado.
     * El sistema de partial updates solo envía las regiones marcadas a la pantalla,
     * reduciendo el tiempo de transferencia en displays lentos (SPI).
     * 
     * Implementación por defecto no hace nada.
     * Sobreescribir en drivers que soporten partial updates (ej: TFT_eSPI).
     * 
     * @param x Coordenada X en píxeles del sprite
     * @param y Coordenada Y en píxeles del sprite
     * @param width Ancho en píxeles
     * @param height Alto en píxeles
     * @return void
     * 
     * @note El sistema automáticamente combina regiones adyacentes para optimizar.
     * @note Llamar desde drawTileDirect() o después de cualquier escritura al buffer.
     * 
     * @par Ejemplo de uso:
     * @code
     * // Después de dibujar un tile en el buffer directo
     * renderer.beginFrame();
     * uint8_t* buffer = renderer.getSpriteBuffer();
     * // ... escribir al buffer ...
     * renderer.markDirty(tileX, tileY, 16, 16);  // Marcar región como modificada
     * renderer.endFrame();
     * @endcode
     */
    virtual void markDirty(int x, int y, int width, int height) {
        (void)x; (void)y; (void)width; (void)height;
    }

    /**
     * @brief Limpiar todas las flags de dirty tracking para el siguiente frame.
     * 
     * Se llama automáticamente al inicio de cada frame (beginFrame).
     * No es necesario llamar manualmente a menos que se requiera
     * un control más fino del tracking.
     * 
     * @note Relacionado con beginFrame() - se llama después de clearDirtyFlags.
     * @note El sistema automáticamente limpia las flags al comenzar cada frame.
     * 
     * @par Cuándo llamar manualmente:
     * @code
     * // Forzar limpieza de dirty regions antes de lo normal
     * renderer.clearDirtyFlags();  // Reiniciar tracking manualmente
     * @endcode
     */
    virtual void clearDirtyFlags() {}

    /**
     * @brief Check if there are dirty regions to update.
     * 
     * @return true if dirty regions exist and should use partial update
     */
    virtual bool hasDirtyRegions() const {
        return false;
    }

    /**
     * @brief Enable or disable partial updates.
     * @param enabled true to enable partial updates
     */
    virtual void setPartialUpdateEnabled(bool enabled) {
        (void)enabled;
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
     * 
     * Default implementation does nothing.
     * Override in drivers that support partial updates.
     */
    virtual void beginFrame() {
        // Default: no-op
    }

    /**
     * @brief Called at the end of each frame to finalize dirty region tracking.
     * 
     * This is called after all game drawing is complete, before sendBuffer.
     * It allows controllers to merge dirty regions and prepare for transfer.
     * 
     * Default implementation does nothing.
     */
    virtual void endFrame() {
        // Default: no-op
    }

    /**
     * @brief Establecer la profundidad de color para salida del display.
     * 
     * Controla el formato de color usado al enviar a la pantalla física.
     * Soporta: 24-bit (RGB888), 16-bit (RGB565), 8-bit indexed, 4-bit indexed.
     * 
     * Implementación por defecto no hace nada.
     * Sobreescribir en drivers que soporten cambio de profundidad.
     * 
     * @param depth Profundidad en bits: 24, 16, 8, o 4
     * @return void
     * 
     * @note Valores soportados:
     * - 24: RGB888 (24-bit true color) - mayor calidad, mayor bandwidth
     * - 16: RGB565 (default) - balance calidad/rendimiento
     * - 8: indexed (256 colores) - menor memoria
     * - 4: indexed (16 colores) - mínimo footprint
     * 
     * @note Por platform:
     * - ESP32/TFT_eSPI: Soporta 16-bit (default), 8-bit con sprite
     * - SDL2/Native: Soporta 24-bit, 16-bit
     * 
     * @par Ejemplo de uso:
     * @code
     * // Cambiar a 8-bit indexed para displays lentos
     * renderer.setColorDepth(8);  // Reducir bandwidth
     * 
     * // Restaurar a 16-bit RGB565 (default)
     * renderer.setColorDepth(16);
     * @endcode
     */
    virtual void setColorDepth(int depth) {
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
        (void)enabled;
        // Default: no-op
    }

        /**
     * @brief Check if automatic dirty marking is enabled.
     * @return true if auto-marking is enabled
     */
    virtual bool isAutoMarkDirty() const {
        return false;
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
        (void)enabled;
        // Default: no-op
    }

    /**
     * @brief Check if debug dirty regions overlay is enabled.
     * @return true if debug overlay is enabled
     */
    virtual bool isDebugDirtyRegions() const {
        return false;
    }


};

}
