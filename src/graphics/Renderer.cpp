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
#include "graphics/Renderer.h"
#include "graphics/FontManager.h"
#include <stdarg.h>
#include <cmath>
#include <cstring>
#ifdef PLATFORM_NATIVE
    #include "drivers/native/SDL2_Drawer.h"
    #include "../../src/platforms/mock/MockSPI.h"
#else
    #include "drivers/esp32/TFT_eSPI_Drawer.h"
    #include <SPI.h>
    #include <SafeString.h>
#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace pixelroot32::graphics {

    inline bool isDrawable(Color c) {
        return c != Color::Transparent;
    }

    Renderer::Renderer(const DisplayConfig& config) 
        : config(config),
          logicalWidth(config.logicalWidth),
          logicalHeight(config.logicalHeight)
    {        
        // Note: This constructor may be problematic if config is not moved,
        // as DrawSurface ownership is tied to DisplayConfig.
        // We cast away const to allow moving the drawer.
        DisplayConfig& nonConstConfig = const_cast<DisplayConfig&>(config);
        drawer = nonConstConfig.releaseDrawSurface();
        
        // If releaseDrawSurface returned null (meaning it was already moved or not initialized),
        // we might need to re-initialize it if we want to support multiple Renderers from one Config
        // (though ownership semantics suggest only one should own it).
        if (!drawer) {
            nonConstConfig.initDrawSurface();
            drawer = nonConstConfig.releaseDrawSurface();
        }

        xOffset = 0;
        yOffset = 0;
    }

    Renderer::Renderer(DisplayConfig&& config)
        : config(std::move(config)),
          logicalWidth(this->config.logicalWidth),
          logicalHeight(this->config.logicalHeight)
    {
        drawer = this->config.releaseDrawSurface();
        xOffset = 0;
        yOffset = 0;
    }


    void Renderer::init() {
        // Configure logical resolution (rendering framebuffer size)
        getDrawSurface().setDisplaySize(config.logicalWidth, config.logicalHeight);
        
        // Configure physical resolution (hardware display size for scaling)
        getDrawSurface().setPhysicalSize(config.physicalWidth, config.physicalHeight);
        
        // Set display rotation (0-3 or 0-270)
        getDrawSurface().setRotation(config.rotation);
        
        getDrawSurface().init();
    }

    void Renderer::beginFrame() {
        getDrawSurface().clearBuffer();
    }

    void Renderer::endFrame() {
        getDrawSurface().sendBuffer();
    }

    void Renderer::drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size) {
        // Legacy method: delegate to new method with default font
        drawText(text, x, y, color, size, nullptr);
    }

    void Renderer::drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font) {
        if (!isDrawable(color) || !text || !*text) {
            return;
        }

        // Get active font (parameter or default)
        const Font* activeFont = font ? font : FontManager::getDefaultFont();
        if (!activeFont || !activeFont->glyphs) {
            // No font available - cannot render text
            // Note: A default font should always be set in Engine::init()
            return;
        }

        int16_t currentX = x;
        const char* p = text;
        float scale = static_cast<float>(size);

        while (*p) {
            char c = *p++;
            uint8_t glyphIndex = FontManager::getGlyphIndex(c, activeFont);

            // Skip unsupported characters
            if (glyphIndex == 255) {
                // Advance by glyph width for unsupported characters
                currentX += static_cast<int16_t>((activeFont->glyphWidth + activeFont->spacing) * scale);
                continue;
            }

            // Get the glyph sprite
            const Sprite& glyph = activeFont->glyphs[glyphIndex];

            // Render the glyph
            if (size == 1) {
                // Use non-scaled version for size 1 (more efficient)
                drawSprite(glyph, currentX, y, color, false);
            } else {
                // Use scaled version for size > 1
                drawSprite(glyph, currentX, y, scale, scale, color, false);
            }

            // Advance position
            currentX += static_cast<int16_t>((activeFont->glyphWidth + activeFont->spacing) * scale);
        }
    }

    void Renderer::drawTextCentered(const char* text, int16_t y, Color color, uint8_t size) {
        // Legacy method: delegate to new method with default font
        drawTextCentered(text, y, color, size, nullptr);
    }

    void Renderer::drawTextCentered(const char* text, int16_t y, Color color, uint8_t size, const Font* font) {
        if (!isDrawable(color) || !text || !*text) {
            return;
        }

        // Get active font (parameter or default)
        const Font* activeFont = font ? font : FontManager::getDefaultFont();
        if (!activeFont || !activeFont->glyphs) {
            // No font available - cannot render text
            // Note: A default font should always be set in Engine::init()
            return;
        }

        // Calculate text width and center it
        int16_t textWidth = FontManager::textWidth(activeFont, text, size);
        int16_t x = (logicalWidth - textWidth) / 2;

        // Render using the regular drawText method
        drawText(text, x, y, color, size, activeFont);
    }

    void Renderer::drawFilledCircle(int x, int y, int radius, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawFilledCircle(finalX, finalY, radius, resolveColor(color, context));
    }

    void Renderer::drawCircle(int x, int y, int radius, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawCircle(finalX, finalY, radius, resolveColor(color, context));
    }

    void Renderer::drawRectangle(int x, int y, int width, int height, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawRectangle(finalX, finalY, width, height, resolveColor(color, context));
    }

    void Renderer::drawFilledRectangle(int x, int y, int width, int height, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawFilledRectangle(finalX, finalY, width, height, resolveColor(color, context));
    }

    void Renderer::drawFilledRectangleW(int x, int y, int width, int height, uint16_t color) {
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawFilledRectangle(finalX, finalY, width, height, color);
    }

    void Renderer::drawLine(int x1, int y1, int x2, int y2, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX1 = offsetBypass ? x1 : xOffset + x1;
        int finalY1 = offsetBypass ? y1 : yOffset + y1;
        int finalX2 = offsetBypass ? x2 : xOffset + x2;
        int finalY2 = offsetBypass ? y2 : yOffset + y2;
        getDrawSurface().drawLine(finalX1, finalY1, finalX2, finalY2, resolveColor(color, context));
    }

    void Renderer::setFont(const uint8_t* font) {
        (void)font;
        // Optional: Implement font setting if your DrawSurface supports it.
    }

    //draw an image to the screen in an bitmap format
    void Renderer::drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawBitmap(finalX, finalY, width, height, bitmap, resolveColor(color, context));
    }

    void IRAM_ATTR Renderer::drawPixel(int x, int y, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawPixel(finalX, finalY, resolveColor(color, context));
    }

    void IRAM_ATTR Renderer::drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX) {
        if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0) {
            return;
        }

        const int screenW = logicalWidth;
        const int screenH = logicalHeight;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        const uint16_t resolvedColor = resolveColor(color, context);

        int startX = offsetBypass ? x : xOffset + x;
        int startY = offsetBypass ? y : yOffset + y;

        for (int row = 0; row < sprite.height; ++row) {
            const int logicalY = startY + row;
            // Note: clipping against logicalWidth/Height might be tricky if xOffset is applied,
            // but the Driver should handle physical clipping. Logical clipping here is for efficiency.
            if (logicalY < 0 || logicalY >= screenH) {
                continue;
            }

            const uint16_t bits = sprite.data[row];

            for (int col = 0; col < sprite.width; ++col) {
                // Read bits from MSB to LSB (bit (width-1) = leftmost, bit 0 = rightmost)
                const int bitIndex = sprite.width - 1 - col;
                const bool bitSet = (bits & (static_cast<uint16_t>(1u) << bitIndex)) != 0;
                if (!bitSet) {
                    continue;
                }

                int logicalX = flipX
                    ? startX + (sprite.width - 1 - col)
                    : startX + col;

                if (logicalX < 0 || logicalX >= screenW) {
                    continue;
                }

                getDrawSurface().drawPixel(logicalX, logicalY, resolvedColor);
            }
        }
    }

    void Renderer::drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
            if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0 || sprite.palette == nullptr || sprite.paletteSize == 0) {
                return;
            }

            uint16_t paletteLUT[4];
            uint8_t paletteCount = sprite.paletteSize > 4 ? 4 : sprite.paletteSize;
            PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
            for (uint8_t i = 0; i < paletteCount; ++i) {
                paletteLUT[i] = resolveColor(sprite.palette[i], context);
            }

            drawSpriteInternal(sprite, x, y, paletteLUT, flipX);
        }
    }

    void IRAM_ATTR Renderer::drawSpriteInternal(const Sprite2bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
            const int screenW = logicalWidth;
            const int screenH = logicalHeight;
            const int bitsPerPixel = 2;
            const int rowStrideBytes = (sprite.width * bitsPerPixel + 7) / 8;

            int startX = offsetBypass ? x : xOffset + x;
            int startY = offsetBypass ? y : yOffset + y;

            // Data: 16-bit words (8 pixels per word). Compiler pack_2bpp: LSB = left pixel (bitOffset = (col&7)<<1), word order [left, right]
            for (int row = 0; row < sprite.height; ++row) {
                const int logicalY = startY + row;
                if (logicalY < 0 || logicalY >= screenH) continue;

                const uint16_t* rowWords = reinterpret_cast<const uint16_t*>(sprite.data + row * rowStrideBytes);

                for (int col = 0; col < sprite.width; ++col) {
                    const int wordIdx = col >> 3; // 8 pixels per word; word 0 = left half, word 1 = right half
                    const int bitOffset = (col & 7) << 1; // LSB = pixel 0 (match compiler pack_2bpp)
                    const uint8_t val = (rowWords[wordIdx] >> bitOffset) & 0x03;

                    if (val == 0) continue;

                    const int logicalX = flipX ? startX + (sprite.width - 1 - col) : startX + col;
                    if (logicalX < 0 || logicalX >= screenW) continue;

                    getDrawSurface().drawPixel(logicalX, logicalY, paletteLUT[val]);
                }
            }
        }
    }

    void Renderer::drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
            if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0 || sprite.palette == nullptr || sprite.paletteSize == 0) {
                return;
            }

            uint16_t paletteLUT[16];
            uint8_t paletteCount = sprite.paletteSize > 16 ? 16 : sprite.paletteSize;
            PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
            for (uint8_t i = 0; i < paletteCount; ++i) {
                paletteLUT[i] = resolveColor(sprite.palette[i], context);
            }

            drawSpriteInternal(sprite, x, y, paletteLUT, flipX);
        }
    }

    void IRAM_ATTR Renderer::drawSpriteInternal(const Sprite4bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
            const int screenW = logicalWidth;
            const int screenH = logicalHeight;
            const int bitsPerPixel = 4;
            const int rowStrideBytes = (sprite.width * bitsPerPixel + 7) / 8;

            int startX = offsetBypass ? x : xOffset + x;
            int startY = offsetBypass ? y : yOffset + y;

            for (int row = 0; row < sprite.height; ++row) {
                const int logicalY = startY + row;
                if (logicalY < 0 || logicalY >= screenH) continue;

                const uint8_t* rowData = sprite.data + row * rowStrideBytes;

                for (int col = 0; col < sprite.width; ++col) {
                    const int byteIdx = col >> 1;
                    const int bitOffset = (col & 1) << 2;
                    const uint8_t val = (rowData[byteIdx] >> bitOffset) & 0x0F;

                    if (val == 0) continue;

                    const int logicalX = flipX ? startX + (sprite.width - 1 - col) : startX + col;
                    if (logicalX < 0 || logicalX >= screenW) continue;

                    getDrawSurface().drawPixel(logicalX, logicalY, paletteLUT[val]);
                }
            }
        }
    }

    void Renderer::drawMultiSprite(const MultiSprite& sprite, int x, int y) {
        // Early-out if descriptor is invalid.
        if (sprite.layers == nullptr || sprite.layerCount == 0 ||
            sprite.width == 0 || sprite.height == 0) {
            return;
        }

        Sprite singleLayer;
        singleLayer.width  = sprite.width;
        singleLayer.height = sprite.height;

        // Iterate over layers and reuse drawSprite() for each one.
        for (uint8_t i = 0; i < sprite.layerCount; ++i) {
            const SpriteLayer& layer = sprite.layers[i];
            if (layer.data == nullptr) {
                continue;
            }

            singleLayer.data = layer.data;
            drawSprite(singleLayer, x, y, layer.color, false);
        }
    }

    void Renderer::drawSprite(const Sprite& sprite, int x, int y, float scaleX, float scaleY, Color color, bool flipX) {
        if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0 || scaleX <= 0 || scaleY <= 0) {
            return;
        }

        const int screenW = logicalWidth;
        const int screenH = logicalHeight;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        const uint16_t resolvedColor = resolveColor(color, context);

        const int dstWidth = static_cast<int>(std::ceil(sprite.width * scaleX));
        const int dstHeight = static_cast<int>(std::ceil(sprite.height * scaleY));

        int startX = offsetBypass ? x : xOffset + x;
        int startY = offsetBypass ? y : yOffset + y;

        for (int dstRow = 0; dstRow < dstHeight; ++dstRow) {
            const int logicalY = startY + dstRow;
            if (logicalY < 0 || logicalY >= screenH) {
                continue;
            }

            int srcRow = (dstRow * sprite.height) / dstHeight;
            if (srcRow >= sprite.height) srcRow = sprite.height - 1;

            const uint16_t bits = sprite.data[srcRow];

            for (int dstCol = 0; dstCol < dstWidth; ++dstCol) {
                int srcCol = (dstCol * sprite.width) / dstWidth;
                if (srcCol >= sprite.width) srcCol = sprite.width - 1;

                if (flipX) {
                    srcCol = sprite.width - 1 - srcCol;
                }

                // Read bits from MSB to LSB (bit (width-1) = leftmost, bit 0 = rightmost)
                const int bitIndex = sprite.width - 1 - srcCol;
                const bool bitSet = (bits & (static_cast<uint16_t>(1u) << bitIndex)) != 0;
                if (!bitSet) {
                    continue;
                }

                const int logicalX = startX + dstCol;
                if (logicalX < 0 || logicalX >= screenW) {
                    continue;
                }

                getDrawSurface().drawPixel(logicalX, logicalY, resolvedColor);
            }
        }
    }

    void Renderer::drawMultiSprite(const MultiSprite& sprite, int x, int y, float scaleX, float scaleY) {
         // Early-out if descriptor is invalid.
        if (sprite.layers == nullptr || sprite.layerCount == 0 ||
            sprite.width == 0 || sprite.height == 0) {
            return;
        }

        Sprite singleLayer;
        singleLayer.width  = sprite.width;
        singleLayer.height = sprite.height;

        // Iterate over layers and reuse scaled drawSprite() for each one.
        for (uint8_t i = 0; i < sprite.layerCount; ++i) {
            const SpriteLayer& layer = sprite.layers[i];
            if (layer.data == nullptr) {
                continue;
            }

            singleLayer.data = layer.data;
            drawSprite(singleLayer, x, y, scaleX, scaleY, layer.color, false);
        }
    }

    void IRAM_ATTR Renderer::drawTileMap(const TileMap& map, int originX, int originY, Color color) {
        if (map.indices == nullptr || map.tiles == nullptr ||
            map.width == 0 || map.height == 0 ||
            map.tileWidth == 0 || map.tileHeight == 0 ||
            map.tileCount == 0) {
            return;
        }

        // Set background context automatically for tilemaps
        PaletteContext bgContext = PaletteContext::Background;
        PaletteContext* oldContext = currentRenderContext;
        setRenderContext(&bgContext);

        int viewOriginX = offsetBypass ? originX : xOffset + originX;
        int viewOriginY = offsetBypass ? originY : yOffset + originY;

        // Viewport Culling: Only draw tiles that are within the screen boundaries
        int startCol = (viewOriginX < 0) ? (-viewOriginX / map.tileWidth) : 0;
        int endCol = (viewOriginX + map.width * map.tileWidth > logicalWidth) 
                     ? ((logicalWidth - viewOriginX + map.tileWidth - 1) / map.tileWidth) 
                     : map.width;
        
        int startRow = (viewOriginY < 0) ? (-viewOriginY / map.tileHeight) : 0;
        int endRow = (viewOriginY + map.height * map.tileHeight > logicalHeight) 
                     ? ((logicalHeight - viewOriginY + map.tileHeight - 1) / map.tileHeight) 
                     : map.height;

        // Clamp to map boundaries
        if (startCol < 0) startCol = 0;
        if (endCol > map.width) endCol = map.width;
        if (startRow < 0) startRow = 0;
        if (endRow > map.height) endRow = map.height;

        for (int ty = startRow; ty < endRow; ++ty) {
            int baseY = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = startCol; tx < endCol; ++tx) {
                int baseX = originX + tx * map.tileWidth;
                uint8_t index = map.indices[rowIndexBase + tx];
                if (index == 0 || index >= map.tileCount) {
                    continue;
                }

                drawSprite(map.tiles[index], baseX, baseY, color, false);
            }
        }

        // Restore context
        setRenderContext(oldContext);
    }

#ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
    void IRAM_ATTR Renderer::drawTileMap(const TileMap2bpp& map, int originX, int originY) {
        if (map.indices == nullptr || map.tiles == nullptr ||
            map.width == 0 || map.height == 0 ||
            map.tileWidth == 0 || map.tileHeight == 0 ||
            map.tileCount == 0) {
            return;
        }

        // Set background context automatically for tilemaps
        PaletteContext bgContext = PaletteContext::Background;
        PaletteContext* oldContext = currentRenderContext;
        setRenderContext(&bgContext);

        int viewOriginX = offsetBypass ? originX : xOffset + originX;
        int viewOriginY = offsetBypass ? originY : yOffset + originY;

        // Viewport Culling
        int startCol = (viewOriginX < 0) ? (-viewOriginX / map.tileWidth) : 0;
        int endCol = (viewOriginX + map.width * map.tileWidth > logicalWidth) 
                     ? ((logicalWidth - viewOriginX + map.tileWidth - 1) / map.tileWidth) 
                     : map.width;
        int startRow = (viewOriginY < 0) ? (-viewOriginY / map.tileHeight) : 0;
        int endRow = (viewOriginY + map.height * map.tileHeight > logicalHeight) 
                     ? ((logicalHeight - viewOriginY + map.tileHeight - 1) / map.tileHeight) 
                     : map.height;

        if (startCol < 0) startCol = 0;
        if (endCol > map.width) endCol = map.width;
        if (startRow < 0) startRow = 0;
        if (endRow > map.height) endRow = map.height;

        // Palette Caching
        uint16_t cachedLUT[4];
        const Color* lastPalette = nullptr;
        bool lutReady = false;

        for (int ty = startRow; ty < endRow; ++ty) {
            int baseY = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = startCol; tx < endCol; ++tx) {
                int baseX = originX + tx * map.tileWidth;
                uint8_t index = map.indices[rowIndexBase + tx];
                
                // Optimized check: skip empty tile (index 0) and out of bounds
                if (index == 0 || index >= map.tileCount) {
                    continue;
                }

                const Sprite2bpp& tile = map.tiles[index];
                
                // Update LUT only if palette changes
                if (!lutReady || tile.palette != lastPalette) {
                    uint8_t paletteCount = tile.paletteSize > 4 ? 4 : tile.paletteSize;
                    for (uint8_t i = 0; i < paletteCount; ++i) {
                        cachedLUT[i] = resolveColor(tile.palette[i], bgContext);
                    }
                    lastPalette = tile.palette;
                    lutReady = true;
                }

                drawSpriteInternal(tile, baseX, baseY, cachedLUT, false);
            }
        }

        // Restore context
        setRenderContext(oldContext);
    }
#endif

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
    void IRAM_ATTR Renderer::drawTileMap(const TileMap4bpp& map, int originX, int originY) {
        if (map.indices == nullptr || map.tiles == nullptr ||
            map.width == 0 || map.height == 0 ||
            map.tileWidth == 0 || map.tileHeight == 0 ||
            map.tileCount == 0) {
            return;
        }

        // Set background context automatically for tilemaps
        PaletteContext bgContext = PaletteContext::Background;
        PaletteContext* oldContext = currentRenderContext;
        setRenderContext(&bgContext);

        int viewOriginX = offsetBypass ? originX : xOffset + originX;
        int viewOriginY = offsetBypass ? originY : yOffset + originY;

        // Viewport Culling
        int startCol = (viewOriginX < 0) ? (-viewOriginX / map.tileWidth) : 0;
        int endCol = (viewOriginX + map.width * map.tileWidth > logicalWidth) 
                     ? ((logicalWidth - viewOriginX + map.tileWidth - 1) / map.tileWidth) 
                     : map.width;
        int startRow = (viewOriginY < 0) ? (-viewOriginY / map.tileHeight) : 0;
        int endRow = (viewOriginY + map.height * map.tileHeight > logicalHeight) 
                     ? ((logicalHeight - viewOriginY + map.tileHeight - 1) / map.tileHeight) 
                     : map.height;

        if (startCol < 0) startCol = 0;
        if (endCol > map.width) endCol = map.width;
        if (startRow < 0) startRow = 0;
        if (endRow > map.height) endRow = map.height;

        // Palette Caching
        uint16_t cachedLUT[16];
        const Color* lastPalette = nullptr;
        bool lutReady = false;

        for (int ty = startRow; ty < endRow; ++ty) {
            int baseY = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = startCol; tx < endCol; ++tx) {
                int baseX = originX + tx * map.tileWidth;
                uint8_t index = map.indices[rowIndexBase + tx];
                
                // Optimized check: skip empty tile (index 0) and out of bounds
                if (index == 0 || index >= map.tileCount) {
                    continue;
                }

                const Sprite4bpp& tile = map.tiles[index];
                
                // Update LUT only if palette changes
                if (!lutReady || tile.palette != lastPalette) {
                    uint8_t paletteCount = tile.paletteSize > 16 ? 16 : tile.paletteSize;
                    for (uint8_t i = 0; i < paletteCount; ++i) {
                        cachedLUT[i] = resolveColor(tile.palette[i], bgContext);
                    }
                    lastPalette = tile.palette;
                    lutReady = true;
                }

                drawSpriteInternal(tile, baseX, baseY, cachedLUT, false);
            }
        }

        // Restore context
        setRenderContext(oldContext);
    }
#endif
}
