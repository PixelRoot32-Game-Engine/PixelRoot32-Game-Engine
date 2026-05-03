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
#include "graphics/TileAnimation.h"
#include "drivers/esp32/TFT_eSPI_Drawer.h"
#include "core/Log.h"
#include <stdarg.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cassert>

#if defined(PIXELROOT32_DEBUG_MODE)
using pixelroot32::core::logging::LogLevel;
using pixelroot32::core::logging::log;
#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace pixelroot32::graphics {

    inline bool isDrawable(Color c) {
        return c != Color::Transparent;
    }

    /// Match TFT_eSprite::drawPixel for 8bpp sprites (TFT_eSPI Extensions/Sprite.cpp).
    inline uint8_t packRgb565ToTftSprite8(uint16_t rgb565) {
        return static_cast<uint8_t>(
            ((rgb565 & 0xE000) >> 8) |
            ((rgb565 & 0x0700) >> 6) |
            ((rgb565 & 0x0018) >> 3));
    }

    Renderer::Renderer(const DisplayConfig& config) 
        : config(config),
          logicalWidth(config.logicalWidth),
          logicalHeight(config.logicalHeight)
    {
        assert(logicalWidth > 0 && "Renderer: logical width must be > 0");
        assert(logicalHeight > 0 && "Renderer: logical height must be > 0");
        
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
        assert(logicalWidth > 0 && "Renderer: logical width must be > 0");
        assert(logicalHeight > 0 && "Renderer: logical height must be > 0");
        
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

        if constexpr (pixelroot32::platforms::config::EnableDirtyRegions) {
            dirtyGrid.init(logicalWidth, logicalHeight);
        }
    }

    void Renderer::ensureDirtyGridSized() {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            return;
        }
        if (dirtyGrid.getCols() == 0 && logicalWidth > 0 && logicalHeight > 0) {
            dirtyGrid.init(logicalWidth, logicalHeight);
        }
    }

    void Renderer::markDirtyLogicalRect(int x, int y, int w, int h) {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            (void)x;
            (void)y;
            (void)w;
            (void)h;
            return;
        }
        if (tilemapSpriteDirtyMode_ == TilemapSpriteDirtyMode::SuppressPerSpriteBoundsMark) {
            return;
        }
        if (w <= 0 || h <= 0) {
            return;
        }
        dirtyGrid.markRect(x, y, w, h);
    }

#if defined(PIXELROOT32_DEBUG_MODE)
    void Renderer::drawDebugDirtyCellOverlay() {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            return;
        }
        if (!debugDirtyCellOverlay_ || dirtyGrid.getCols() == 0) {
            return;
        }
        const uint8_t cols = dirtyGrid.getCols();
        const uint8_t rows = dirtyGrid.getRows();
        const uint16_t outlineCol = resolveColor(Color::Magenta, PaletteContext::Sprite);
        for (uint8_t cy = 0; cy < rows; ++cy) {
            for (uint8_t cx = 0; cx < cols; ++cx) {
                if (!dirtyGrid.isCurrMarked(cx, cy)) {
                    continue;
                }
                const int px = static_cast<int>(cx) * static_cast<int>(DirtyGrid::CELL_W);
                const int py = static_cast<int>(cy) * static_cast<int>(DirtyGrid::CELL_H);
                getDrawSurface().drawRectangle(px, py, DirtyGrid::CELL_W, DirtyGrid::CELL_H, outlineCol);
            }
        }
    }
#else
    void Renderer::drawDebugDirtyCellOverlay() {
    }
#endif

    void Renderer::forceFullRedraw() {
        if constexpr (pixelroot32::platforms::config::EnableDirtyRegions) {
            dirtyGrid.setFullDirty(true);
        }
    }

    void Renderer::resetFramebufferClearSuppressionAdvice() {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            return;
        }
        suppressFramebufferClearBeforeStaticMemcpy_ = false;
    }

    void Renderer::accumulateFramebufferClearSuppressionAdvice(bool skipClearDueToMemcpyRestore) {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            (void)skipClearDueToMemcpyRestore;
            return;
        }
        if (skipClearDueToMemcpyRestore) {
            suppressFramebufferClearBeforeStaticMemcpy_ = true;
        }
    }

    void Renderer::clearDirtyCellsFramebuffer8() {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            return;
        }
        if (logicalFrameBuffer8 == nullptr) {
            return;
        }
        constexpr uint8_t kClear8bpp = 0;  // aligns with TFT_BLACK in 8bpp sprite buffer
        dirtyGrid.clearFramebuffer8FromPrev(logicalFrameBuffer8, logicalWidth, logicalHeight, kClear8bpp);
    }

    void Renderer::beginFrame() {
        logicalFrameBuffer8 = getDrawSurface().getSpriteBuffer();

        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            suppressFramebufferClearBeforeStaticMemcpy_ = false;
            getDrawSurface().clearBuffer();
            return;
        }

        ensureDirtyGridSized();
        dirtyGrid.swapAndClear();
#if PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING && defined(PIXELROOT32_DEBUG_MODE)
        {
            const uint32_t total = dirtyGrid.totalCellCount();
            const uint32_t marked = dirtyGrid.countPrevMarkedCells();
            const float ratio =
                total > 0 ? static_cast<float>(marked) / static_cast<float>(total) : 0.f;
            pixelroot32::core::logging::log(
                pixelroot32::core::logging::LogLevel::Profiling,
                "dirty_ratio=%.4f (%u/%u)",
                ratio,
                static_cast<unsigned>(marked),
                static_cast<unsigned>(total));
        }
#endif

        const bool haveFb8 = (logicalFrameBuffer8 != nullptr);
        const bool skipClearForMemcpy =
            haveFb8 && suppressFramebufferClearBeforeStaticMemcpy_ && !dirtyGrid.isFullDirty();

        suppressFramebufferClearBeforeStaticMemcpy_ = false;

        if (skipClearForMemcpy) {
            // Full framebuffer will be restored from StaticTilemapLayerCache before dynamic draws.
        } else if (haveFb8 && !dirtyGrid.isFullDirty() && (dirtyGrid.countPrevMarkedCells() > 0)) {
            clearDirtyCellsFramebuffer8();
        } else {
            getDrawSurface().clearBuffer();
            if (dirtyGrid.isFullDirty()) {
                dirtyGrid.setFullDirty(false);
            }
        }
    }

    void Renderer::endFrame() {
#if defined(PIXELROOT32_DEBUG_MODE)
        drawDebugDirtyCellOverlay();
#endif
        getDrawSurface().sendBuffer();
    }

    void Renderer::drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size) {
        // Legacy method: delegate to new method with default font
        drawText(text, x, y, color, size, nullptr);
    }

    void Renderer::drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font) {
        if (!isDrawable(color) || text.empty()) {
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
        float scale = static_cast<float>(size);

        for (char c : text) {
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

    void Renderer::drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size) {
        // Legacy method: delegate to new method with default font
        drawTextCentered(text, y, color, size, nullptr);
    }

    void Renderer::drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size, const Font* font) {
        if (!isDrawable(color) || text.empty()) {
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
        markDirtyLogicalRect(finalX - radius, finalY - radius, 2 * radius + 1, 2 * radius + 1);
    }

    void Renderer::drawCircle(int x, int y, int radius, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawCircle(finalX, finalY, radius, resolveColor(color, context));
        markDirtyLogicalRect(finalX - radius, finalY - radius, 2 * radius + 1, 2 * radius + 1);
    }

    void Renderer::drawRectangle(int x, int y, int width, int height, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawRectangle(finalX, finalY, width, height, resolveColor(color, context));
        markDirtyLogicalRect(finalX, finalY, width, height);
    }

    void Renderer::drawFilledRectangle(int x, int y, int width, int height, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawFilledRectangle(finalX, finalY, width, height, resolveColor(color, context));
        markDirtyLogicalRect(finalX, finalY, width, height);
    }

    void Renderer::drawFilledRectangleW(int x, int y, int width, int height, uint16_t color) {
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawFilledRectangle(finalX, finalY, width, height, color);
        markDirtyLogicalRect(finalX, finalY, width, height);
    }

    void Renderer::drawLine(int x1, int y1, int x2, int y2, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX1 = offsetBypass ? x1 : xOffset + x1;
        int finalY1 = offsetBypass ? y1 : yOffset + y1;
        int finalX2 = offsetBypass ? x2 : xOffset + x2;
        int finalY2 = offsetBypass ? y2 : yOffset + y2;
        getDrawSurface().drawLine(finalX1, finalY1, finalX2, finalY2, resolveColor(color, context));
        const int minX = std::min(finalX1, finalX2);
        const int minY = std::min(finalY1, finalY2);
        const int maxX = std::max(finalX1, finalX2);
        const int maxY = std::max(finalY1, finalY2);
        markDirtyLogicalRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
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
        markDirtyLogicalRect(finalX, finalY, width, height);
    }

    void Renderer::drawPixel(int x, int y, Color color) {
        if (!isDrawable(color)) return;
        PaletteContext context = (currentRenderContext != nullptr) ? *currentRenderContext : PaletteContext::Sprite;
        int finalX = offsetBypass ? x : xOffset + x;
        int finalY = offsetBypass ? y : yOffset + y;
        getDrawSurface().drawPixel(finalX, finalY, resolveColor(color, context));
        markDirtyLogicalRect(finalX, finalY, 1, 1);
    }

    void Renderer::drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX) {
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
        markDirtyLogicalRect(startX, startY, sprite.width, sprite.height);
    }

    void Renderer::drawSprite(const Sprite2bpp& sprite, int x, int y, uint8_t paletteSlot, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
            if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0 || sprite.palette == nullptr || sprite.paletteSize == 0) {
                return;
            }

            // Use context slot if active, otherwise use parameter
            uint8_t effectiveSlot = (currentSpritePaletteSlot != kSpritePaletteSlotContextInactive) ? 
                                   currentSpritePaletteSlot : paletteSlot;

            const uint16_t* palettePtr = getSpritePaletteSlot(effectiveSlot);
            
            uint16_t paletteLUT[4];
            uint8_t paletteCount = sprite.paletteSize > 4 ? 4 : sprite.paletteSize;
            for (uint8_t i = 0; i < paletteCount; ++i) {
                paletteLUT[i] = resolveColorWithPalette(sprite.palette[i], palettePtr);
            }

            drawSpriteInternal(sprite, x, y, paletteLUT, flipX);
        }
    }

    // Legacy overload for backward compatibility (3-parameter calls)
    void Renderer::drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX) {
        drawSprite(sprite, x, y, 0, flipX);  // Default to slot 0
    }

    void Renderer::drawSpriteInternal(const Sprite2bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
            const int screenW = logicalWidth;
            const int screenH = logicalHeight;
            const int bitsPerPixel = 2;
            const int rowStrideBytes = (sprite.width * bitsPerPixel + 7) / 8;

            int startX = offsetBypass ? x : xOffset + x;
            int startY = offsetBypass ? y : yOffset + y;

            uint8_t* const fb8 = logicalFrameBuffer8;

            // Data: 16-bit words (8 pixels per word). Compiler pack_2bpp: LSB = left pixel (bitOffset = (col&7)<<1), word order [left, right]
            for (int row = 0; row < sprite.height; ++row) {
                const int logicalY = startY + row;
                if (logicalY < 0 || logicalY >= screenH) continue;

                const uint16_t* rowWords = reinterpret_cast<const uint16_t*>(sprite.data + row * rowStrideBytes);
                uint8_t* dstRow = fb8 ? (fb8 + logicalY * screenW) : nullptr;

                for (int col = 0; col < sprite.width; ++col) {
                    const int wordIdx = col >> 3; // 8 pixels per word; word 0 = left half, word 1 = right half
                    const int bitOffset = (col & 7) << 1; // LSB = pixel 0 (match compiler pack_2bpp)
                    const uint8_t val = (rowWords[wordIdx] >> bitOffset) & 0x03;

                    if (val == 0) continue;

                    const int logicalX = flipX ? startX + (sprite.width - 1 - col) : startX + col;
                    if (logicalX < 0 || logicalX >= screenW) continue;

                    if (dstRow) {
                        dstRow[logicalX] = packRgb565ToTftSprite8(paletteLUT[val]);
                    } else {
                        getDrawSurface().drawPixel(logicalX, logicalY, paletteLUT[val]);
                    }
                }
            }
            markDirtyLogicalRect(startX, startY, sprite.width, sprite.height);
        }
    }

    void Renderer::drawSprite(const Sprite4bpp& sprite, int x, int y, uint8_t paletteSlot, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
            if (sprite.data == nullptr || sprite.width == 0 || sprite.height == 0 || sprite.palette == nullptr || sprite.paletteSize == 0) {
                return;
            }

            // Use context slot if active, otherwise use parameter
            uint8_t effectiveSlot = (currentSpritePaletteSlot != kSpritePaletteSlotContextInactive) ? 
                                   currentSpritePaletteSlot : paletteSlot;

            const uint16_t* palettePtr = getSpritePaletteSlot(effectiveSlot);
            
            uint16_t paletteLUT[16];
            uint8_t paletteCount = sprite.paletteSize > 16 ? 16 : sprite.paletteSize;
            for (uint8_t i = 0; i < paletteCount; ++i) {
                paletteLUT[i] = resolveColorWithPalette(sprite.palette[i], palettePtr);
            }

            drawSpriteInternal(sprite, x, y, paletteLUT, flipX);
        }
    }

    // Legacy overload for backward compatibility (3-parameter calls)
    void Renderer::drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX) {
        drawSprite(sprite, x, y, 0, flipX);  // Default to slot 0
    }

    void IRAM_ATTR Renderer::drawSpriteInternal(const Sprite4bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX) {
        if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
            const int screenW = logicalWidth;
            const int screenH = logicalHeight;
            const int rowStrideBytes = (sprite.width * 4 + 7) / 8;

            int startX = offsetBypass ? x : xOffset + x;
            int startY = offsetBypass ? y : yOffset + y;

            uint8_t* const fb8 = logicalFrameBuffer8;

            for (int row = 0; row < sprite.height; ++row) {
                const int logicalY = startY + row;
                if (logicalY < 0 || logicalY >= screenH) continue;

                const uint8_t* rowData = sprite.data + row * rowStrideBytes;

                if (fb8 && !flipX) {
                    uint8_t* dstRow = fb8 + logicalY * screenW;
                    int col = 0;
                    for (; col + 1 < sprite.width; col += 2) {
                        const uint8_t b = rowData[col >> 1];
                        const uint8_t v0 = b & 0x0F;
                        const uint8_t v1 = (b >> 4) & 0x0F;
                        const int lx0 = startX + col;
                        const int lx1 = startX + col + 1;
                        if (v0 != 0 && lx0 >= 0 && lx0 < screenW) {
                            dstRow[lx0] = packRgb565ToTftSprite8(paletteLUT[v0]);
                        }
                        if (v1 != 0 && lx1 >= 0 && lx1 < screenW) {
                            dstRow[lx1] = packRgb565ToTftSprite8(paletteLUT[v1]);
                        }
                    }
                    if (col < sprite.width) {
                        const int byteIdx = col >> 1;
                        const int bitOffset = (col & 1) << 2;
                        const uint8_t val = (rowData[byteIdx] >> bitOffset) & 0x0F;
                        if (val != 0) {
                            const int lx = startX + col;
                            if (lx >= 0 && lx < screenW) {
                                dstRow[lx] = packRgb565ToTftSprite8(paletteLUT[val]);
                            }
                        }
                    }
                } else if (fb8) {
                    uint8_t* dstRow = fb8 + logicalY * screenW;
                    for (int col = 0; col < sprite.width; ++col) {
                        const int byteIdx = col >> 1;
                        const int bitOffset = (col & 1) << 2;
                        const uint8_t val = (rowData[byteIdx] >> bitOffset) & 0x0F;
                        if (val == 0) continue;
                        const int logicalX = startX + (sprite.width - 1 - col);
                        if (logicalX < 0 || logicalX >= screenW) continue;
                        dstRow[logicalX] = packRgb565ToTftSprite8(paletteLUT[val]);
                    }
                } else {
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
            markDirtyLogicalRect(startX, startY, sprite.width, sprite.height);
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
        markDirtyLogicalRect(startX, startY, dstWidth, dstHeight);
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

    void Renderer::drawTileMap(const TileMap& map,
                                 int originX,
                                 int originY,
                                 Color color,
                                 LayerType layerType) {
        if (map.indices == nullptr || map.tiles == nullptr ||
            map.width == 0 || map.height == 0 ||
            map.tileWidth == 0 || map.tileHeight == 0 ||
            map.tileCount == 0) {
            return;
        }

        PaletteContext bgContext = PaletteContext::Background;
        PaletteContext* oldContext = currentRenderContext;
        setRenderContext(&bgContext);

        int viewOriginX = offsetBypass ? originX : xOffset + originX;
        int viewOriginY = offsetBypass ? originY : yOffset + originY;

        const bool selectiveAnimMarks =
            (layerType == LayerType::Dynamic && map.animManager != nullptr);

        const TilemapSpriteDirtyMode savedMode = tilemapSpriteDirtyMode_;

        if (layerType == LayerType::Static || selectiveAnimMarks) {
            tilemapSpriteDirtyMode_ = TilemapSpriteDirtyMode::SuppressPerSpriteBoundsMark;
        } else {
            tilemapSpriteDirtyMode_ = TilemapSpriteDirtyMode::Normal;
        }

        const void* const savedAnimKey   = animDynTrackMapKey_;
        const int       savedAnimOx     = animDynTrackOx_;
        const int       savedAnimOy     = animDynTrackOy_;
        const bool      savedAnimPrimed = animDynTrackPrimed_;

        const bool mapOrOriginMovedAnim = selectiveAnimMarks &&
            (!animDynTrackPrimed_ || animDynTrackMapKey_ != static_cast<const void*>(map.indices) ||
             animDynTrackOx_ != viewOriginX || animDynTrackOy_ != viewOriginY);

        int startCol = (viewOriginX < 0) ? (-viewOriginX / map.tileWidth) : 0;
        int endCol   = (viewOriginX + map.width * map.tileWidth > logicalWidth)
                           ? ((logicalWidth - viewOriginX + map.tileWidth - 1) / map.tileWidth)
                           : map.width;

        int startRow = (viewOriginY < 0) ? (-viewOriginY / map.tileHeight) : 0;
        int endRow   = (viewOriginY + map.height * map.tileHeight > logicalHeight)
                           ? ((logicalHeight - viewOriginY + map.tileHeight - 1) / map.tileHeight)
                           : map.height;

        if (startCol < 0) startCol = 0;
        if (endCol > map.width) endCol = map.width;
        if (startRow < 0) startRow = 0;
        if (endRow > map.height) endRow = map.height;

        for (int ty = startRow; ty < endRow; ++ty) {
            int baseY        = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = startCol; tx < endCol; ++tx) {
                int       baseX    = originX + tx * map.tileWidth;
                uint8_t   rawIndex = map.indices[rowIndexBase + tx];
                uint8_t   index    = rawIndex;

                if (map.animManager) {
                    index = map.animManager->resolveFrame(rawIndex);
                }

                if (index == 0 || index >= map.tileCount) {
                    continue;
                }

                if (map.runtimeMask) {
                    int tileIndex = rowIndexBase + tx;
                    if (!(map.runtimeMask[tileIndex >> 3] & (1 << (tileIndex & 7)))) {
                        continue;
                    }
                }

                if (layerType == LayerType::Dynamic) {
                    bool markCell = true;
                    if (map.animManager != nullptr) {
                        markCell = mapOrOriginMovedAnim ||
                                   map.animManager->animatedTileAppearanceChanged(rawIndex);
                    }
                    if (markCell) {
                        if constexpr (pixelroot32::platforms::config::EnableDirtyRegions) {
                            const int pixelX = offsetBypass ? baseX : xOffset + baseX;
                            const int pixelY = offsetBypass ? baseY : yOffset + baseY;
                            dirtyGrid.markRect(pixelX, pixelY, map.tileWidth, map.tileHeight);
                        }
                    }
                }

                drawSprite(map.tiles[index], baseX, baseY, color, false);
            }
        }

        if (selectiveAnimMarks) {
            animDynTrackPrimed_ = true;
            animDynTrackMapKey_ = map.indices;
            animDynTrackOx_     = viewOriginX;
            animDynTrackOy_     = viewOriginY;
        } else {
            animDynTrackMapKey_   = savedAnimKey;
            animDynTrackOx_       = savedAnimOx;
            animDynTrackOy_       = savedAnimOy;
            animDynTrackPrimed_  = savedAnimPrimed;
        }

        tilemapSpriteDirtyMode_ = savedMode;
        setRenderContext(oldContext);
    }

    void Renderer::drawTileMap(const TileMap2bpp& map, int originX, int originY, LayerType layerType) {
        if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
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

        const bool selectiveAnimMarks =
            (layerType == LayerType::Dynamic && map.animManager != nullptr);

        const TilemapSpriteDirtyMode savedMode = tilemapSpriteDirtyMode_;

        if (layerType == LayerType::Static || selectiveAnimMarks) {
            tilemapSpriteDirtyMode_ = TilemapSpriteDirtyMode::SuppressPerSpriteBoundsMark;
        } else {
            tilemapSpriteDirtyMode_ = TilemapSpriteDirtyMode::Normal;
        }

        const void* const savedAnimKey   = animDynTrackMapKey_;
        const int       savedAnimOx     = animDynTrackOx_;
        const int       savedAnimOy     = animDynTrackOy_;
        const bool      savedAnimPrimed = animDynTrackPrimed_;

        const bool mapOrOriginMovedAnim = selectiveAnimMarks &&
            (!animDynTrackPrimed_ || animDynTrackMapKey_ != static_cast<const void*>(map.indices) ||
             animDynTrackOx_ != viewOriginX || animDynTrackOy_ != viewOriginY);

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

        // Palette Caching (tile palette + background palette slot)
        uint16_t cachedLUT[4];
        const Color* lastTilePalettePtr = nullptr;
        const uint16_t* lastBackgroundPalettePtr = nullptr;

        for (int ty = startRow; ty < endRow; ++ty) {
            int baseY = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = startCol; tx < endCol; ++tx) {
                int baseX = originX + tx * map.tileWidth;
                int cellIndex = rowIndexBase + tx;
                uint8_t rawIndex = map.indices[cellIndex];
                uint8_t index   = rawIndex;

                if (map.animManager) {
                    index = map.animManager->resolveFrame(rawIndex);
                }

                if (index == 0 || index >= map.tileCount) {
                    continue;
                }

                if (map.runtimeMask) {
                    if (!(map.runtimeMask[cellIndex >> 3] & (1 << (cellIndex & 7)))) {
                        continue;
                    }
                }

                if (layerType == LayerType::Dynamic) {
                    bool markCell = true;
                    if (map.animManager != nullptr) {
                        markCell = mapOrOriginMovedAnim ||
                                   map.animManager->animatedTileAppearanceChanged(rawIndex);
                    }
                    if (markCell) {
                        if constexpr (pixelroot32::platforms::config::EnableDirtyRegions) {
                            const int pixelX = offsetBypass ? baseX : xOffset + baseX;
                            const int pixelY = offsetBypass ? baseY : yOffset + baseY;
                            dirtyGrid.markRect(pixelX, pixelY, map.tileWidth, map.tileHeight);
                        }
                    }
                }

                const Sprite2bpp& tile = map.tiles[index];

                // Per-cell background palette: use paletteIndices if present, else slot 0
                const uint16_t* palettePtr = (map.paletteIndices != nullptr)
                    ? getBackgroundPaletteSlot(map.paletteIndices[cellIndex] & kTileCellPaletteMask)
                    : getBackgroundPaletteSlot(0);
                
                // Rebuild LUT only when tile palette or background palette slot changes
                uint8_t paletteCount = 0;
                if (tile.palette != lastTilePalettePtr || palettePtr != lastBackgroundPalettePtr) {
                    paletteCount = tile.paletteSize > 4 ? 4 : tile.paletteSize;
                    for (uint8_t i = 0; i < paletteCount; ++i) {
                        cachedLUT[i] = resolveColorWithPalette(tile.palette[i], palettePtr);
                    }
                    lastTilePalettePtr = tile.palette;
                    lastBackgroundPalettePtr = palettePtr;
                }

                // Use original path - drawSpriteInternal handles custom palettes correctly
                drawSpriteInternal(tile, baseX, baseY, cachedLUT, false);
            }
        }

        if (selectiveAnimMarks) {
            animDynTrackPrimed_ = true;
            animDynTrackMapKey_ = map.indices;
            animDynTrackOx_     = viewOriginX;
            animDynTrackOy_     = viewOriginY;
        } else {
            animDynTrackMapKey_  = savedAnimKey;
            animDynTrackOx_      = savedAnimOx;
            animDynTrackOy_      = savedAnimOy;
            animDynTrackPrimed_  = savedAnimPrimed;
        }

        tilemapSpriteDirtyMode_ = savedMode;

        setRenderContext(oldContext);
        }
    }

    void Renderer::drawTileMap(const TileMap4bpp& map, int originX, int originY, LayerType layerType) {
        if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
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

            const bool selectiveAnimMarks =
                (layerType == LayerType::Dynamic && map.animManager != nullptr);

            const TilemapSpriteDirtyMode savedMode = tilemapSpriteDirtyMode_;

            if (layerType == LayerType::Static || selectiveAnimMarks) {
                tilemapSpriteDirtyMode_ = TilemapSpriteDirtyMode::SuppressPerSpriteBoundsMark;
            } else {
                tilemapSpriteDirtyMode_ = TilemapSpriteDirtyMode::Normal;
            }

            const void* const savedAnimKey   = animDynTrackMapKey_;
            const int       savedAnimOx     = animDynTrackOx_;
            const int       savedAnimOy     = animDynTrackOy_;
            const bool      savedAnimPrimed = animDynTrackPrimed_;

            const bool mapOrOriginMovedAnim = selectiveAnimMarks &&
                (!animDynTrackPrimed_ || animDynTrackMapKey_ != static_cast<const void*>(map.indices) ||
                 animDynTrackOx_ != viewOriginX || animDynTrackOy_ != viewOriginY);

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

            // Palette Caching (tile palette + background palette slot)
            uint16_t cachedLUT[16];
            const Color* lastTilePalettePtr = nullptr;
            const uint16_t* lastBackgroundPalettePtr = nullptr;

            for (int ty = startRow; ty < endRow; ++ty) {
                int baseY = originY + ty * map.tileHeight;
                int rowIndexBase = ty * map.width;

                for (int tx = startCol; tx < endCol; ++tx) {
                    int baseX = originX + tx * map.tileWidth;
                    int cellIndex = rowIndexBase + tx;
                    uint8_t rawIndex = map.indices[cellIndex];
                    uint8_t index    = rawIndex;

                    if (map.animManager) {
                        index = map.animManager->resolveFrame(rawIndex);
                    }

                    if (index == 0 || index >= map.tileCount) {
                        continue;
                    }

                    if (map.runtimeMask) {
                        if (!(map.runtimeMask[cellIndex >> 3] & (1 << (cellIndex & 7)))) {
                            continue;
                        }
                    }

                    if (layerType == LayerType::Dynamic) {
                        bool markCell = true;
                        if (map.animManager != nullptr) {
                            markCell = mapOrOriginMovedAnim ||
                                       map.animManager->animatedTileAppearanceChanged(rawIndex);
                        }
                        if (markCell) {
                            if constexpr (pixelroot32::platforms::config::EnableDirtyRegions) {
                                const int pixelX = offsetBypass ? baseX : xOffset + baseX;
                                const int pixelY = offsetBypass ? baseY : yOffset + baseY;
                                dirtyGrid.markRect(pixelX, pixelY, map.tileWidth, map.tileHeight);
                            }
                        }
                    }

                    const Sprite4bpp& tile = map.tiles[index];

                    // Per-cell background palette: use paletteIndices if present, else slot 0
                    const uint16_t* palettePtr = (map.paletteIndices != nullptr)
                        ? getBackgroundPaletteSlot(map.paletteIndices[cellIndex] & kTileCellPaletteMask)
                        : getBackgroundPaletteSlot(0);
                    
                    // Rebuild LUT only when tile palette or background palette slot changes
                    if (tile.palette != lastTilePalettePtr || palettePtr != lastBackgroundPalettePtr) {
                        uint8_t paletteCount = tile.paletteSize > 16 ? 16 : tile.paletteSize;
                        for (uint8_t i = 0; i < paletteCount; ++i) {
                            cachedLUT[i] = resolveColorWithPalette(tile.palette[i], palettePtr);
                        }
                        lastTilePalettePtr = tile.palette;
                        lastBackgroundPalettePtr = palettePtr;
                    }

                    // Use drawSpriteInternal - handles custom palettes correctly
                    drawSpriteInternal(tile, baseX, baseY, cachedLUT, false);
                }
            }

            if (selectiveAnimMarks) {
                animDynTrackPrimed_ = true;
                animDynTrackMapKey_ = map.indices;
                animDynTrackOx_     = viewOriginX;
                animDynTrackOy_     = viewOriginY;
            } else {
                animDynTrackMapKey_  = savedAnimKey;
                animDynTrackOx_      = savedAnimOx;
                animDynTrackOy_      = savedAnimOy;
                animDynTrackPrimed_  = savedAnimPrimed;
            }

            tilemapSpriteDirtyMode_ = savedMode;

            setRenderContext(oldContext);
        }
    }

    void Renderer::setSpritePaletteSlotContext(uint8_t slot) {
        currentSpritePaletteSlot = slot;
    }

    uint8_t Renderer::getSpritePaletteSlotContext() const {
        return currentSpritePaletteSlot;
    }

}
