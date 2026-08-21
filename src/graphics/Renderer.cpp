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
            if (!dirtyGrid.init(logicalWidth, logicalHeight)) {
#if defined(PIXELROOT32_DEBUG_MODE)
                log(LogLevel::Error, "DirtyGrid::init allocation failed (%dx%d)", logicalWidth, logicalHeight);
#endif
            }
        }
    }

    void Renderer::ensureDirtyGridSized() {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            return;
        }
        if (dirtyGrid.getCols() == 0 && logicalWidth > 0 && logicalHeight > 0) {
            (void)dirtyGrid.init(logicalWidth, logicalHeight);
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

    Renderer::AnimDynTrackEntry* Renderer::findOrAllocAnimSlot(const void* mapKey) {
        // First pass: find existing slot for this map
        for (uint8_t i = 0; i < kMaxAnimDynTrackSlots; ++i) {
            if (animDynTrackSlots_[i].mapKey == mapKey) {
                return &animDynTrackSlots_[i];
            }
        }
        // Second pass: find empty slot
        for (uint8_t i = 0; i < kMaxAnimDynTrackSlots; ++i) {
            if (animDynTrackSlots_[i].mapKey == nullptr) {
                return &animDynTrackSlots_[i];
            }
        }
        // All slots full: evict oldest (slot 0), shift down, return last
        for (uint8_t i = 1; i < kMaxAnimDynTrackSlots; ++i) {
            animDynTrackSlots_[i - 1] = animDynTrackSlots_[i];
        }
        animDynTrackSlots_[kMaxAnimDynTrackSlots - 1] = {};
        return &animDynTrackSlots_[kMaxAnimDynTrackSlots - 1];
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

        // If fullDirty is set, highlight all cells
        if (dirtyGrid.isFullDirty()) {
            for (uint8_t cy = 0; cy < rows; ++cy) {
                for (uint8_t cx = 0; cx < cols; ++cx) {
                    const int px = static_cast<int>(cx) * static_cast<int>(DirtyGrid::CELL_W);
                    const int py = static_cast<int>(cy) * static_cast<int>(DirtyGrid::CELL_H);
                    getDrawSurface().drawRectangle(px, py, DirtyGrid::CELL_W, DirtyGrid::CELL_H, outlineCol);
                }
            }
            return;
        }

        // Otherwise, highlight only marked cells
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

    bool Renderer::restoreDirtyCellsFromSnapshot(const uint8_t* snapshot) {
        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            (void)snapshot;
            return false;
        }
        if (snapshot == nullptr || logicalFrameBuffer8 == nullptr) {
            return false;
        }
        if (!selectiveRestoreValidThisFrame_) {
            // beginFrame() wiped the whole framebuffer this frame, so the cells
            // outside prev-dirty no longer hold the static layers and repainting
            // only prev-dirty would leave them black.
            return false;
        }
        dirtyGrid.restoreFramebuffer8FromPrev(logicalFrameBuffer8, snapshot,
                                              logicalWidth, logicalHeight);
        return true;
    }

    void Renderer::beginFrame() {
        logicalFrameBuffer8 = getDrawSurface().getSpriteBuffer();

        // Whatever path this frame takes below, assume it wipes everything until
        // proven otherwise. Only the two selective outcomes set this true.
        selectiveRestoreValidThisFrame_ = false;

        if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
            suppressFramebufferClearBeforeStaticMemcpy_ = false;
            getDrawSurface().clearBuffer();
            return;
        }

        // Skip dirty grid operations entirely for non-8bpp drivers (e.g., SDL2/native).
        // Dirty regions provide no selective clear benefit without an 8bpp framebuffer.
        const bool haveFb8 = (logicalFrameBuffer8 != nullptr);
        if (!haveFb8) {
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

        const bool skipClearForMemcpy =
            suppressFramebufferClearBeforeStaticMemcpy_ && !dirtyGrid.isFullDirty();

        suppressFramebufferClearBeforeStaticMemcpy_ = false;

        if (skipClearForMemcpy) {
            // Full framebuffer will be restored from StaticTilemapLayerCache before dynamic draws.
            // Nothing was touched, so pixels outside prev-dirty are still last frame's.
            selectiveRestoreValidThisFrame_ = true;
        } else if (!dirtyGrid.isFullDirty() && (dirtyGrid.countPrevMarkedCells() > 0)) {
            clearDirtyCellsFramebuffer8();
            // Only prev-dirty cells were blanked; everything else survived.
            selectiveRestoreValidThisFrame_ = true;
        } else {
            getDrawSurface().clearBuffer();
            if (dirtyGrid.isFullDirty()) {
                dirtyGrid.setFullDirty(false);
            }
            // The whole buffer is gone: a per-cell restore would leave the
            // static layers black outside prev-dirty. selectiveRestoreValidThisFrame_
            // stays false so StaticLayerSnapshot falls back to a full copy.
        }
    }

    void Renderer::endFrame() {
#if defined(PIXELROOT32_DEBUG_MODE)
        drawDebugDirtyCellOverlay();
#endif
        // Snapshot the camera offset for the projected-tilemap dirty-skip
        // gate. Taken here — after beginFrame()'s swapAndClear() and before
        // sendBuffer() — so prevXOffset_/prevYOffset_ match the offset used
        // for this frame's prev-buffer dirty marks.
        prevXOffset_ = xOffset;
        prevYOffset_ = yOffset;
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

        // A 1bpp sprite is drawn with a single colour, so the 8bpp packing is hoisted
        // out of both loops. This is the main win over the 4bpp path, which has to
        // pack per pixel because every pixel may pick a different palette entry.
        const uint8_t packedColor = packRgb565ToTftSprite8(resolvedColor);

        int startX = offsetBypass ? x : xOffset + x;
        int startY = offsetBypass ? y : yOffset + y;

        uint8_t* const fb8 = logicalFrameBuffer8;

        // Mask of the leftmost column; shifting it right walks bits MSB -> LSB,
        // i.e. bit (width-1) = leftmost pixel, bit 0 = rightmost pixel.
        const uint16_t firstColMask = static_cast<uint16_t>(static_cast<uint16_t>(1u) << (sprite.width - 1));

        for (int row = 0; row < sprite.height; ++row) {
            const int logicalY = startY + row;
            // Note: clipping against logicalWidth/Height might be tricky if xOffset is applied,
            // but the Driver should handle physical clipping. Logical clipping here is for efficiency.
            if (logicalY < 0 || logicalY >= screenH) {
                continue;
            }

            const uint16_t bits = sprite.data[row];
            // Empty rows are very common in glyphs and 1bpp tiles; skip the column scan.
            if (bits == 0) {
                continue;
            }

            if (fb8 && !flipX) {
                uint8_t* dstRow = fb8 + logicalY * screenW;
                uint16_t mask = firstColMask;
                for (int col = 0; col < sprite.width; ++col, mask >>= 1) {
                    if ((bits & mask) == 0) continue;
                    const int lx = startX + col;
                    if (lx < 0 || lx >= screenW) continue;
                    dstRow[lx] = packedColor;
                }
            } else if (fb8) {
                uint8_t* dstRow = fb8 + logicalY * screenW;
                uint16_t mask = firstColMask;
                for (int col = 0; col < sprite.width; ++col, mask >>= 1) {
                    if ((bits & mask) == 0) continue;
                    const int lx = startX + (sprite.width - 1 - col);
                    if (lx < 0 || lx >= screenW) continue;
                    dstRow[lx] = packedColor;
                }
            } else {
                // Fallback for surfaces without an 8bpp framebuffer (U8G2/SDL/mock).
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
            
            // All 4 entries are filled, not just the sprite's paletteSize: see
            // the 4bpp overload below for why the tail cannot be left
            // uninitialised.
            uint16_t paletteLUT[4];
            uint8_t paletteCount = sprite.paletteSize > 4 ? 4 : sprite.paletteSize;
            for (uint8_t i = 0; i < paletteCount; ++i) {
                paletteLUT[i] = resolveColorWithPalette(sprite.palette[i], palettePtr);
            }
            for (uint8_t i = paletteCount; i < 4; ++i) {
                paletteLUT[i] = 0;
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

            // Pack the palette once per sprite instead of once per pixel; see
            // the 4bpp blit for the full rationale. Four entries here.
            uint8_t packedLUT[4];
            if (fb8) {
                for (int i = 0; i < 4; ++i) {
                    packedLUT[i] = packRgb565ToTftSprite8(paletteLUT[i]);
                }
            }

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
                        dstRow[logicalX] = packedLUT[val];
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

            // All 16 entries are filled, not just the sprite's paletteSize: a
            // 4bpp pixel can name any index 0..15 regardless of what the
            // descriptor declares, and drawSpriteInternal reads the table
            // without a range check (see its packing loop). Leaving the tail
            // uninitialised put a stack value on screen for such a pixel.
            // Black matches what resolveColorWithPalette returns for an index
            // it cannot map.
            uint16_t paletteLUT[16];
            uint8_t paletteCount = sprite.paletteSize > 16 ? 16 : sprite.paletteSize;
            for (uint8_t i = 0; i < paletteCount; ++i) {
                paletteLUT[i] = resolveColorWithPalette(sprite.palette[i], palettePtr);
            }
            for (uint8_t i = paletteCount; i < 16; ++i) {
                paletteLUT[i] = 0;
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

            // Pack the palette ONCE per sprite rather than once per pixel.
            //
            // The framebuffer stores TFT_eSprite 8bpp, so every written pixel
            // needs its RGB565 narrowed to RRRGGGBB. Doing that inside the
            // pixel loop repeats the same 16 conversions thousands of times: a
            // 7x7 isometric room pushes ~37k source pixels through here per
            // frame against 16 distinct colours. Hoisting it trades ~37k
            // conversions for 16 and turns the inner loop into a table read.
            //
            // Only the direct-framebuffer paths use this; the drawPixel
            // fallback below takes RGB565 and keeps reading paletteLUT.
            uint8_t packedLUT[16];
            if (fb8) {
                for (int i = 0; i < 16; ++i) {
                    packedLUT[i] = packRgb565ToTftSprite8(paletteLUT[i]);
                }
            }

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
                            dstRow[lx0] = packedLUT[v0];
                        }
                        if (v1 != 0 && lx1 >= 0 && lx1 < screenW) {
                            dstRow[lx1] = packedLUT[v1];
                        }
                    }
                    if (col < sprite.width) {
                        const int byteIdx = col >> 1;
                        const int bitOffset = (col & 1) << 2;
                        const uint8_t val = (rowData[byteIdx] >> bitOffset) & 0x0F;
                        if (val != 0) {
                            const int lx = startX + col;
                            if (lx >= 0 && lx < screenW) {
                                dstRow[lx] = packedLUT[val];
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
                        dstRow[logicalX] = packedLUT[val];
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

        auto h = computeTilemapDirtyTracking(map, originX, originY, layerType);

        for (int ty = h.startRow; ty < h.endRow; ++ty) {
            int baseY        = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = h.startCol; tx < h.endCol; ++tx) {
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
                        markCell = h.mapOrOriginMovedAnim ||
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

        if (h.animSlot) {
            h.animSlot->primed = true;
            h.animSlot->mapKey = map.indices;
            h.animSlot->ox     = h.viewOriginX;
            h.animSlot->oy     = h.viewOriginY;
        }

        tilemapSpriteDirtyMode_ = h.savedMode;
        setRenderContext(h.oldContext);
    }

    void Renderer::drawTileMap(const TileMap2bpp& map, int originX, int originY, LayerType layerType) {
        if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
        if (map.indices == nullptr || map.tiles == nullptr ||
            map.width == 0 || map.height == 0 ||
            map.tileWidth == 0 || map.tileHeight == 0 ||
            map.tileCount == 0) {
            return;
        }

        auto h = computeTilemapDirtyTracking(map, originX, originY, layerType);

        // Palette Caching (tile palette + background palette slot)
        uint16_t cachedLUT[4];
        const Color* lastTilePalettePtr = nullptr;
        const uint16_t* lastBackgroundPalettePtr = nullptr;

        for (int ty = h.startRow; ty < h.endRow; ++ty) {
            int baseY = originY + ty * map.tileHeight;
            int rowIndexBase = ty * map.width;

            for (int tx = h.startCol; tx < h.endCol; ++tx) {
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
                        markCell = h.mapOrOriginMovedAnim ||
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
                    // Fill the tail: drawSpriteInternal indexes all 4 slots.
                    for (uint8_t i = paletteCount; i < 4; ++i) {
                        cachedLUT[i] = 0;
                    }
                    lastTilePalettePtr = tile.palette;
                    lastBackgroundPalettePtr = palettePtr;
                }

                // Use original path - drawSpriteInternal handles custom palettes correctly
                drawSpriteInternal(tile, baseX, baseY, cachedLUT, false);
            }
        }

        if (h.animSlot) {
            h.animSlot->primed = true;
            h.animSlot->mapKey = map.indices;
            h.animSlot->ox     = h.viewOriginX;
            h.animSlot->oy     = h.viewOriginY;
        }

        tilemapSpriteDirtyMode_ = h.savedMode;

        setRenderContext(h.oldContext);
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

            auto h = computeTilemapDirtyTracking(map, originX, originY, layerType);

            // Palette Caching (tile palette + background palette slot)
            uint16_t cachedLUT[16];
            const Color* lastTilePalettePtr = nullptr;
            const uint16_t* lastBackgroundPalettePtr = nullptr;

            for (int ty = h.startRow; ty < h.endRow; ++ty) {
                int baseY = originY + ty * map.tileHeight;
                int rowIndexBase = ty * map.width;

                for (int tx = h.startCol; tx < h.endCol; ++tx) {
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
                            markCell = h.mapOrOriginMovedAnim ||
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
                        // Fill the tail for the same reason drawSprite() does:
                        // drawSpriteInternal indexes all 16 slots.
                        for (uint8_t i = paletteCount; i < 16; ++i) {
                            cachedLUT[i] = 0;
                        }
                        lastTilePalettePtr = tile.palette;
                        lastBackgroundPalettePtr = palettePtr;
                    }

                    // Use drawSpriteInternal - handles custom palettes correctly
                    drawSpriteInternal(tile, baseX, baseY, cachedLUT, false);
                }
            }

            if (h.animSlot) {
                h.animSlot->primed = true;
                h.animSlot->mapKey = map.indices;
                h.animSlot->ox     = h.viewOriginX;
                h.animSlot->oy     = h.viewOriginY;
            }

            tilemapSpriteDirtyMode_ = h.savedMode;

            setRenderContext(h.oldContext);
        }
    }

#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION
    template<typename TileT>
    void Renderer::drawTileMapProjectedImpl(const TileMapGeneric<TileT>& map,
                                             int originX,
                                             int originY,
                                             LayerType layerType,
                                             const pixelroot32::math::ProjectionSpec& projection,
                                             Color color) {
        if (map.indices == nullptr || map.tiles == nullptr ||
            map.width == 0 || map.height == 0 ||
            map.tileWidth == 0 || map.tileHeight == 0 ||
            map.tileCount == 0) {
            return;
        }

        auto h = computeTilemapDirtyTracking(map, originX, originY, layerType);

        pixelroot32::math::ProjectionSpec drawSpec = projection;
        drawSpec.originX += originX;
        drawSpec.originY += originY;

        pixelroot32::math::ProjectionSpec cullSpec = projection;
        cullSpec.originX += h.viewOriginX;
        cullSpec.originY += h.viewOriginY;

        // Bounded per-call scan (see decision 5 in the design record):
        // worst case is the largest tileCount shipped anywhere in this
        // repo (46, examples/animated_tilemap), so this is ~46 flash
        // reads per draw call, per layer, per frame -- not per tile,
        // and it does not grow with map size.
        uint8_t maxWidth = 0;
        uint8_t maxFootY = 0;
        uint8_t maxBelow = 0;
        for (uint16_t i = 0; i < map.tileCount; ++i) {
            const TileT& scanTile = map.tiles[i];
            if (scanTile.width > maxWidth) {
                maxWidth = scanTile.width;
            }
            const uint8_t footY = map.footYFor(i);
            if (footY > maxFootY) {
                maxFootY = footY;
            }
            const uint8_t below =
                (scanTile.height > footY) ? static_cast<uint8_t>(scanTile.height - footY) : 0;
            if (below > maxBelow) {
                maxBelow = below;
            }
        }

        // Pad the cull rect by the tileset's own worst-case sprite
        // extent so a tile whose CELL sits outside the window but
        // whose SPRITE still overlaps it is not culled away (AC-5).
        const pixelroot32::math::CellRange range = pixelroot32::math::cellRangeForScreenRect(
            cullSpec,
            -static_cast<int>(maxWidth), -static_cast<int>(maxBelow),
            logicalWidth + 2 * static_cast<int>(maxWidth),
            logicalHeight + static_cast<int>(maxBelow) + static_cast<int>(maxFootY),
            map.width, map.height);
        h.startCol = range.startCol;
        h.endCol   = range.endCol;
        h.startRow = range.startRow;
        h.endRow   = range.endRow;

        // Palette Caching (tile palette + background palette slot). Sized for
        // the widest per-format LUT this function currently serves (4bpp,
        // 16 entries); the format-specific tail below is what actually reads
        // and writes it.
        uint16_t cachedLUT[16];
        const Color* lastTilePalettePtr = nullptr;
        const uint16_t* lastBackgroundPalettePtr = nullptr;

        for (int ty = h.startRow; ty < h.endRow; ++ty) {
            int rowIndexBase = ty * map.width;

            for (int tx = h.startCol; tx < h.endCol; ++tx) {
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

                const TileT& tile = map.tiles[index];
                const int centreX = pixelroot32::math::cellToScreenX(tx, ty, drawSpec);
                const int centreY = pixelroot32::math::cellToScreenY(tx, ty, drawSpec);
                const int drawX = centreX - tile.width / 2;
                const int drawY = centreY - map.footYFor(index);

                if (layerType == LayerType::Dynamic) {
                    bool markCell = true;
                    if (map.animManager != nullptr) {
                        markCell = h.mapOrOriginMovedAnim ||
                                   map.animManager->animatedTileAppearanceChanged(rawIndex);
                    }
                    if (markCell) {
                        if constexpr (pixelroot32::platforms::config::EnableDirtyRegions) {
                            const int pixelX = offsetBypass ? drawX : xOffset + drawX;
                            const int pixelY = offsetBypass ? drawY : yOffset + drawY;
                            dirtyGrid.markRect(pixelX, pixelY, tile.width, tile.height);
                        }
                    }
                }

                // Per-format tail: which palette LUT to build and which blit
                // to call, selected per TileT with `if constexpr`. 4bpp and
                // 2bpp build a palette LUT and blit through
                // drawSpriteInternal; 1bpp has no per-tile palette and blits
                // through drawSprite() with the map's single Color instead.
                if constexpr (std::is_same_v<TileT, Sprite4bpp>) {
                    const uint16_t* palettePtr = (map.paletteIndices != nullptr)
                        ? getBackgroundPaletteSlot(map.paletteIndices[cellIndex] & kTileCellPaletteMask)
                        : getBackgroundPaletteSlot(0);

                    if (tile.palette != lastTilePalettePtr || palettePtr != lastBackgroundPalettePtr) {
                        uint8_t paletteCount = tile.paletteSize > 16 ? 16 : tile.paletteSize;
                        for (uint8_t i = 0; i < paletteCount; ++i) {
                            cachedLUT[i] = resolveColorWithPalette(tile.palette[i], palettePtr);
                        }
                        for (uint8_t i = paletteCount; i < 16; ++i) {
                            cachedLUT[i] = 0;
                        }
                        lastTilePalettePtr = tile.palette;
                        lastBackgroundPalettePtr = palettePtr;
                    }

                    drawSpriteInternal(tile, drawX, drawY, cachedLUT, false);
                }

                if constexpr (std::is_same_v<TileT, Sprite2bpp>) {
                    const uint16_t* palettePtr = (map.paletteIndices != nullptr)
                        ? getBackgroundPaletteSlot(map.paletteIndices[cellIndex] & kTileCellPaletteMask)
                        : getBackgroundPaletteSlot(0);

                    if (tile.palette != lastTilePalettePtr || palettePtr != lastBackgroundPalettePtr) {
                        uint8_t paletteCount = tile.paletteSize > 4 ? 4 : tile.paletteSize;
                        for (uint8_t i = 0; i < paletteCount; ++i) {
                            cachedLUT[i] = resolveColorWithPalette(tile.palette[i], palettePtr);
                        }
                        for (uint8_t i = paletteCount; i < 4; ++i) {
                            cachedLUT[i] = 0;
                        }
                        lastTilePalettePtr = tile.palette;
                        lastBackgroundPalettePtr = palettePtr;
                    }

                    drawSpriteInternal(tile, drawX, drawY, cachedLUT, false);
                }

                if constexpr (std::is_same_v<TileT, Sprite>) {
                    drawSprite(tile, drawX, drawY, color, false);
                }
            }
        }

        if (h.animSlot) {
            h.animSlot->primed = true;
            h.animSlot->mapKey = map.indices;
            h.animSlot->ox     = h.viewOriginX;
            h.animSlot->oy     = h.viewOriginY;
        }

        tilemapSpriteDirtyMode_ = h.savedMode;
        setRenderContext(h.oldContext);
    }

    void Renderer::drawTileMap(const TileMap4bpp& map, int originX, int originY,
                                LayerType layerType, const pixelroot32::math::ProjectionSpec& projection) {
        if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
            // Color is ignored on this path: the 4bpp tail blits through its
            // own per-tile palette LUT, not a single map-wide fill colour.
            drawTileMapProjectedImpl<Sprite4bpp>(map, originX, originY, layerType, projection, Color::Black);
        }
    }

    void Renderer::drawTileMap(const TileMap2bpp& map, int originX, int originY,
                                LayerType layerType, const pixelroot32::math::ProjectionSpec& projection) {
        if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
            // Color is ignored on this path: the 2bpp tail blits through its
            // own per-tile palette LUT, not a single map-wide fill colour.
            drawTileMapProjectedImpl<Sprite2bpp>(map, originX, originY, layerType, projection, Color::Black);
        }
    }

    void Renderer::drawTileMap(const TileMap& map, int originX, int originY, Color color,
                                LayerType layerType, const pixelroot32::math::ProjectionSpec& projection) {
        // No feature-flag gate, matching the orthogonal 1bpp overload above:
        // 1bpp sprites have no build-time enable flag.
        drawTileMapProjectedImpl<Sprite>(map, originX, originY, layerType, projection, color);
    }
#endif

    void Renderer::setSpritePaletteSlotContext(uint8_t slot) {
        currentSpritePaletteSlot = slot;
    }

    uint8_t Renderer::getSpritePaletteSlotContext() const {
        return currentSpritePaletteSlot;
    }

}
