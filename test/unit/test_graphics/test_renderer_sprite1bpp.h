/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the 1bpp Sprite fast path in Renderer::drawSprite().
 *
 * Two rendering branches must stay observationally identical:
 *  - the direct 8bpp framebuffer path, taken when the draw surface exposes a
 *    sprite buffer (ESP32 TFT_eSPI driver),
 *  - the virtual drawPixel() fallback, taken by U8G2/SDL/mock surfaces that
 *    return a null sprite buffer.
 */

#pragma once

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "graphics/Font5x7.h"
#include "graphics/FontManager.h"
#include "../../mocks/MockDrawSurface.h"

#include <cstring>
#include <memory>
#include <vector>

using namespace pixelroot32::graphics;

// ============================================================================
// Fixtures
// ============================================================================

static constexpr int kScreenW = 16;
static constexpr int kScreenH = 16;
static constexpr size_t kFbSize = static_cast<size_t>(kScreenW) * kScreenH;

/// 3x3 test glyph. Rows are MSB-first: bit (width - 1 - col) is column `col`.
///   row 0: X . X
///   row 1: . X .
///   row 2: X X X
static const uint16_t kGlyphRows[3] = {0x5, 0x2, 0x7};
static const Sprite kGlyph{kGlyphRows, 3, 3};

/// Same geometry but with an empty middle row, to exercise the `bits == 0` skip.
static const uint16_t kGappedRows[3] = {0x7, 0x0, 0x7};
static const Sprite kGapped{kGappedRows, 3, 3};

/// Mirror of the packing performed by Renderer (TFT_eSprite 8bpp convention).
static uint8_t packExpected(uint16_t rgb565) {
    return static_cast<uint8_t>(
        ((rgb565 & 0xE000) >> 8) |
        ((rgb565 & 0x0700) >> 6) |
        ((rgb565 & 0x0018) >> 3));
}

/**
 * @brief Owns a Renderer wired to a MockDrawSurface plus a host-side framebuffer.
 */
struct SpriteHarness {
    MockDrawSurface* surface = nullptr;
    std::unique_ptr<Renderer> renderer;
    std::vector<uint8_t> framebuffer;

    explicit SpriteHarness(bool withSpriteBuffer)
        : framebuffer(kFbSize, 0) {
        auto owned = std::make_unique<MockDrawSurface>();
        surface = owned.get();
        if (withSpriteBuffer) {
            surface->setSpriteBuffer(framebuffer.data(), framebuffer.size());
        }
        DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(owned.release(), kScreenW, kScreenH);
        renderer = std::make_unique<Renderer>(config);
        renderer->setDisplaySize(kScreenW, kScreenH);
        // beginFrame() is what latches the draw surface sprite buffer into the renderer.
        renderer->beginFrame();
        std::memset(framebuffer.data(), 0, framebuffer.size());
        surface->calls.clear();
    }

    /// Rasterizes the recorded drawPixel() calls so both branches can be compared.
    std::vector<uint8_t> rasterizeRecordedPixels() const {
        std::vector<uint8_t> out(kFbSize, 0);
        for (const auto& call : surface->calls) {
            if (call.type != "pixel") continue;
            TEST_ASSERT_TRUE(call.x >= 0 && call.x < kScreenW);
            TEST_ASSERT_TRUE(call.y >= 0 && call.y < kScreenH);
            out[static_cast<size_t>(call.y) * kScreenW + call.x] = packExpected(call.color);
        }
        return out;
    }
};

static uint8_t expectedInk() {
    return packExpected(resolveColor(Color::White, PaletteContext::Sprite));
}

static uint8_t pixelAt(const std::vector<uint8_t>& fb, int x, int y) {
    return fb[static_cast<size_t>(y) * kScreenW + x];
}

static size_t countNonZero(const std::vector<uint8_t>& fb) {
    size_t n = 0;
    for (uint8_t v : fb) {
        if (v != 0) ++n;
    }
    return n;
}

/// Renders `sprite` through both branches and asserts they agree pixel for pixel.
static void assertBranchesMatch(const Sprite& sprite, int x, int y, bool flipX) {
    SpriteHarness fast(true);
    fast.renderer->drawSprite(sprite, x, y, Color::White, flipX);

    SpriteHarness fallback(false);
    fallback.renderer->drawSprite(sprite, x, y, Color::White, flipX);

    const std::vector<uint8_t> reference = fallback.rasterizeRecordedPixels();
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reference.data(), fast.framebuffer.data(), kFbSize);
}

// ============================================================================
// Fast path (non-null sprite buffer)
// ============================================================================

void test_sprite1bpp_fast_path_writes_framebuffer(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGlyph, 4, 5, Color::White, false);

    const uint8_t ink = expectedInk();
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 4, 5));
    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(h.framebuffer, 5, 5));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 6, 5));
    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(h.framebuffer, 4, 6));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 5, 6));
    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(h.framebuffer, 6, 6));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 4, 7));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 5, 7));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 6, 7));
    TEST_ASSERT_EQUAL_UINT32(6, countNonZero(h.framebuffer));
}

void test_sprite1bpp_fast_path_bypasses_draw_pixel(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGlyph, 4, 5, Color::White, false);

    TEST_ASSERT_FALSE(h.surface->hasCall("pixel"));
}

void test_sprite1bpp_fast_path_flip_x(void) {
    SpriteHarness h(true);
    // Asymmetric row pattern: X . .  (only column 0 set)
    static const uint16_t rows[1] = {0x4};
    static const Sprite asymmetric{rows, 3, 1};

    h.renderer->drawSprite(asymmetric, 2, 3, Color::White, true);

    const uint8_t ink = expectedInk();
    // flipX maps column 0 to startX + (width - 1 - 0) = 2 + 2 = 4.
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 4, 3));
    TEST_ASSERT_EQUAL_UINT32(1, countNonZero(h.framebuffer));
}

void test_sprite1bpp_fast_path_skips_empty_rows(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGapped, 0, 0, Color::White, false);

    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(h.framebuffer, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(h.framebuffer, 1, 1));
    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(h.framebuffer, 2, 1));
    TEST_ASSERT_EQUAL_UINT32(6, countNonZero(h.framebuffer));
}

void test_sprite1bpp_fast_path_respects_display_offset(void) {
    SpriteHarness h(true);
    h.renderer->setDisplayOffset(2, 1);
    h.renderer->drawSprite(kGlyph, 1, 1, Color::White, false);

    const uint8_t ink = expectedInk();
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 3, 2));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 5, 2));
    TEST_ASSERT_EQUAL_UINT32(6, countNonZero(h.framebuffer));
}

void test_sprite1bpp_fast_path_honours_offset_bypass(void) {
    SpriteHarness h(true);
    h.renderer->setDisplayOffset(2, 1);
    h.renderer->setOffsetBypass(true);
    h.renderer->drawSprite(kGlyph, 1, 1, Color::White, false);

    const uint8_t ink = expectedInk();
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 1, 1));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 3, 1));
    TEST_ASSERT_EQUAL_UINT32(6, countNonZero(h.framebuffer));
}

// ============================================================================
// Clipping on all four edges
// ============================================================================

void test_sprite1bpp_clips_left_edge(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGlyph, -1, 5, Color::White, false);

    const uint8_t ink = expectedInk();
    // Column 0 falls at x = -1 and is dropped; columns 1 and 2 land at x = 0 and 1.
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 1, 5));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 0, 6));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 0, 7));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 1, 7));
    TEST_ASSERT_EQUAL_UINT32(4, countNonZero(h.framebuffer));
}

void test_sprite1bpp_clips_right_edge(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGlyph, kScreenW - 2, 0, Color::White, false);

    const uint8_t ink = expectedInk();
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, kScreenW - 2, 0));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, kScreenW - 1, 1));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, kScreenW - 2, 2));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, kScreenW - 1, 2));
    TEST_ASSERT_EQUAL_UINT32(4, countNonZero(h.framebuffer));
}

void test_sprite1bpp_clips_top_edge(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGlyph, 3, -2, Color::White, false);

    const uint8_t ink = expectedInk();
    // Only the last row (y = 0) survives.
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 3, 0));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 4, 0));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 5, 0));
    TEST_ASSERT_EQUAL_UINT32(3, countNonZero(h.framebuffer));
}

void test_sprite1bpp_clips_bottom_edge(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGlyph, 3, kScreenH - 1, Color::White, false);

    const uint8_t ink = expectedInk();
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 3, kScreenH - 1));
    TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, 5, kScreenH - 1));
    TEST_ASSERT_EQUAL_UINT32(2, countNonZero(h.framebuffer));
}

void test_sprite1bpp_fully_offscreen_writes_nothing(void) {
    SpriteHarness h(true);
    h.renderer->drawSprite(kGlyph, -8, -8, Color::White, false);
    h.renderer->drawSprite(kGlyph, kScreenW + 4, kScreenH + 4, Color::White, false);

    TEST_ASSERT_EQUAL_UINT32(0, countNonZero(h.framebuffer));
}

// ============================================================================
// Fallback path (null sprite buffer) and branch equivalence
// ============================================================================

void test_sprite1bpp_fallback_uses_draw_pixel(void) {
    SpriteHarness h(false);
    h.renderer->drawSprite(kGlyph, 4, 5, Color::White, false);

    TEST_ASSERT_TRUE(h.surface->hasCall("pixel"));
    TEST_ASSERT_EQUAL_UINT32(6, h.surface->calls.size());
    // Framebuffer must stay untouched: the surface owns the pixels in this mode.
    TEST_ASSERT_EQUAL_UINT32(0, countNonZero(h.framebuffer));
}

void test_sprite1bpp_branches_match_basic(void) {
    assertBranchesMatch(kGlyph, 4, 5, false);
}

void test_sprite1bpp_branches_match_flip_x(void) {
    assertBranchesMatch(kGlyph, 4, 5, true);
}

void test_sprite1bpp_branches_match_empty_rows(void) {
    assertBranchesMatch(kGapped, 2, 2, false);
}

void test_sprite1bpp_branches_match_all_edges(void) {
    assertBranchesMatch(kGlyph, -1, 5, false);
    assertBranchesMatch(kGlyph, kScreenW - 2, 5, false);
    assertBranchesMatch(kGlyph, 3, -2, false);
    assertBranchesMatch(kGlyph, 3, kScreenH - 1, false);
    assertBranchesMatch(kGlyph, -1, 5, true);
    assertBranchesMatch(kGlyph, kScreenW - 2, 5, true);
    assertBranchesMatch(kGlyph, 3, -2, true);
    assertBranchesMatch(kGlyph, 3, kScreenH - 1, true);
}

void test_sprite1bpp_branches_match_wide_sprite(void) {
    // 16 px wide: exercises the full uint16_t row width, including bit 15.
    static const uint16_t rows[4] = {0xFFFF, 0x8001, 0x0FF0, 0x0000};
    static const Sprite wide{rows, 16, 4};

    assertBranchesMatch(wide, 0, 0, false);
    assertBranchesMatch(wide, -3, 1, false);
    assertBranchesMatch(wide, 5, 2, false);
    assertBranchesMatch(wide, 5, 2, true);
}

void test_sprite1bpp_branches_match_text_rendering(void) {
    SpriteHarness fast(true);
    fast.renderer->drawText("Hi 42!", 1, 4, Color::White, 1);

    SpriteHarness fallback(false);
    fallback.renderer->drawText("Hi 42!", 1, 4, Color::White, 1);

    const std::vector<uint8_t> reference = fallback.rasterizeRecordedPixels();
    TEST_ASSERT_TRUE(countNonZero(reference) > 0);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reference.data(), fast.framebuffer.data(), kFbSize);
}

// The sprite1bpp tests are registered by the shared runner in test_graphics.cpp.
// setUp() there calls FontManager::setDefaultFont(&FONT_5X7), which the
// text-rendering parity test relies on.
