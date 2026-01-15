#pragma once
#include "DrawSurface.h"
#include "DisplayConfig.h"

#ifdef PLATFORM_ESP32
    #include <mock/MockSafeString.h>
#endif

namespace pixelroot32::graphics {

/**
 * @class Renderer
 * @brief High-level graphics rendering system.
 *
 * The Renderer class provides a unified API for drawing shapes, text, and images.
 * It abstracts the underlying hardware implementation (DrawSurface) and manages
 * display configuration, including rotation and offsets.
 */
class Renderer {
public:
    /**
     * @brief Constructs the Renderer with a specific display configuration.
     * @param config The display configuration settings.
     */
    Renderer(const DisplayConfig& config);

    ~Renderer() {
       delete drawer;
    }

    /**
     * @brief Initializes the renderer and the underlying draw surface.
     */
    void init();

    /**
     * @brief Prepares the buffer for a new frame (clears screen).
     */
    void beginFrame();

    /**
     * @brief Finalizes the frame and sends the buffer to the display.
     */
    void endFrame();

    /**
     * @brief Gets the underlying DrawSurface implementation.
     * @return Reference to the DrawSurface.
     */
    DrawSurface& getDrawSurface() { return *drawer; }

    /**
     * @brief Draws a string of text.
     * @param text The text to draw.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Text color (RGB565).
     * @param size Text size multiplier.
     */
    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size);

    /**
     * @brief Draws text centered horizontally at a given Y coordinate.
     * @param text The text to draw.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size.
     */
    void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size);

    /**
     * @brief Draws a filled circle.
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Radius of the circle.
     * @param color Fill color.
     */
    void drawFilledCircle(int x, int y, int radius,uint16_t color);

    /**
     * @brief Draws a circle outline.
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Radius of the circle.
     * @param color Outline color.
     */
    void drawCircle(int x, int y, int radius,uint16_t color);

    /**
     * @brief Draws a rectangle outline.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Outline color.
     */
    void drawRectangle(int x, int y, int width, int height,uint16_t color);

    /**
     * @brief Draws a filled rectangle.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Fill color.
     */
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color);

    /**
     * @brief Draws a line between two points.
     * @param x1 Start X.
     * @param y1 Start Y.
     * @param x2 End X.
     * @param y2 End Y.
     * @param color Line color.
     */
    void drawLine(int x1, int y1, int x2, int y2,uint16_t color);

    /**
     * @brief Draws a bitmap image.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the bitmap.
     * @param height Height of the bitmap.
     * @param bitmap Pointer to the bitmap data.
     * @param color Color to draw the bitmap pixels (if monochrome) or ignored.
     */
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color);

    /**
     * @brief Draws a single pixel.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Pixel color.
     */
    void drawPixel(int x, int y, uint16_t color);

    /**
     * @brief Sets the logical display size.
     * @param w Width.
     * @param h Height.
     */
    void setDisplaySize(int w, int h) {
        width = w;
        height = h;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    /**
     * @brief Sets a global offset for all drawing operations.
     * @param x X offset.
     * @param y Y offset.
     */
    void setDisplayOffset(int x, int y) {
        xOffset = x;
        yOffset = y;
    }

    /**
     * @brief Sets the display contrast (brightness).
     * @param level Contrast level (0-255).
     */
    void setContrast(uint8_t level) {
        getDrawSurface().setContrast(level);
    }

    /**
     * @brief Sets the font for text rendering.
     * @param font Pointer to the font data.
     */
    void setFont(const uint8_t* font);

    int getXOffset() const { return xOffset; }
    int getYOffset() const { return yOffset; }

private:
    DrawSurface* drawer; ///< Pointer to the platform-specific implementation.

    DisplayConfig config;

    int width = 240;
    int height = 240;

    int xOffset = 0;
    int yOffset = 0;
};

}
