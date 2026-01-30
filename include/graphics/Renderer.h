/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#pragma once
#include "DrawSurface.h"
#include "DisplayConfig.h"
#include "Color.h"
#include "Font.h"

#ifdef PLATFORM_ESP32
    #include <mock/MockSafeString.h>
#endif

namespace pixelroot32::graphics {

/**
 * @brief Compact sprite descriptor for monochrome bitmapped sprites.
 *
 * Sprites are stored as an array of 16-bit rows. Each row packs horizontal
 * pixels into bits, using the following convention:
 *
 * - Bit 0 represents the leftmost pixel of the row.
 * - Bit (width - 1) represents the rightmost pixel of the row.
 *
 * Only the lowest (width) bits of each row are used. A bit value of 1 means
 * "pixel on", 0 means "pixel off".
 *
 * This format is optimized for small microcontroller displays (NES/GameBoy
 * style assets) and keeps data in flash-friendly, constexpr-friendly form.
 */
struct Sprite {
    const uint16_t* data;   ///< Pointer to packed row data (size = height).
    uint8_t         width;  ///< Sprite width in pixels (<= 16).
    uint8_t         height; ///< Sprite height in pixels.
};

#ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
struct Sprite2bpp {
    const uint8_t*  data;
    const Color*    palette;
    uint8_t         width;
    uint8_t         height;
    uint8_t         paletteSize;
};
#endif

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
struct Sprite4bpp {
    const uint8_t*  data;
    const Color*    palette;
    uint8_t         width;
    uint8_t         height;
    uint8_t         paletteSize;
};
#endif

/**
 * @brief Single monochrome layer used by layered sprites.
 *
 * Each layer uses the same width/height as its owning MultiSprite but can
 * provide its own bitmap and color.
 */
struct SpriteLayer {
    const uint16_t* data; ///< Pointer to packed row data for this layer.
    Color           color;///< Color used for "on" pixels in this layer.
};

/**
 * @brief Multi-layer, multi-color sprite built from 1bpp layers.
 *
 * A MultiSprite combines several SpriteLayer entries that share the same
 * width and height. Layers are drawn in array order, allowing more complex
 * visuals (highlights, outlines) while keeping each layer 1bpp.
 *
 * This design keeps compatibility with the existing Sprite format while
 * enabling NES/GameBoy-style layered sprites.
 */
struct MultiSprite {
    uint8_t              width;      ///< Sprite width in pixels (<= 16).
    uint8_t              height;     ///< Sprite height in pixels.
    const SpriteLayer*   layers;     ///< Pointer to array of layers.
    uint8_t              layerCount; ///< Number of layers in the array.
};

template<typename T>
struct TileMapGeneric {
    uint8_t*        indices;
    uint8_t         width;
    uint8_t         height;
    const T*        tiles;
    uint8_t         tileWidth;
    uint8_t         tileHeight;
    uint16_t        tileCount;
};

using TileMap = TileMapGeneric<Sprite>;

#ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
using TileMap2bpp = TileMapGeneric<Sprite2bpp>;
#endif

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
using TileMap4bpp = TileMapGeneric<Sprite4bpp>;
#endif

/**
 * @brief Single animation frame that can reference either a Sprite or a MultiSprite.
 *
 * Exactly one of the pointers is expected to be non-null for a valid frame.
 * This allows the same animation system to drive both simple and layered
 * sprites without exposing bit-level details to game code.
 */
struct SpriteAnimationFrame {
    const Sprite*      sprite;      ///< Optional pointer to a simple 1bpp sprite frame.
    const MultiSprite* multiSprite; ///< Optional pointer to a layered sprite frame.
};

/**
 * @brief Lightweight, step-based sprite animation controller.
 *
 * SpriteAnimation owns no memory. It references a compile-time array of
 * SpriteAnimationFrame entries and exposes simple integer-based control:
 *
 * - step(): advance to the next frame (wrapping at frameCount)
 * - reset(): go back to frame 0
 * - getCurrentSprite()/getCurrentMultiSprite(): query current frame data
 *
 * The animation object never draws anything; Actors remain responsible for
 * asking which frame to render and calling Renderer accordingly.
 *
 * Initially this struct is used for "step-based" animation (advance once per
 * logical event, such as a horde movement). The design can be extended later
 * with time-based advancement without changing Renderer.
 */
struct SpriteAnimation {
    const SpriteAnimationFrame* frames;    ///< Pointer to immutable frame table.
    uint8_t                     frameCount;///< Number of frames in the table.
    uint8_t                     current;   ///< Current frame index [0, frameCount).

    /// Reset the animation to the first frame.
    void reset() {
        current = 0;
    }

    /// Advance to the next frame in a loop (step-based advancement).
    void step() {
        if (!frames || frameCount == 0) {
            return;
        }
        ++current;
        if (current >= frameCount) {
            current = 0;
        }
    }

    /// Get the current frame descriptor (may contain either type of sprite).
    const SpriteAnimationFrame& getCurrentFrame() const {
        return frames[current];
    }

    /// Convenience helper: returns the current simple Sprite, if any.
    const Sprite* getCurrentSprite() const {
        if (!frames || frameCount == 0) {
            return nullptr;
        }
        return frames[current].sprite;
    }

    /// Convenience helper: returns the current MultiSprite, if any.
    const MultiSprite* getCurrentMultiSprite() const {
        if (!frames || frameCount == 0) {
            return nullptr;
        }
        return frames[current].multiSprite;
    }
};

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
     * @brief Draws a string of text (legacy method, uses default font).
     * @param text The text to draw.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size multiplier.
     */
    void drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size);

    /**
     * @brief Draws a string of text using a specific font.
     * @param text The text to draw.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size multiplier.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     */
    void drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font);

    /**
     * @brief Draws text centered horizontally at a given Y coordinate (legacy method, uses default font).
     * @param text The text to draw.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size.
     */
    void drawTextCentered(const char* text, int16_t y, Color color, uint8_t size);

    /**
     * @brief Draws text centered horizontally at a given Y coordinate using a specific font.
     * @param text The text to draw.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     */
    void drawTextCentered(const char* text, int16_t y, Color color, uint8_t size, const Font* font);

    /**
     * @brief Draws a filled circle.
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Radius of the circle.
     * @param color Fill color.
     */
    void drawFilledCircle(int x, int y, int radius, Color color);   

    /**
     * @brief Draws a circle outline.
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Radius of the circle.
     * @param color Outline color.
     */
    void drawCircle(int x, int y, int radius, Color color);
    
    /**
     * @brief Draws a rectangle outline.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Outline color.
     */
    void drawRectangle(int x, int y, int width, int height, Color color);

    /**
     * @brief Draws a filled rectangle.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Fill color.
     */
    void drawFilledRectangle(int x, int y, int width, int height, Color color);

    /**
     * @brief Draws a filled rectangle with a 16-bit color.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Fill color (RGB565).
     */
    void drawFilledRectangleW(int x, int y, int width, int height, uint16_t color);

    /**
     * @brief Draws a line between two points.
     * @param x1 Start X.
     * @param y1 Start Y.
     * @param x2 End X.
     * @param y2 End Y.
     * @param color Line color.
     */
    void drawLine(int x1, int y1, int x2, int y2, Color color); 

    /**
     * @brief Draws a bitmap image.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the bitmap.
     * @param height Height of the bitmap.
     * @param bitmap Pointer to the bitmap data.
     * @param color Color to draw the bitmap pixels (if monochrome) or ignored.
     */
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, Color color);

    /**
     * @brief Draws a single pixel.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Pixel color.
     */
    void drawPixel(int x, int y, Color color);

    /**
     * @brief Sets the logical display size (rendering resolution).
     * @param w Logical width.
     * @param h Logical height.
     */
    void setDisplaySize(int w, int h) {
        logicalWidth = w;
        logicalHeight = h;
    }

    /// @brief Gets the logical rendering width.
    int getLogicalWidth() const { return logicalWidth; }
    
    /// @brief Gets the logical rendering height.
    int getLogicalHeight() const { return logicalHeight; }
    
    /// @deprecated Use getLogicalWidth() instead.
    int getWidth() const { return logicalWidth; }
    
    /// @deprecated Use getLogicalHeight() instead.
    int getHeight() const { return logicalHeight; }

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

    /**
     * @brief Sets the render context for palette selection.
     * 
     * This method allows the renderer to use the appropriate palette based on
     * the current render layer. When set, primitives will use this context
     * instead of their default (Sprite).
     * 
     * @param context The palette context to use (Background or Sprite).
     *                 Pass nullptr to use method-specific defaults.
     */
    void setRenderContext(PaletteContext* context) {
        currentRenderContext = context;
    }

    /**
     * @brief Gets the current render context.
     * @return Pointer to the current context, or nullptr if using defaults.
     */
    PaletteContext* getRenderContext() const {
        return currentRenderContext;
    }

    /**
     * @brief Draws a 1bpp monochrome sprite using the Sprite descriptor.
     *
     * Sprite data is interpreted bit-by-bit using the Sprite convention:
     * bit 0 = leftmost pixel, bit (width - 1) = rightmost pixel.
     *
     * This API intentionally hides all bit-level details from game code.
     *
     * @param sprite Sprite descriptor (data, width, height).
     * @param x      Top-left X coordinate in logical screen space.
     * @param y      Top-left Y coordinate in logical screen space.
     * @param color  Color used for "on" pixels.
     * @param flipX  If true, sprite is mirrored horizontally.
     */
    void drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX = false);

    /**
     * @brief Draws a scaled 1bpp monochrome sprite.
     *
     * Similar to drawSprite but applies nearest-neighbor scaling.
     * The destination size is calculated as ceil(width * scaleX) x ceil(height * scaleY).
     *
     * @param sprite Sprite descriptor.
     * @param x      Top-left X coordinate.
     * @param y      Top-left Y coordinate.
     * @param scaleX Horizontal scaling factor (e.g., 1.25).
     * @param scaleY Vertical scaling factor (e.g., 1.25).
     * @param color  Color used for "on" pixels.
     * @param flipX  If true, sprite is mirrored horizontally before scaling.
     */
    void drawSprite(const Sprite& sprite, int x, int y, float scaleX, float scaleY, Color color, bool flipX = false);

#ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
    void drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX = false);
#endif

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
    void drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX = false);
#endif

    /**
     * @brief Draws a multi-layer sprite composed of several 1bpp layers.
     *
     * Each layer is rendered in array order using the existing drawSprite()
     * implementation, avoiding duplicated bit iteration logic.
     *
     * @param sprite Multi-layer sprite descriptor.
     * @param x      Top-left X coordinate in logical screen space.
     * @param y      Top-left Y coordinate in logical screen space.
     */
    void drawMultiSprite(const MultiSprite& sprite, int x, int y);

    /**
     * @brief Draws a scaled multi-layer sprite.
     *
     * Reuses the scaled drawSprite implementation for each layer.
     *
     * @param sprite Multi-layer sprite descriptor.
     * @param x      Top-left X coordinate.
     * @param y      Top-left Y coordinate.
     * @param scaleX Horizontal scaling factor.
     * @param scaleY Vertical scaling factor.
     */
    void drawMultiSprite(const MultiSprite& sprite, int x, int y, float scaleX, float scaleY);

    /**
     * @brief Draws a tilemap of 1bpp sprites.
     */
    void drawTileMap(const TileMap& map, int originX, int originY, Color color = Color::White);

#ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
    /**
     * @brief Draws a tilemap of 2bpp sprites.
     */
    void drawTileMap(const TileMap2bpp& map, int originX, int originY);
#endif

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
    /**
     * @brief Draws a tilemap of 4bpp sprites.
     */
    void drawTileMap(const TileMap4bpp& map, int originX, int originY);
#endif

private:
    DrawSurface* drawer; ///< Pointer to the platform-specific implementation.

    DisplayConfig config;

    int logicalWidth = 240;  ///< Logical rendering width (used for clipping)
    int logicalHeight = 240; ///< Logical rendering height (used for clipping)

    int xOffset = 0;
    int yOffset = 0;

    PaletteContext* currentRenderContext = nullptr; ///< Current render context for palette selection (nullptr = use method defaults)

#ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
    void drawSpriteInternal(const Sprite2bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX);
#endif
#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
    void drawSpriteInternal(const Sprite4bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX);
#endif
};

}
