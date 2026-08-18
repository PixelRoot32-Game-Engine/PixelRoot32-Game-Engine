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
#else
    RUN_TEST(test_four_bpp_disabled);
#endif

    return UNITY_END();
}
