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

    Renderer::Renderer(const DisplayConfig& config) : config(config) {
        // #if defined(PLATFORM_NATIVE)
        //     drawer = new pixelroot32::drivers::native::SDL2_Drawer();
        // #else
        //     drawer = new pixelroot32::drivers::esp32::TFT_eSPI_Drawer();
        // #endif
        
        drawer = config.drawSurface;
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
        getDrawSurface().drawText(text, x, y, resolveColor(color), size);
    }

    void Renderer::drawTextCentered(const char* text, int16_t y, Color color, uint8_t size) {
        getDrawSurface().drawTextCentered(text, y, resolveColor(color), size);
    }

    void Renderer::drawFilledCircle(int x, int y, int radius, Color color) {
        getDrawSurface().drawFilledCircle(xOffset + x, yOffset + y, radius, resolveColor(color));
    }

    void Renderer::drawCircle(int x, int y, int radius, Color color) {
        getDrawSurface().drawCircle(xOffset + x, yOffset + y, radius, resolveColor(color));
    }

    void Renderer::drawRectangle(int x, int y, int width, int height, Color color) {
        getDrawSurface().drawRectangle(xOffset + x, yOffset + y, width, height, resolveColor(color));
    }

    void Renderer::drawFilledRectangle(int x, int y, int width, int height, Color color) {
        getDrawSurface().drawFilledRectangle(xOffset + x, yOffset + y, width, height, resolveColor(color));
    }

    void Renderer::drawFilledRectangleW(int x, int y, int width, int height, uint16_t color) {
        getDrawSurface().drawFilledRectangle(xOffset + x, yOffset + y, width, height, color);
    }

    void Renderer::drawLine(int x1, int y1, int x2, int y2, Color color) {
        getDrawSurface().drawLine(xOffset + x1, yOffset + y1, xOffset + x2, yOffset + y2, resolveColor(color));
    }

    void Renderer::setFont(const uint8_t* font) {
        (void)font;
        // Optional: Implement font setting if your DrawSurface supports it.
    }

    //draw an image to the screen in an bitmap format
    void Renderer::drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, Color color) {
        getDrawSurface().drawBitmap(xOffset + x, yOffset + y, width, height, bitmap, resolveColor(color));
    }

    void Renderer::drawPixel(int x, int y, Color color) {
        getDrawSurface().drawPixel(x, y, resolveColor(color));
    }

    void Renderer::drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX) {
        // Early-out if sprite is invalid.
        if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0) {
            return;
        }

        const int screenW = width;
        const int screenH = height;
        const uint16_t resolvedColor = resolveColor(color);

        // Iterate over rows (Y axis).
        for (int row = 0; row < sprite.height; ++row) {
            const int dstY = y + row;
            if (dstY < 0 || dstY >= screenH) {
                continue;
            }

            const uint16_t bits = sprite.data[row];

            // Iterate over columns (X axis).
            for (int col = 0; col < sprite.width; ++col) {
                const bool bitSet = (bits & (static_cast<uint16_t>(1u) << col)) != 0;
                if (!bitSet) {
                    continue;
                }

                int logicalX = flipX
                    ? x + (sprite.width - 1 - col)
                    : x + col;

                if (logicalX < 0 || logicalX >= screenW) {
                    continue;
                }

                const int dstX = xOffset + logicalX;
                const int finalY = yOffset + dstY;

                getDrawSurface().drawPixel(dstX, finalY, resolvedColor);
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
        // Early-out if sprite is invalid or scale is non-positive.
        if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0 || scaleX <= 0 || scaleY <= 0) {
            return;
        }

        const int screenW = width;
        const int screenH = height;
        const uint16_t resolvedColor = resolveColor(color);

        // Calculate destination dimensions.
        const int dstWidth = static_cast<int>(std::ceil(sprite.width * scaleX));
        const int dstHeight = static_cast<int>(std::ceil(sprite.height * scaleY));

        // Iterate over destination rows (Y axis).
        for (int dstRow = 0; dstRow < dstHeight; ++dstRow) {
            const int logicalY = y + dstRow;
            if (logicalY < 0 || logicalY >= screenH) {
                continue;
            }

            // Map back to source row.
            int srcRow = (dstRow * sprite.height) / dstHeight;
            if (srcRow >= sprite.height) srcRow = sprite.height - 1; // Safety clamp

            const uint16_t bits = sprite.data[srcRow];

            // Iterate over destination columns (X axis).
            for (int dstCol = 0; dstCol < dstWidth; ++dstCol) {
                // Map back to source column.
                int srcCol = (dstCol * sprite.width) / dstWidth;
                if (srcCol >= sprite.width) srcCol = sprite.width - 1; // Safety clamp

                // Handle X flipping on source column index.
                if (flipX) {
                    srcCol = sprite.width - 1 - srcCol;
                }

                // Check bit at source.
                const bool bitSet = (bits & (static_cast<uint16_t>(1u) << srcCol)) != 0;
                
                if (bitSet) {
                    const int logicalX = x + dstCol;
                    
                    // Bounds check X.
                    if (logicalX >= 0 && logicalX < screenW) {
                         getDrawSurface().drawPixel(xOffset + logicalX, yOffset + logicalY, resolvedColor);
                    }
                }
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

        const int screenW = width;
        const int screenH = height;

        for (int ty = 0; ty < map.height; ++ty) {
            int baseY = originY + ty * map.tileHeight;
            if (baseY >= screenH || baseY + map.tileHeight <= 0) {
                continue;
            }

            int rowIndexBase = ty * map.width;

            for (int tx = 0; tx < map.width; ++tx) {
                int baseX = originX + tx * map.tileWidth;
                if (baseX >= screenW || baseX + map.tileWidth <= 0) {
                    continue;
                }

                uint8_t index = map.indices[rowIndexBase + tx];
                if (index >= map.tileCount) {
                    continue;
                }

                const Sprite& tile = map.tiles[index];
                if (tile.data == nullptr) {
                    continue;
                }

                drawSprite(tile, baseX, baseY, color);
            }
        }
    }
}


