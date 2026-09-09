/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Tests for computeSpanTable (4bpp + 2bpp).
 *
 * Each test builds a synthetic sprite in scratch memory (NOT a real Sprite
 * with PROGMEM/const data), runs computeSpanTable, and checks the resulting
 * per-row minX/maxX arrays against hand-derived expected values.
 *
 * The 4bpp encoding packs two pixels per byte: low nibble = even col,
 * high nibble = odd col. The 2bpp encoding packs four pixels per byte.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/SpanTable.h"

#include <cstdint>
#include <cstring>

using namespace pixelroot32::graphics;

namespace {

/// Packs 4 nibbles into one 4bpp byte (cols 0..3, low-to-high).
inline uint8_t pack4bppNibbles(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
    return static_cast<uint8_t>((c0 & 0x0F) | ((c1 & 0x0F) << 4));
}

/// Packs 4 2-bit pairs into one 2bpp byte, in the byte layout that
/// drawSpriteInternal reads: col 0 = high pair (bits 7..6), col 1 = bits 5..4,
/// col 2 = bits 3..2, col 3 = low pair (bits 1..0).
inline uint8_t pack2bppPairs(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
    return static_cast<uint8_t>(((c0 & 0x03) << 6) | ((c1 & 0x03) << 4) |
                                ((c2 & 0x03) << 2) | ((c3 & 0x03)));
}

constexpr uint8_t kPalette4bpp[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
constexpr Color   kIdentityColors[16] = {
    Color(0), Color(1), Color(2), Color(3), Color(4), Color(5), Color(6), Color(7),
    Color(8), Color(9), Color(10), Color(11), Color(12), Color(13), Color(14), Color(15),
};
constexpr Color   kIdentityColors2bpp[4] = {Color(0), Color(1), Color(2), Color(3)};

}  // namespace

void setUp(void) { test_setup(); }
void tearDown(void) { test_teardown(); }

/// AC: synthetic 8x4 sprite with a known opaque region produces the
/// expected per-row minX/maxX arrays.
void test_compute_span_table_4bpp_known_region(void) {
    // 8x4 sprite. Each row is 4 bytes (8 cols * 4 bits / 8).
    // Row layout:
    //   row 0: . . O O O O . .  -> minX=2, maxX=6
    //   row 1: O O O O O O O O  -> minX=0, maxX=8
    //   row 2: O O O O . . . .  -> minX=0, maxX=4
    //   row 3: . . . . . . . .  -> minX=0, maxX=0 (fully transparent)
    uint8_t data[16] = {};
    // row 0: cols 2..5 opaque (palette index 1).
    // For width=8, rowBytes=4. col 2,3 -> byte 1; col 4,5 -> byte 2.
    data[0] = pack4bppNibbles(0, 0, 0, 0);  // cols 0,1
    data[1] = pack4bppNibbles(1, 1, 0, 0);  // cols 2,3
    data[2] = pack4bppNibbles(1, 1, 0, 0);  // cols 4,5
    data[3] = pack4bppNibbles(0, 0, 0, 0);  // cols 6,7
    // row 1: all opaque
    data[4] = pack4bppNibbles(1, 1, 1, 1);
    data[5] = pack4bppNibbles(1, 1, 1, 1);
    data[6] = pack4bppNibbles(1, 1, 1, 1);
    data[7] = pack4bppNibbles(1, 1, 1, 1);
    // row 2: cols 0..3 opaque
    data[8] = pack4bppNibbles(1, 1, 1, 1);  // cols 0,1
    data[9] = pack4bppNibbles(1, 1, 0, 0);  // cols 2,3
    data[10] = pack4bppNibbles(0, 0, 0, 0);
    data[11] = pack4bppNibbles(0, 0, 0, 0);
    // row 3: all zero (already)

    Sprite4bpp sprite{data, kIdentityColors, 8, 4, 16};
    uint8_t minX[4];
    uint8_t maxX[4];
    computeSpanTable(sprite, minX, maxX);

    TEST_ASSERT_EQUAL_UINT8(2, minX[0]); TEST_ASSERT_EQUAL_UINT8(6, maxX[0]);
    TEST_ASSERT_EQUAL_UINT8(0, minX[1]); TEST_ASSERT_EQUAL_UINT8(8, maxX[1]);
    TEST_ASSERT_EQUAL_UINT8(0, minX[2]); TEST_ASSERT_EQUAL_UINT8(4, maxX[2]);
    TEST_ASSERT_EQUAL_UINT8(0, minX[3]); TEST_ASSERT_EQUAL_UINT8(0, maxX[3]);
}

/// AC: a row that contains no opaque nibbles produces minX=0, maxX=0.
void test_compute_span_table_fully_transparent_row(void) {
    uint8_t data[4] = {};  // 8x1, all zero
    Sprite4bpp sprite{data, kIdentityColors, 8, 1, 16};
    uint8_t minX[1];
    uint8_t maxX[1];
    computeSpanTable(sprite, minX, maxX);
    TEST_ASSERT_EQUAL_UINT8(0, minX[0]);
    TEST_ASSERT_EQUAL_UINT8(0, maxX[0]);
}

/// AC: odd width (5) correctly reads the low-half nibble of the last
/// byte (col 4 in a 5-wide sprite). Encoding: even col -> low nibble,
/// odd col -> high nibble, so col 4 (even) lives in byte 2's low nibble.
void test_compute_span_table_4bpp_odd_width(void) {
    // 5x1 sprite. Width 5 -> rowBytes = (5*4+7)/8 = 3.
    // Byte 0: cols 0,1; byte 1: cols 2,3; byte 2: col 4 (low nibble).
    // Set col 4 = palette index 5.
    uint8_t data[3] = {};
    data[2] = 0x05u;  // low nibble 5 -> col 4 = 5

    Sprite4bpp sprite{data, kIdentityColors, 5, 1, 16};
    uint8_t minX[1];
    uint8_t maxX[1];
    computeSpanTable(sprite, minX, maxX);
    TEST_ASSERT_EQUAL_UINT8(4, minX[0]);
    TEST_ASSERT_EQUAL_UINT8(5, maxX[0]);
}

/// AC: 2bpp variant mirrors 4bpp correctness. Encoding: 4 pixels per byte,
/// col c at shift (3 - (c & 3)) * 2.
void test_compute_span_table_2bpp_known_region(void) {
    // 8x2 sprite. Each row is 2 bytes (8 cols * 2 bits / 8).
    // Row 0: col 2 opaque (value 1); col 5, 6 opaque.
    // Row 1: cols 0..7 opaque (value 3 = index 3).
    uint8_t data[4] = {};
    // row 0 byte 0: cols 0..3. col 2 = 1.
    data[0] = pack2bppPairs(0, 0, 1, 0);  // col 2 = 1
    // row 0 byte 1: cols 4..7. col 5 = 1, col 6 = 1.
    data[1] = pack2bppPairs(0, 1, 1, 0);  // col 5 = 1, col 6 = 1
    // row 1: all opaque (value 3)
    data[2] = pack2bppPairs(3, 3, 3, 3);
    data[3] = pack2bppPairs(3, 3, 3, 3);

    Sprite2bpp sprite{data, kIdentityColors2bpp, 8, 2, 4};
    uint8_t minX[2];
    uint8_t maxX[2];
    computeSpanTable(sprite, minX, maxX);
    TEST_ASSERT_EQUAL_UINT8(2, minX[0]); TEST_ASSERT_EQUAL_UINT8(7, maxX[0]);
    TEST_ASSERT_EQUAL_UINT8(0, minX[1]); TEST_ASSERT_EQUAL_UINT8(8, maxX[1]);
}

/// AC: 2bpp fully transparent row produces {0, 0}.
void test_compute_span_table_2bpp_transparent_row(void) {
    uint8_t data[2] = {};  // 8x1, all zero
    Sprite2bpp sprite{data, kIdentityColors2bpp, 8, 1, 4};
    uint8_t minX[1];
    uint8_t maxX[1];
    computeSpanTable(sprite, minX, maxX);
    TEST_ASSERT_EQUAL_UINT8(0, minX[0]);
    TEST_ASSERT_EQUAL_UINT8(0, maxX[0]);
}

/// AC: computeSpanTable does NOT modify the sprite's rowMinX/rowMaxX
/// pointers (caller assigns them after the call).
void test_compute_span_table_does_not_modify_sprite_pointers(void) {
    uint8_t data[4] = {0x11, 0x11, 0x11, 0x11};  // 8x1 all opaque
    Sprite4bpp sprite{data, kIdentityColors, 8, 1, 16};
    sprite.rowMinX = nullptr;
    sprite.rowMaxX = nullptr;
    uint8_t minX[1];
    uint8_t maxX[1];
    computeSpanTable(sprite, minX, maxX);
    TEST_ASSERT_NULL(sprite.rowMinX);
    TEST_ASSERT_NULL(sprite.rowMaxX);
    TEST_ASSERT_EQUAL_UINT8(0, minX[0]);
    TEST_ASSERT_EQUAL_UINT8(8, maxX[0]);
}

int runUnityTests() {
    UNITY_BEGIN();
    RUN_TEST(test_compute_span_table_4bpp_known_region);
    RUN_TEST(test_compute_span_table_fully_transparent_row);
    RUN_TEST(test_compute_span_table_4bpp_odd_width);
    RUN_TEST(test_compute_span_table_2bpp_known_region);
    RUN_TEST(test_compute_span_table_2bpp_transparent_row);
    RUN_TEST(test_compute_span_table_does_not_modify_sprite_pointers);
    return UNITY_END();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return runUnityTests();
}
