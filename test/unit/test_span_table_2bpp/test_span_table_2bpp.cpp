/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * computeSpanTable(Sprite2bpp&) must read pixels in the same bit order as the
 * 2bpp blit (Renderer::drawSpriteInternal), or the span it computes clips the
 * wrong columns.
 *
 * The blit reads each row as little-endian 16-bit words with
 * bitOffset = (col & 7) << 1, i.e. LSB = leftmost pixel: pixel `col` lives in
 * byte `col >> 2` at shift `(col & 3) * 2`. Rows are
 * (width * 2 + 7) / 8 bytes apart.
 *
 * The sprite used here is 12 pixels wide (not a multiple of 8, so the row
 * stride is 3 bytes) and asymmetric: some rows are opaque only at the left,
 * others only at the right, so a reader with the wrong bit order cannot
 * produce the right span by accident.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/DisplayConfig.h"
#include "graphics/SpanTable.h"
#include "../../mocks/MockDrawSurface.h"

#include <cstdint>
#include <cstring>
#include <memory>

using namespace pixelroot32::graphics;

namespace {

constexpr int kFbWidth = 32;
constexpr int kFbHeight = 32;
constexpr uint8_t kSentinel = 0xAAu;

uint8_t gFrameBuffer[kFbWidth * kFbHeight];

const uint16_t kPalette[16] = {
    0x0000u, 0xF800u, 0x07E0u, 0x001Fu,
    0xFFE0u, 0xF81Fu, 0x07FFu, 0xFFFFu,
    0x8410u, 0xC618u, 0x7BEFu, 0x39E7u,
    0xFC00u, 0x03E0u, 0x0018u, 0xE01Fu,
};

const Color kIdentityColors2bpp[4] = {
    static_cast<Color>(0), static_cast<Color>(1),
    static_cast<Color>(2), static_cast<Color>(3),
};

constexpr int kWidth = 12;
constexpr int kHeight = 5;
constexpr int kRowBytes = (kWidth * 2 + 7) / 8;  // 3, as the blit computes it

/// Pixel values, one row per line. 0 is transparent.
const uint8_t kPixels[kHeight][kWidth] = {
    {1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // left only   -> [0, 3)
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 1},  // right only  -> [9, 12)
    {0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},  // single px   -> [5, 6)
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // transparent -> [0, 0)
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 3, 0},  // spans word boundary -> [7, 11)
};

const uint8_t kExpectedMin[kHeight] = {0, 9, 5, 0, 7};
const uint8_t kExpectedMax[kHeight] = {3, 12, 6, 0, 11};

/// Packed sprite data, plus one padding byte: the blit reads whole 16-bit
/// words, so the last row's second word reaches one byte past the row.
uint8_t gData[kRowBytes * kHeight + 1];

/// Packs kPixels exactly the way the blit reads it (LSB = leftmost pixel).
void packSprite() {
    std::memset(gData, 0, sizeof(gData));
    for (int row = 0; row < kHeight; ++row) {
        for (int col = 0; col < kWidth; ++col) {
            const uint8_t v = static_cast<uint8_t>(kPixels[row][col] & 0x03u);
            gData[row * kRowBytes + (col >> 2)] |=
                static_cast<uint8_t>(v << ((col & 3) * 2));
        }
    }
}

Sprite2bpp makeSprite() {
    packSprite();
    return Sprite2bpp{gData, kIdentityColors2bpp, kWidth, kHeight, 4};
}

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
        std::memset(gFrameBuffer, kSentinel, sizeof(gFrameBuffer));
    }
};

}  // namespace

void setUp(void) {
    setDualCustomPalette(kPalette, kPalette);
}

void tearDown(void) {}

/// The span of every row equals the opaque extent of the pixels the blit reads.
void test_2bpp_span_matches_blit_bit_order(void) {
    Sprite2bpp sprite = makeSprite();
    uint8_t minX[kHeight];
    uint8_t maxX[kHeight];
    computeSpanTable(sprite, minX, maxX);

    for (int row = 0; row < kHeight; ++row) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(kExpectedMin[row], minX[row], "rowMinX");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(kExpectedMax[row], maxX[row], "rowMaxX");
    }
}

/// computeSpanTable leaves the sprite's own span pointers alone.
void test_2bpp_span_does_not_modify_sprite(void) {
    Sprite2bpp sprite = makeSprite();
    uint8_t minX[kHeight];
    uint8_t maxX[kHeight];
    computeSpanTable(sprite, minX, maxX);
    TEST_ASSERT_NULL(sprite.rowMinX);
    TEST_ASSERT_NULL(sprite.rowMaxX);
}

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)

/// Drawing with the computed span table writes exactly the same framebuffer as
/// drawing the full bounding box: the span must never clip an opaque pixel.
void test_2bpp_span_draw_matches_full_bbox_draw(void) {
    Sprite2bpp sprite = makeSprite();

    static uint8_t baseline[kFbWidth * kFbHeight];
    {
        Harness h;
        h.renderer->drawSprite(sprite, 3, 2, false);
        std::memcpy(baseline, gFrameBuffer, sizeof(baseline));
    }

    uint8_t minX[kHeight];
    uint8_t maxX[kHeight];
    computeSpanTable(sprite, minX, maxX);
    Sprite2bpp withSpan = sprite;
    withSpan.rowMinX = minX;
    withSpan.rowMaxX = maxX;

    Harness h;
    h.renderer->drawSprite(withSpan, 3, 2, false);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(baseline, gFrameBuffer, sizeof(baseline));

    // And the baseline really drew the asymmetric pixels (guards against a
    // vacuous comparison of two untouched framebuffers).
    TEST_ASSERT_NOT_EQUAL(kSentinel, baseline[2 * kFbWidth + 3 + 0]);   // row 0, col 0
    TEST_ASSERT_NOT_EQUAL(kSentinel, baseline[3 * kFbWidth + 3 + 11]);  // row 1, col 11
    TEST_ASSERT_EQUAL_UINT8(kSentinel, baseline[2 * kFbWidth + 3 + 3]); // row 0, col 3
}

#endif

int runUnityTests() {
    UNITY_BEGIN();
    RUN_TEST(test_2bpp_span_matches_blit_bit_order);
    RUN_TEST(test_2bpp_span_does_not_modify_sprite);
#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)
    RUN_TEST(test_2bpp_span_draw_matches_full_bbox_draw);
#endif
    return UNITY_END();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return runUnityTests();
}
