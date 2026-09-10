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

// ============================================================================
// Zero-pixel-movement golden (Phase 3.7) and extYOffset placement (Phase 4)
// ============================================================================

/// "Hi" drawn at (0,0), size 1: pixel positions computed independently from
/// FONT5X7's GLYPH_H/GLYPH_i bit patterns (not derived from Renderer code),
/// so this locks the ASCII draw position contract the nextGlyph refactor
/// must not move.
void test_renderer_draw_text_ascii_golden_positions(void) {
    SpriteHarness h(true);
    h.renderer->drawText("Hi", 0, 0, Color::White, 1);

    const uint8_t ink = expectedInk();
    static const int expectedOn[][2] = {
        {0, 0}, {4, 0}, {0, 1}, {4, 1}, {0, 2}, {4, 2},
        {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3},
        {0, 4}, {4, 4}, {0, 5}, {4, 5}, {0, 6}, {4, 6}, // 'H'
        {8, 0}, {7, 2}, {8, 2}, {8, 3}, {8, 4}, {8, 5}, {7, 6}, {8, 6}, {9, 6}, // 'i' at x=6
    };
    for (const auto& p : expectedOn) {
        TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, p[0], p[1]));
    }
    TEST_ASSERT_EQUAL_UINT32(26, countNonZero(h.framebuffer));
}

// Synthetic font: base 'A' plus a supplement glyph at U+00F1 whose body rows
// (1-7) are byte-identical to GLYPH_A's 7 rows -- only row 0 (the accent) and
// extYOffset differ. Proves D5's baseline-sharing arithmetic.
static const uint16_t kExtRowsSize1[8] = {0x000A, 0x0004, 0x000A, 0x0011, 0x0011, 0x001F, 0x0011, 0x0011};
static const Sprite kExtBaseGlyph[] = {{GLYPH_A, 5, 7}};
static const Sprite kExtSupplementGlyph[] = {{kExtRowsSize1, 5, 8}};
static const Font kExtFont = {kExtBaseGlyph, 65, 65, 5, 7, 1, 8, kExtSupplementGlyph, 0xF1, 0xF1, -1};

void test_renderer_draw_text_extended_glyph_shares_baseline_size1(void) {
    SpriteHarness plain(true);
    plain.renderer->drawText("A", 2, 5, Color::White, 1, &kExtFont);

    SpriteHarness accented(true);
    accented.renderer->drawText("\xC3\xB1", 2, 5, Color::White, 1, &kExtFont);

    // GLYPH_A's own rows land at screen y=5..11; the accented glyph's body
    // (rows 1-7 of its 8-row sprite, shifted up by extYOffset=-1) must land
    // on the exact same screen rows.
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            TEST_ASSERT_EQUAL_UINT8(pixelAt(plain.framebuffer, 2 + col, 5 + row),
                                     pixelAt(accented.framebuffer, 2 + col, 5 + row));
        }
    }
}

void test_renderer_draw_text_extended_glyph_shares_baseline_size2(void) {
    SpriteHarness plain(true);
    plain.renderer->drawText("A", 1, 1, Color::White, 2, &kExtFont);

    SpriteHarness accented(true);
    accented.renderer->drawText("\xC3\xB1", 1, 1, Color::White, 2, &kExtFont);

    // Per D5's arithmetic proof, the body's screen rows are unaffected by size.
    // GLYPH_A at size 2 occupies dst rows 0-13 (ceil(7*2)); kept in-bounds of
    // the 16x16 harness screen.
    for (int row = 0; row < 14; ++row) {
        for (int col = 0; col < 10; ++col) {
            TEST_ASSERT_EQUAL_UINT8(pixelAt(plain.framebuffer, 1 + col, 1 + row),
                                     pixelAt(accented.framebuffer, 1 + col, 1 + row));
        }
    }
}

void test_renderer_draw_text_extended_glyph_clips_above_screen(void) {
    SpriteHarness h(true);
    // y=0, extYOffset=-1: the accent row falls at logicalY=-1 and must clip
    // safely (no crash, no wraparound write) while the body still draws at y=0.
    h.renderer->drawText("\xC3\xB1", 2, 0, Color::White, 1, &kExtFont);

    // With the accent row clipped away, only GLYPH_A's 16 body pixels remain --
    // identical to drawing plain 'A' at the same position.
    SpriteHarness reference(true);
    reference.renderer->drawText("A", 2, 0, Color::White, 1, &kExtFont);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reference.framebuffer.data(), h.framebuffer.data(), kFbSize);
    TEST_ASSERT_EQUAL_UINT32(16, countNonZero(h.framebuffer));
}

// ============================================================================
// Latin-1 supplement glyphs, real FONT_5X7 (Phase 5): bit order + legibility
// ============================================================================
// Guarded on the same flag as the glyph data itself -- these assert against
// FONT5X7_LATIN1_GLYPHS, which only exists when PIXELROOT32_ENABLE_FONT_LATIN1
// is on (see Font5x7.cpp). The flag-off contract is covered separately in
// test_font_manager.cpp (test_font_manager_font5x7_latin1_flag_contract).
#if PIXELROOT32_ENABLE_FONT_LATIN1

/// Renders "\xC3\x91" (UTF-8 for U+00D1 'Ntilde') and asserts the exact lit
/// pixel set, computed independently from the intended bit pattern (not from
/// Renderer's own bit-order code). Row 3 of the body (0x19 = 11001b) is
/// asymmetric -- if the leftmost-pixel convention were ever flipped to bit 0
/// (the wrong claim in the sprite-renderer skill doc), this glyph would light
/// a different, detectably wrong column set instead of silently looking fine.
void test_renderer_draw_text_latin1_n_tilde_bit_order_not_mirrored(void) {
    SpriteHarness h(true);
    FontManager::setDefaultFont(&FONT_5X7);
    h.renderer->drawText("\xC3\x91", 1, 2, Color::White, 1);

    const uint8_t ink = expectedInk();
    // x0=1, y0=2; extYOffset=-1 puts the tilde row at y0-1=1.
    static const int expectedOn[][2] = {
        {1, 1}, {3, 1}, {5, 1},                  // row0 (tilde): cols 0,2,4
        {1, 2}, {5, 2},                          // row1 (body row0): cols 0,4
        {1, 3}, {5, 3},                          // row2 (body row1): cols 0,4
        {1, 4}, {2, 4}, {5, 4},                  // row3 (body row2, 0x19): cols 0,1,4
        {1, 5}, {3, 5}, {5, 5},                  // row4 (body row3): cols 0,2,4
        {1, 6}, {4, 6}, {5, 6},                  // row5 (body row4, 0x13): cols 0,3,4
        {1, 7}, {5, 7},                          // row6 (body row5): cols 0,4
        {1, 8}, {5, 8},                          // row7 (body row6): cols 0,4
    };
    for (const auto& p : expectedOn) {
        TEST_ASSERT_EQUAL_UINT8(ink, pixelAt(h.framebuffer, p[0], p[1]));
    }
    TEST_ASSERT_EQUAL_UINT32(20, countNonZero(h.framebuffer));

    // Not mirrored: 0x19 = 11001b lights columns {0,1,4}. Under the wrong
    // "bit 0 = leftmost" convention it would instead light {0,3,4} -- column
    // 3 (screen x=4) would falsely turn on. It must stay dark.
    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(h.framebuffer, 4, 4));
}

/// 'A' vs accented 'Aacute': the 7-row letter body must land on the exact
/// same screen rows (baseline sharing, D5), and the accent row must add ink
/// that plain 'A' does not have -- i.e. the two glyphs are both aligned AND
/// visually distinguishable, not merely occupying the same cell.
void test_renderer_draw_text_latin1_A_acute_distinguishable_from_A(void) {
    FontManager::setDefaultFont(&FONT_5X7);

    SpriteHarness plain(true);
    plain.renderer->drawText("A", 2, 4, Color::White, 1);

    SpriteHarness accented(true);
    accented.renderer->drawText("\xC3\x81", 2, 4, Color::White, 1);  // Aacute

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            TEST_ASSERT_EQUAL_UINT8(pixelAt(plain.framebuffer, 2 + col, 4 + row),
                                     pixelAt(accented.framebuffer, 2 + col, 4 + row));
        }
    }

    // The accent row (screen y=3, one above the shared body) must carry ink
    // for the accented glyph that the plain glyph never draws there.
    TEST_ASSERT_EQUAL_UINT32(countNonZero(plain.framebuffer) + 1, countNonZero(accented.framebuffer));
}

/// Same distinguishability contract as above, for 'N' vs 'Ntilde'. Together
/// with the bit-order test, this covers the two accented uppercase glyphs
/// the demo games actually render (interfaces render in capitals).
void test_renderer_draw_text_latin1_N_tilde_distinguishable_from_N(void) {
    FontManager::setDefaultFont(&FONT_5X7);

    SpriteHarness plain(true);
    plain.renderer->drawText("N", 2, 4, Color::White, 1);

    SpriteHarness accented(true);
    accented.renderer->drawText("\xC3\x91", 2, 4, Color::White, 1);  // Ntilde

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            TEST_ASSERT_EQUAL_UINT8(pixelAt(plain.framebuffer, 2 + col, 4 + row),
                                     pixelAt(accented.framebuffer, 2 + col, 4 + row));
        }
    }
    TEST_ASSERT_TRUE(countNonZero(accented.framebuffer) > countNonZero(plain.framebuffer));
}

/// Regression: 'i' is tittle-bearing -- its own topmost lit row is the dot,
/// not letter ink. The accent must REPLACE that dot, never stack above it:
/// Spanish typography draws exactly one mark over an accented i ("mi", "si",
/// "asi", "pais" all carry a single stroke, never two). Coordinator-reported
/// defect: the first generated GLYPH_LATIN_i_acute lit both the acute (its
/// own row 0) and the inherited tittle (row 1) at once.
void test_renderer_draw_text_latin1_i_acute_replaces_tittle_not_stacks(void) {
    FontManager::setDefaultFont(&FONT_5X7);

    SpriteHarness plain(true);
    plain.renderer->drawText("i", 2, 4, Color::White, 1);

    SpriteHarness accented(true);
    accented.renderer->drawText("\xC3\xAD", 2, 4, Color::White, 1);  // iacute

    // The stem (GLYPH_i rows 2-6, screen y=6..10) is untouched and shared
    // with the plain glyph -- only the mark above it changes.
    for (int row = 2; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            TEST_ASSERT_EQUAL_UINT8(pixelAt(plain.framebuffer, 2 + col, 4 + row),
                                     pixelAt(accented.framebuffer, 2 + col, 4 + row));
        }
    }

    // Where the base glyph's own tittle used to sit (screen y=4, the same
    // row GLYPH_i's row 0 draws at) must now be dark -- the accent replaced
    // it instead of stacking above it.
    TEST_ASSERT_EQUAL_UINT8(0, pixelAt(accented.framebuffer, 4, 4));

    // Exactly one lit pixel across the three rows above the stem gap (accent
    // row y=3, former-tittle row y=4, gap row y=5): the acute alone.
    int litAboveStem = 0;
    for (int row = 3; row <= 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (pixelAt(accented.framebuffer, 2 + col, row) != 0) {
                ++litAboveStem;
            }
        }
    }
    TEST_ASSERT_EQUAL_INT(1, litAboveStem);
    TEST_ASSERT_EQUAL_UINT8(expectedInk(), pixelAt(accented.framebuffer, 5, 3));
}

#endif // PIXELROOT32_ENABLE_FONT_LATIN1

// The sprite1bpp tests are registered by the shared runner in test_graphics.cpp.
// setUp() there calls FontManager::setDefaultFont(&FONT_5X7), which the
// text-rendering parity test relies on.
