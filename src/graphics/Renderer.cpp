/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 Gabriel Perez
 *
 * This file remains licensed under the MIT License.
 */
#include "graphics/Renderer.h"
#include <stdarg.h>
#include <cmath>
#ifdef PLATFORM_NATIVE
    #include "drivers/native/SDL2_Drawer.h"
    #include "../../src/platforms/mock/MockSPI.h"
#else
    #include "drivers/esp32/TFT_eSPI_Drawer.h"
    #include <SPI.h>
    #include <SafeString.h>
#endif

namespace pixelroot32::graphics {

    inline bool isDrawable(Color c) {
        return c != Color::Transparent;
    }

    Renderer::Renderer(const DisplayConfig& config) : config(config) {        
        drawer = &config.getDrawSurface();
        xOffset = config.xOffset;
        yOffset = config.yOffset;
    }


    void Renderer::init() {
        setDisplaySize(config.width, config.height);
        getDrawSurface().setDisplaySize(getHeight(), getWidth());
        getDrawSurface().init();
    }

    void Renderer::beginFrame() {
        getDrawSurface().clearBuffer();
    }

    void Renderer::endFrame() {
        getDrawSurface().sendBuffer();
    }

    void Renderer::drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawText(text, x, y, resolveColor(color, context), size);
    }

    void Renderer::drawTextCentered(const char* text, int16_t y, Color color, uint8_t size) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawTextCentered(text, y, resolveColor(color, context), size);
    }

    void Renderer::drawFilledCircle(int x, int y, int radius, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawFilledCircle(xOffset + x, yOffset + y, radius, resolveColor(color, context));
    }

    void Renderer::drawCircle(int x, int y, int radius, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawCircle(xOffset + x, yOffset + y, radius, resolveColor(color, context));
    }

    void Renderer::drawRectangle(int x, int y, int width, int height, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawRectangle(xOffset + x, yOffset + y, width, height, resolveColor(color, context));
    }

    void Renderer::drawFilledRectangle(int x, int y, int width, int height, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawFilledRectangle(xOffset + x, yOffset + y, width, height, resolveColor(color, context));
    }

    void Renderer::drawFilledRectangleW(int x, int y, int width, int height, uint16_t color) {
        getDrawSurface().drawFilledRectangle(xOffset + x, yOffset + y, width, height, color);
    }

    void Renderer::drawLine(int x1, int y1, int x2, int y2, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawLine(xOffset + x1, yOffset + y1, xOffset + x2, yOffset + y2, resolveColor(color, context));
    }

    void Renderer::setFont(const uint8_t* font) {
        (void)font;
        // Optional: Implement font setting if your DrawSurface supports it.
    }

    //draw an image to the screen in an bitmap format
    void Renderer::drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawBitmap(xOffset + x, yOffset + y, width, height, bitmap, resolveColor(color, context));
    }

    void Renderer::drawPixel(int x, int y, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        getDrawSurface().drawPixel(x, y, resolveColor(color, context));
    }

    void Renderer::drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX) {
        if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0) {
            return;
        }

        const int screenW = width;
        const int screenH = height;
        const uint16_t resolvedColor = resolveColor(color, PaletteContext::Sprite);

        for (int row = 0; row < sprite.height; ++row) {
            const int logicalY = y + row;
            const int finalY = yOffset + logicalY;
            if (finalY < 0 || finalY >= screenH) {
                continue;
            }

            const uint16_t bits = sprite.data[row];

            for (int col = 0; col < sprite.width; ++col) {
                const bool bitSet = (bits & (static_cast<uint16_t>(1u) << col)) != 0;
                if (!bitSet) {
                    continue;
                }

                int logicalX = flipX
                    ? x + (sprite.width - 1 - col)
                    : x + col;

                const int finalX = xOffset + logicalX;
                if (finalX < 0 || finalX >= screenW) {
                    continue;
                }

                getDrawSurface().drawPixel(finalX, finalY, resolvedColor);
            }
        }
    }

#ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
    void Renderer::drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX) {
        if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0) {
            return;
        }

        if (sprite.palette == nullptr || sprite.paletteSize == 0) {
            return;
        }

        const int screenW = width;
        const int screenH = height;

        uint16_t paletteLUT[4];
        uint8_t paletteCount = sprite.paletteSize > 4 ? 4 : sprite.paletteSize;
        for (uint8_t i = 0; i < paletteCount; ++i) {
            paletteLUT[i] = resolveColor(sprite.palette[i], PaletteContext::Sprite);
        }

        const int bitsPerPixel = 2;
        const int rowStrideBits = sprite.width * bitsPerPixel;
        const int rowStrideBytes = (rowStrideBits + 7) / 8;

        for (int row = 0; row < sprite.height; ++row) {
            const int dstY = y + row;
            const int finalY = yOffset + dstY;
            if (finalY < 0 || finalY >= screenH) {
                continue;
            }

            const uint8_t* rowData = sprite.data + row * rowStrideBytes;

            for (int col = 0; col < sprite.width; ++col) {
                const int bitIndex = col * bitsPerPixel;
                const int byteIndex = bitIndex >> 3;
                const int shift = bitIndex & 7;

                const uint8_t packed = rowData[byteIndex];
                const uint8_t value = static_cast<uint8_t>((packed >> shift) & 0x3u);

                if (value == 0 || value >= paletteCount) {
                    continue;
                }

                int logicalX = flipX
                    ? x + (sprite.width - 1 - col)
                    : x + col;
                const int dstX = xOffset + logicalX;
                if (dstX < 0 || dstX >= screenW) {
                    continue;
                }

                getDrawSurface().drawPixel(dstX, finalY, paletteLUT[value]);
            }
        }
    }
#endif

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
    void Renderer::drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX) {
        if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0) {
            return;
        }

        if (sprite.palette == nullptr || sprite.paletteSize == 0) {
            return;
        }

        const int screenW = width;
        const int screenH = height;

        uint16_t paletteLUT[16];
        uint8_t paletteCount = sprite.paletteSize > 16 ? 16 : sprite.paletteSize;
        for (uint8_t i = 0; i < paletteCount; ++i) {
            paletteLUT[i] = resolveColor(sprite.palette[i], PaletteContext::Sprite);
        }

        const int bitsPerPixel = 4;
        const int rowStrideBits = sprite.width * bitsPerPixel;
        const int rowStrideBytes = (rowStrideBits + 7) / 8;

        for (int row = 0; row < sprite.height; ++row) {
            const int dstY = y + row;
            const int finalY = yOffset + dstY;
            if (finalY < 0 || finalY >= screenH) {
                continue;
            }

            const uint8_t* rowData = sprite.data + row * rowStrideBytes;

            for (int col = 0; col < sprite.width; ++col) {
                const int bitIndex = col * bitsPerPixel;
                const int byteIndex = bitIndex >> 3;
                const int shift = bitIndex & 7;

                const uint8_t packed = rowData[byteIndex];
                const uint8_t value = static_cast<uint8_t>((packed >> shift) & 0x0Fu);

                if (value == 0 || value >= paletteCount) {
                    continue;
                }

                int logicalX = flipX
                    ? x + (sprite.width - 1 - col)
                    : x + col;
                const int finalX = xOffset + logicalX;
                if (finalX < 0 || finalX >= screenW) {
                    continue;
                }

                getDrawSurface().drawPixel(finalX, finalY, paletteLUT[value]);
            }
        }
    }
#endif

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

        const int screenW = width;
        const int screenH = height;
        const uint16_t resolvedColor = resolveColor(color, PaletteContext::Sprite);

        const int dstWidth = static_cast<int>(std::ceil(sprite.width * scaleX));
        const int dstHeight = static_cast<int>(std::ceil(sprite.height * scaleY));

        for (int dstRow = 0; dstRow < dstHeight; ++dstRow) {
            const int logicalY = y + dstRow;
            const int finalY = yOffset + logicalY;
            if (finalY < 0 || finalY >= screenH) {
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

                const bool bitSet = (bits & (static_cast<uint16_t>(1u) << srcCol)) != 0;
                if (!bitSet) {
                    continue;
                }

                const int logicalX = x + dstCol;
                const int finalX = xOffset + logicalX;
                if (finalX < 0 || finalX >= screenW) {
                    continue;
                }

                getDrawSurface().drawPixel(finalX, finalY, resolvedColor);
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

    void Renderer::drawTileMap(const TileMap& map, int originX, int originY, Color color) {
        if (map.indices == nullptr || map.tiles == nullptr ||
            map.width == 0 || map.height == 0 ||
            map.tileWidth == 0 || map.tileHeight == 0 ||
            map.tileCount == 0) {
            return;
        }

        // Resolve color with Background context for tilemaps
        const uint16_t resolvedColor = resolveColor(color, PaletteContext::Background);
        const int screenW = width;
        const int screenH = height;

        for (int ty = 0; ty < map.height; ++ty) {
            int baseY = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = 0; tx < map.width; ++tx) {
                int baseX = originX + tx * map.tileWidth;
                uint8_t index = map.indices[rowIndexBase + tx];
                if (index >= map.tileCount) {
                    continue;
                }

                const Sprite& tile = map.tiles[index];
                if (tile.data == nullptr) {
                    continue;
                }

                // Draw tile directly using Background palette context
                for (int row = 0; row < tile.height; ++row) {
                    const int logicalY = baseY + row;
                    const int finalY = yOffset + logicalY;
                    if (finalY < 0 || finalY >= screenH) {
                        continue;
                    }

                    const uint16_t bits = tile.data[row];

                    for (int col = 0; col < tile.width; ++col) {
                        const bool bitSet = (bits & (static_cast<uint16_t>(1u) << col)) != 0;
                        if (!bitSet) {
                            continue;
                        }

                        const int logicalX = baseX + col;
                        const int finalX = xOffset + logicalX;
                        if (finalX < 0 || finalX >= screenW) {
                            continue;
                        }

                        getDrawSurface().drawPixel(finalX, finalY, resolvedColor);
                    }
                }
            }
        }
    }
}
