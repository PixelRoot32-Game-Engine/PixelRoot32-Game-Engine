/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Characterization tests for the 4bpp sprite blit against a direct 8bpp
 * logical framebuffer (the path every ESP32/TFT_eSPI build takes).
 *
 * This is the hot loop of any sprite-per-cell renderer -- an isometric room
 * drives ~37k source pixels per frame through it -- so it is the one place
 * where a per-pixel operation is worth moving out of the loop. These tests pin
 * the OBSERVABLE result of that loop (which bytes land in the framebuffer) so
 * the packing can be hoisted into a per-sprite table without changing a pixel.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/DisplayConfig.h"
#include "../../mocks/MockDrawSurface.h"

#include <cstdint>
#include <cstring>
#include <memory>

using namespace pixelroot32::graphics;

namespace {

constexpr int kFbWidth = 32;
constexpr int kFbHeight = 32;

/// Fill value that no colour in kPalette below can legitimately pack to, so
/// "left alone" and "written black" stay distinguishable.
constexpr uint8_t kSentinel = 0xAAu;

uint8_t gFrameBuffer[kFbWidth * kFbHeight];

/**
 * @brief The packing Renderer must apply, restated independently.
 *
 * Deliberately a duplicate of the engine's packRgb565ToTftSprite8 rather than
 * a call to it: a test that reuses the implementation cannot catch the
 * implementation changing. This is the TFT_eSprite 8bpp format (RRRGGGBB).
 */
uint8_t expectedPack(uint16_t rgb565) {
    return static_cast<uint8_t>(((rgb565 & 0xE000) >> 8) |
                                ((rgb565 & 0x0700) >> 6) |
                                ((rgb565 & 0x0018) >> 3));
}

/// 16 distinct RGB565 entries. Index 0 is never drawn: 4bpp value 0 is transparent.
const uint16_t kPalette[16] = {
    0x0000u, 0xF800u, 0x07E0u, 0x001Fu,
    0xFFE0u, 0xF81Fu, 0x07FFu, 0xFFFFu,
    0x8410u, 0xC618u, 0x7BEFu, 0x39E7u,
    0xFC00u, 0x03E0u, 0x0018u, 0xE01Fu,
};

/// Identity mapping: sprite palette entry i resolves to kPalette[i].
const Color kIdentityMapping[16] = {
    static_cast<Color>(0),  static_cast<Color>(1),  static_cast<Color>(2),  static_cast<Color>(3),
    static_cast<Color>(4),  static_cast<Color>(5),  static_cast<Color>(6),  static_cast<Color>(7),
    static_cast<Color>(8),  static_cast<Color>(9),  static_cast<Color>(10), static_cast<Color>(11),
    static_cast<Color>(12), static_cast<Color>(13), static_cast<Color>(14), static_cast<Color>(15),
};

/// One 4bpp row is packed low-nibble-first: byte n holds pixels 2n and 2n+1.
uint8_t nibblePair(uint8_t evenPixel, uint8_t oddPixel) {
    return static_cast<uint8_t>((evenPixel & 0x0Fu) | ((oddPixel & 0x0Fu) << 4));
}

/**
 * @brief Owns a Renderer wired to a MockDrawSurface exposing gFrameBuffer.
 *
 * The buffer is filled with kSentinel rather than zeroed so "the blit left this
 * pixel alone" and "the blit wrote black here" are distinguishable, which is
 * what the transparency and clipping cases turn on.
 */
struct Harness {
    std::unique_ptr<Renderer> renderer;

    Harness() {
        auto mock = std::make_unique<MockDrawSurface>();
        MockDrawSurface* mockRaw = mock.get();
        mockRaw->setSpriteBuffer(gFrameBuffer, sizeof(gFrameBuffer));

        DisplayConfig config =
            PIXELROOT32_CUSTOM_DISPLAY(mock.release(), kFbWidth, kFbHeight);
        renderer = std::make_unique<Renderer>(std::move(config));
        renderer->init();
        renderer->beginFrame();

        // Seeded AFTER beginFrame so the sentinel survives whatever clearing
        // strategy the dirty-region configuration picked for this build.
        std::memset(gFrameBuffer, kSentinel, sizeof(gFrameBuffer));
    }
};

uint8_t fbAt(int x, int y) {
    return gFrameBuffer[y * kFbWidth + x];
}

}  // namespace

void setUp(void) {
    setDualCustomPalette(kPalette, kPalette);
}

void tearDown(void) {}

#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)

/**
 * @brief Every opaque pixel lands as its palette colour packed to 8bpp.
 *
 * The baseline the optimisation must not move: four different palette indices
 * in one row, each asserted against the packing computed independently above.
 */
void test_opaque_pixels_land_packed(void) {
    const uint8_t data[] = {nibblePair(1, 2), nibblePair(3, 4)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 4, 1, 16};

    Harness h;
    h.renderer->drawSprite(sprite, 0, 0, false);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[2]), fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[3]), fbAt(2, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[4]), fbAt(3, 0));
}

/// 4bpp value 0 is transparent: the framebuffer byte must survive untouched.
void test_index_zero_leaves_the_framebuffer_untouched(void) {
    const uint8_t data[] = {nibblePair(0, 5), nibblePair(0, 6)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 4, 1, 16};

    Harness h;
    h.renderer->drawSprite(sprite, 0, 0, false);

    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[5]), fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(2, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[6]), fbAt(3, 0));
}

/// flipX mirrors the row in place; it is a separate loop and needs its own pin.
void test_flip_x_mirrors_the_row(void) {
    const uint8_t data[] = {nibblePair(1, 2), nibblePair(3, 4)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 4, 1, 16};

    Harness h;
    h.renderer->drawSprite(sprite, 0, 0, true);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[4]), fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[3]), fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[2]), fbAt(2, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(3, 0));
}

/// An odd width exercises the trailing single-pixel tail of the paired loop.
void test_odd_width_draws_its_last_column(void) {
    const uint8_t data[] = {nibblePair(1, 2), nibblePair(3, 0)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 3, 1, 16};

    Harness h;
    h.renderer->drawSprite(sprite, 0, 0, false);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[2]), fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[3]), fbAt(2, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(3, 0));
}

/**
 * @brief A sprite straddling the left edge clips instead of wrapping.
 *
 * Worth its own case because the paired loop bounds-checks each of its two
 * pixels separately: an off-by-one there wraps a pixel onto the previous
 * scanline rather than dropping it, which reads as a stray dot on screen.
 */
void test_negative_x_clips_without_wrapping(void) {
    const uint8_t data[] = {nibblePair(1, 2), nibblePair(3, 4)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 4, 1, 16};

    Harness h;
    h.renderer->drawSprite(sprite, -2, 1, false);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[3]), fbAt(0, 1));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[4]), fbAt(1, 1));
    // The two clipped columns must not have landed on the previous row's tail.
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(kFbWidth - 1, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(kFbWidth - 2, 0));
}

/// Rows past the bottom edge are skipped, not wrapped to the top.
void test_rows_outside_the_framebuffer_are_skipped(void) {
    const uint8_t data[] = {nibblePair(1, 2), nibblePair(1, 2), nibblePair(1, 2)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 2, 3, 16};

    Harness h;
    h.renderer->drawSprite(sprite, 0, kFbHeight - 1, false);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(0, kFbHeight - 1));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[2]), fbAt(1, kFbHeight - 1));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(1, 0));
}

/**
 * @brief A pixel value above the sprite's own paletteSize resolves to black.
 *
 * The one behavioural change in this area. The blit resolves at most
 * `paletteSize` palette entries, so a value beyond that reads a table slot the
 * caller never filled: previously an uninitialised stack read, which put an
 * arbitrary colour on screen and could differ between builds. Black is
 * deterministic and is what an unmapped index resolves to everywhere else
 * (see resolveColorWithPalette).
 */
void test_index_beyond_palette_size_resolves_to_black(void) {
    const uint8_t data[] = {nibblePair(1, 9), nibblePair(15, 0)};
    // Declares two entries, but the pixel data references 9 and 15.
    const Sprite4bpp sprite = {data, kIdentityMapping, 3, 1, 2};

    Harness h;
    h.renderer->drawSprite(sprite, 0, 0, false);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(0x0000u), fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(0x0000u), fbAt(2, 0));
}

/// Same rule on the mirrored path, which reads the table through its own loop.
void test_index_beyond_palette_size_resolves_to_black_when_flipped(void) {
    const uint8_t data[] = {nibblePair(1, 9)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 2, 1, 2};

    Harness h;
    h.renderer->drawSprite(sprite, 0, 0, true);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(0x0000u), fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(1, 0));
}

// =============================================================================
// Span-limited blit tests (change iso-perf-blit-fastpath, T4)
// =============================================================================
//
// drawSpriteInternal reads sprite.rowMinX/rowMaxX when both are non-null AND
// flipX is false. The span metadata lets the inner column loop skip leading
// and trailing transparent nibbles. These tests verify that:
//   - When span pointers reflect the actual opaque region, output matches a
//     full-bbox draw (no visible regression).
//   - flipX=true with span pointers bypasses the span limits (output mirrors
//     the same as without span pointers).
//   - nullptr span pointers produce byte-identical output to a pre-change
//     baseline (the regression pin).

/// Diamond sprite with explicit span metadata: opaque only in cols 2..5 of an
/// 8-wide row. The expected visible pixels are identical to a full-bbox draw
/// of a sprite whose transparent padding has the SAME nibble pattern, so the
/// span path must agree byte-for-byte with the null-span path.
void test_span_limits_produce_identical_visible_pixels(void) {
    // 8x1 sprite, opaque cols 2..5 (palette indices 1,2,3,4). Cols 0,1,6,7 are
    // value 0 (transparent). rowMinX[0]=2, rowMaxX[0]=6.
    const uint8_t data[] = {nibblePair(0, 0), nibblePair(1, 2),
                            nibblePair(3, 4), nibblePair(0, 0)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 8, 1, 16};

    uint8_t rowMinX[1] = {2};
    uint8_t rowMaxX[1] = {6};
    Sprite4bpp spriteWithSpan = sprite;
    spriteWithSpan.rowMinX = rowMinX;
    spriteWithSpan.rowMaxX = rowMaxX;

    Harness h;
    h.renderer->drawSprite(spriteWithSpan, 0, 0, false);

    // Opaque cols 2..5 packed to expected colours.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(2, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[2]), fbAt(3, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[3]), fbAt(4, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[4]), fbAt(5, 0));
    // Transparent cols 0,1,6,7: sentinel survives because the span-limited
    // loop never visited them (or visited and saw val==0).
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(6, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(7, 0));
}

/// When span pointers are null, drawSpriteInternal iterates the full bbox.
/// A diamond sprite drawn via the null-span path must produce the SAME visible
/// pixels as the span-limited path -- proves the optimisation is a refactor,
/// not a behavioural change.
void test_null_span_matches_full_bbox_draw(void) {
    const uint8_t data[] = {nibblePair(0, 0), nibblePair(1, 2),
                            nibblePair(3, 4), nibblePair(0, 0)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 8, 1, 16};
    // rowMinX/rowMaxX default to nullptr per the Sprite4bpp default member
    // initializers; this asserts the baseline that the span path agrees with.

    Harness h;
    h.renderer->drawSprite(sprite, 0, 0, false);

    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(2, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[2]), fbAt(3, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[3]), fbAt(4, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[4]), fbAt(5, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(6, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(7, 0));
}

/// flipX=true MUST bypass span limits (the mirrored layout invalidates the
/// precomputed min/max). Output should equal the null-span flipX output.
void test_flip_x_bypasses_span_limits(void) {
    const uint8_t data[] = {nibblePair(0, 0), nibblePair(1, 2),
                            nibblePair(3, 4), nibblePair(0, 0)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 8, 1, 16};

    uint8_t rowMinX[1] = {2};
    uint8_t rowMaxX[1] = {6};
    Sprite4bpp spriteWithSpan = sprite;
    spriteWithSpan.rowMinX = rowMinX;
    spriteWithSpan.rowMaxX = rowMaxX;

    Harness h;
    h.renderer->drawSprite(spriteWithSpan, 0, 0, true);

    // flipX mirrors: opaque cols were 2..5, so on screen they land at 2..5
    // (mirror within 8-wide row puts col 2 at lx 5, col 5 at lx 2). Just
    // confirm the opaque pixels are present at the expected mirrored cols.
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[4]), fbAt(2, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[3]), fbAt(3, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[2]), fbAt(4, 0));
    TEST_ASSERT_EQUAL_UINT8(expectedPack(kPalette[1]), fbAt(5, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(0, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(1, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(6, 0));
    TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(7, 0));
}

/// An empty span (minX > maxX) is the convention computeSpanTable uses for
/// fully transparent rows. drawSpriteInternal must not write anything and
/// must not crash.
void test_empty_span_row_writes_nothing(void) {
    const uint8_t data[] = {nibblePair(1, 2), nibblePair(3, 4)};
    const Sprite4bpp sprite = {data, kIdentityMapping, 4, 1, 16};

    uint8_t rowMinX[1] = {5};  // past maxX -> empty range
    uint8_t rowMaxX[1] = {3};
    Sprite4bpp spriteWithSpan = sprite;
    spriteWithSpan.rowMinX = rowMinX;
    spriteWithSpan.rowMaxX = rowMaxX;

    Harness h;
    h.renderer->drawSprite(spriteWithSpan, 0, 0, false);

    // Sentinel survives everywhere on this row.
    for (int x = 0; x < 4; ++x) {
        TEST_ASSERT_EQUAL_UINT8(kSentinel, fbAt(x, 0));
    }
}

#else

void test_four_bpp_disabled(void) {
    TEST_PASS_MESSAGE("PIXELROOT32_ENABLE_4BPP_SPRITES=0: no 4bpp blit to pin.");
}

#endif  // PIXELROOT32_ENABLE_4BPP_SPRITES

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if defined(PIXELROOT32_ENABLE_4BPP_SPRITES)
    RUN_TEST(test_opaque_pixels_land_packed);
    RUN_TEST(test_index_zero_leaves_the_framebuffer_untouched);
    RUN_TEST(test_flip_x_mirrors_the_row);
    RUN_TEST(test_odd_width_draws_its_last_column);
    RUN_TEST(test_negative_x_clips_without_wrapping);
    RUN_TEST(test_rows_outside_the_framebuffer_are_skipped);
    RUN_TEST(test_index_beyond_palette_size_resolves_to_black);
    RUN_TEST(test_index_beyond_palette_size_resolves_to_black_when_flipped);
    RUN_TEST(test_span_limits_produce_identical_visible_pixels);
    RUN_TEST(test_null_span_matches_full_bbox_draw);
    RUN_TEST(test_flip_x_bypasses_span_limits);
    RUN_TEST(test_empty_span_row_writes_nothing);
#else
    RUN_TEST(test_four_bpp_disabled);
#endif

    return UNITY_END();
}
