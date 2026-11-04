/**
 * @file test_rgb444.cpp
 * @brief Unit tests for graphics/Rgb444 (12-bit RGB444 wire packing)
 * @version 1.0
 * @date 2026-08-11
 *
 * Tests for the RGB565 -> RGB444 packing helpers used by the TFT_eSPI driver
 * when PIXELROOT32_TFT_12BIT_COLOR is enabled:
 * - packRgb565ToRgb444() channel extraction
 * - packRgb444Pair() byte layout on the wire
 * - Bijectivity of the RGB332 -> RGB565 -> RGB444 chain (the property that
 *   justifies sending 12 bits instead of 16)
 * - Exact representation of the 2-bit blue channel
 * - The two wire transforms the driver's 8bpp expansion table applies to a
 *   caller-supplied RGB565 entry (setPalette8()): the byte-swapped RGB565 word
 *   and its RGB444 reduction
 */

#include <unity.h>
#include "graphics/Rgb444.h"
#include "../../test_config.h"

using pixelroot32::graphics::packRgb565ToRgb444;
using pixelroot32::graphics::packRgb444Pair;

// =============================================================================
// Setup / Teardown
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Local mirror of TFT_eSPI::color8to16
// =============================================================================

/**
 * @brief Mirrors TFT_eSPI::color8to16() (TFT_eSPI.cpp, "color8to16").
 *
 * Reproduced here verbatim so the native test suite does not need to link or
 * include TFT_eSPI. If upstream ever changes the expansion, this mirror is the
 * single place to update; the bijectivity test below is what would catch a
 * divergence turning into on-screen colour collisions.
 *
 * Layout of the 8-bit source: RRRGGGBB.
 * - red5   = R2 R1 R0 R2 R1   (3 bits replicated up to 5)
 * - green6 = G2 G1 G0 G2 G1 G0 (3 bits replicated up to 6)
 * - blue5  = {0, 11, 21, 31}[B] (table lookup, not replication)
 */
static uint16_t color8to16Mirror(uint8_t color) {
    static const uint8_t blue[] = {0, 11, 21, 31};
    uint16_t color16 = 0;
    color16 = static_cast<uint16_t>((color & 0x1C) << 6) |
              static_cast<uint16_t>((color & 0xC0) << 5) |
              static_cast<uint16_t>((color & 0xE0) << 8);
    color16 = static_cast<uint16_t>(color16 |
              static_cast<uint16_t>((color & 0x1C) << 3) |
              blue[color & 0x03]);
    return color16;
}

// =============================================================================
// Tests for packRgb565ToRgb444 - channel extraction
// =============================================================================

/**
 * @test Pure black stays black
 * @expected 0x0000 -> 0x000
 */
void test_pack444_black(void) {
    TEST_ASSERT_EQUAL_HEX16(0x000, packRgb565ToRgb444(0x0000));
}

/**
 * @test Pure white stays white
 * @expected 0xFFFF -> 0xFFF
 */
void test_pack444_white(void) {
    TEST_ASSERT_EQUAL_HEX16(0xFFF, packRgb565ToRgb444(0xFFFF));
}

/**
 * @test Full red only
 * @expected 0xF800 (red5 = 31) -> 0xF00
 */
void test_pack444_full_red(void) {
    TEST_ASSERT_EQUAL_HEX16(0xF00, packRgb565ToRgb444(0xF800));
}

/**
 * @test Full green only
 * @expected 0x07E0 (green6 = 63) -> 0x0F0
 */
void test_pack444_full_green(void) {
    TEST_ASSERT_EQUAL_HEX16(0x0F0, packRgb565ToRgb444(0x07E0));
}

/**
 * @test Full blue only
 * @expected 0x001F (blue5 = 31) -> 0x00F
 */
void test_pack444_full_blue(void) {
    TEST_ASSERT_EQUAL_HEX16(0x00F, packRgb565ToRgb444(0x001F));
}

/**
 * @test Result never exceeds 12 bits for any RGB565 input
 * @expected High nibble is always zero
 */
void test_pack444_result_is_12_bits(void) {
    for (uint32_t v = 0; v <= 0xFFFF; ++v) {
        const uint16_t packed = packRgb565ToRgb444(static_cast<uint16_t>(v));
        TEST_ASSERT_EQUAL_HEX16(0x0000, static_cast<uint16_t>(packed & 0xF000));
    }
}

/**
 * @test Each channel takes the top 4 bits of its RGB565 field
 * @expected Hand-computed value for a mixed colour
 *
 * 0x1234 = 0001 0010 0011 0100
 *   red5   = 00010b = 2   -> top 4 bits = 0001b = 1
 *   green6 = 010001b = 17 -> top 4 bits = 0100b = 4
 *   blue5  = 10100b = 20  -> top 4 bits = 1010b = 10 (0xA)
 */
void test_pack444_mixed_colour(void) {
    TEST_ASSERT_EQUAL_HEX16(0x14A, packRgb565ToRgb444(0x1234));
}

// =============================================================================
// Tests for packRgb444Pair - wire byte layout
// =============================================================================

/**
 * @test Byte layout of a red/blue pair
 * @expected byte0 = R0<<4|G0, byte1 = B0<<4|R1, byte2 = G1<<4|B1
 *
 * Pixel 0 = 0xF800 -> 0xF00 (R=15, G=0,  B=0)
 * Pixel 1 = 0x001F -> 0x00F (R=0,  G=0,  B=15)
 *   byte0 = 0xF0, byte1 = 0x00, byte2 = 0x0F
 */
void test_pair_layout_red_blue(void) {
    uint8_t out[3] = {0xAA, 0xAA, 0xAA};
    packRgb444Pair(out, packRgb565ToRgb444(0xF800), packRgb565ToRgb444(0x001F));
    TEST_ASSERT_EQUAL_HEX8(0xF0, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x0F, out[2]);
}

/**
 * @test Byte layout of two arbitrary 12-bit colours
 * @expected Nibbles land in R0 G0 B0 R1 G1 B1 stream order
 *
 * Pixel 0 = 0x123, pixel 1 = 0x456
 *   byte0 = 0x12, byte1 = 0x34, byte2 = 0x56
 */
void test_pair_layout_is_continuous_nibble_stream(void) {
    uint8_t out[3] = {0, 0, 0};
    packRgb444Pair(out, 0x123, 0x456);
    TEST_ASSERT_EQUAL_HEX8(0x12, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x56, out[2]);
}

/**
 * @test Two identical pixels produce a symmetric triple
 * @expected 0xABC twice -> 0xAB 0xCA 0xBC
 */
void test_pair_layout_duplicated_pixel(void) {
    uint8_t out[3] = {0, 0, 0};
    packRgb444Pair(out, 0xABC, 0xABC);
    TEST_ASSERT_EQUAL_HEX8(0xAB, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0xCA, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBC, out[2]);
}

/**
 * @test packRgb444Pair writes exactly three bytes
 * @expected The guard byte past the triple is untouched
 */
void test_pair_writes_exactly_three_bytes(void) {
    uint8_t out[4] = {0, 0, 0, 0x5A};
    packRgb444Pair(out, 0xFFF, 0xFFF);
    TEST_ASSERT_EQUAL_HEX8(0xFF, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x5A, out[3]);
}

/**
 * @test Ignores any bits above the low 12 of each argument
 * @expected 0xF123 behaves exactly like 0x123
 */
void test_pair_ignores_high_bits(void) {
    uint8_t out[3] = {0, 0, 0};
    packRgb444Pair(out, 0xF123, 0xA456);
    TEST_ASSERT_EQUAL_HEX8(0x12, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x56, out[2]);
}

// =============================================================================
// Bijectivity - the property that justifies the whole optimization
// =============================================================================

/**
 * @test RGB332 -> RGB565 -> RGB444 loses no distinguishable colour
 * @expected All 256 framebuffer indices map to 256 distinct 12-bit values
 *
 * The framebuffer is 8bpp RGB332, so the source has at most 256 colours.
 * RGB444 offers 8 red levels x 8 green levels x 4 blue levels reachable from
 * that source = exactly 256 slots with zero collisions. If this ever fails,
 * 12-bit mode would visibly merge palette entries and must not be shipped.
 */
void test_rgb332_to_rgb444_is_bijective(void) {
    bool seen[4096] = {false};
    int distinct = 0;

    for (int i = 0; i < 256; ++i) {
        const uint16_t rgb565 = color8to16Mirror(static_cast<uint8_t>(i));
        const uint16_t rgb444 = packRgb565ToRgb444(rgb565);
        TEST_ASSERT_TRUE_MESSAGE(rgb444 < 4096, "RGB444 value out of 12-bit range");
        TEST_ASSERT_FALSE_MESSAGE(seen[rgb444], "RGB332 -> RGB444 collision detected");
        seen[rgb444] = true;
        ++distinct;
    }

    TEST_ASSERT_EQUAL_INT(256, distinct);
}

/**
 * @test The red channel resolves 8 distinct levels
 * @expected One level per RGB332 red code
 */
void test_rgb332_red_has_8_levels(void) {
    bool seen[16] = {false};
    int distinct = 0;
    for (int r = 0; r < 8; ++r) {
        const uint8_t rgb332 = static_cast<uint8_t>(r << 5);
        const uint16_t rgb444 = packRgb565ToRgb444(color8to16Mirror(rgb332));
        const uint8_t level = static_cast<uint8_t>((rgb444 >> 8) & 0x0F);
        TEST_ASSERT_FALSE(seen[level]);
        seen[level] = true;
        ++distinct;
    }
    TEST_ASSERT_EQUAL_INT(8, distinct);
}

/**
 * @test The green channel resolves 8 distinct levels
 * @expected One level per RGB332 green code
 */
void test_rgb332_green_has_8_levels(void) {
    bool seen[16] = {false};
    int distinct = 0;
    for (int g = 0; g < 8; ++g) {
        const uint8_t rgb332 = static_cast<uint8_t>(g << 2);
        const uint16_t rgb444 = packRgb565ToRgb444(color8to16Mirror(rgb332));
        const uint8_t level = static_cast<uint8_t>((rgb444 >> 4) & 0x0F);
        TEST_ASSERT_FALSE(seen[level]);
        seen[level] = true;
        ++distinct;
    }
    TEST_ASSERT_EQUAL_INT(8, distinct);
}

/**
 * @test The 2-bit blue channel is exactly representable in 4 bits
 * @expected Blue codes 0,1,2,3 map to 0,5,10,15
 *
 * TFT_eSPI expands 2-bit blue through the table {0, 11, 21, 31}; taking the top
 * 4 bits of those gives {0, 5, 10, 15} - evenly spaced and lossless, unlike a
 * naive truncation which would collapse two of the four codes.
 */
void test_blue_channel_is_exactly_representable(void) {
    const uint8_t expected[4] = {0, 5, 10, 15};
    for (int b = 0; b < 4; ++b) {
        const uint8_t rgb332 = static_cast<uint8_t>(b);
        const uint16_t rgb444 = packRgb565ToRgb444(color8to16Mirror(rgb332));
        TEST_ASSERT_EQUAL_HEX8(expected[b], static_cast<uint8_t>(rgb444 & 0x0F));
    }
}

/**
 * @test A full palette sweep survives the pair packer intact
 * @expected Every (i, j) pair round-trips back to the two source 12-bit values
 *
 * Guards the driver's hot loop: the packer is what the 1:1 and scaled paths run
 * per pixel pair, so an asymmetric nibble bug would corrupt every other pixel.
 */
void test_pair_round_trips_for_all_palette_pairs(void) {
    for (int i = 0; i < 256; ++i) {
        const uint16_t a = packRgb565ToRgb444(color8to16Mirror(static_cast<uint8_t>(i)));
        for (int j = 0; j < 256; ++j) {
            const uint16_t b = packRgb565ToRgb444(color8to16Mirror(static_cast<uint8_t>(j)));
            uint8_t out[3] = {0, 0, 0};
            packRgb444Pair(out, a, b);

            const uint16_t decodedA = static_cast<uint16_t>((out[0] << 4) | (out[1] >> 4));
            const uint16_t decodedB = static_cast<uint16_t>(((out[1] & 0x0F) << 8) | out[2]);
            TEST_ASSERT_EQUAL_HEX16(a, decodedA);
            TEST_ASSERT_EQUAL_HEX16(b, decodedB);
        }
    }
}

// =============================================================================
// constexpr usability
// =============================================================================

/**
 * @test The packer is usable in a constant expression
 * @expected Compile-time evaluation matches the runtime result
 */
void test_pack444_is_constexpr(void) {
    constexpr uint16_t kWhite = packRgb565ToRgb444(0xFFFF);
    static_assert(kWhite == 0xFFF, "packRgb565ToRgb444 must be constexpr-evaluable");
    TEST_ASSERT_EQUAL_HEX16(0xFFF, kWhite);
}

// =============================================================================
// The 8bpp surface's expansion table (DrawSurface::setPalette8)
// =============================================================================

// The driver itself is not constructible on the host: TFT_eSPI_Drawer needs
// TFT_eSPI and Arduino, and no native environment has either. These cases pin
// the two transforms its table builder applies to a caller-supplied RGB565
// entry -- the part that can be wrong without hardware. The RGB565 present path
// stores the word byte-swapped; the RGB444 path stores
// packRgb565ToRgb444(entry) and rebuilds pairLUT from it.

/// Mirrors the byte swap the RGB565 present path stores for each table entry.
static uint16_t swapWireWord(uint16_t value) {
    return static_cast<uint16_t>((value >> 8) | (value << 8));
}

/**
 * @test A supplied table's 565 word survives the wire byte swap
 * @expected swap(swap(v)) == v, and each byte is the other byte of v
 *
 * pushPixelsDMA() dumps the line buffer verbatim, so the table stores each word
 * pre-swapped. A table installed with setPalette8() goes through that same
 * store, which is only lossless if the swap is the inverse the panel applies.
 * The entries here vary both bytes, so a swap that touched only one of them
 * fails instead of passing by coincidence.
 */
void test_palette565_wire_swap_is_lossless(void) {
    for (int i = 0; i < 256; ++i) {
        const uint16_t entry = static_cast<uint16_t>((i << 8) | ((i * 37 + 11) & 0xFF));
        const uint16_t wire = swapWireWord(entry);
        TEST_ASSERT_EQUAL_HEX8(static_cast<uint8_t>(entry >> 8),
                               static_cast<uint8_t>(wire & 0xFF));
        TEST_ASSERT_EQUAL_HEX8(static_cast<uint8_t>(entry & 0xFF),
                               static_cast<uint8_t>(wire >> 8));
        TEST_ASSERT_EQUAL_HEX16(entry, swapWireWord(wire));
    }
}

/**
 * @test A neutral 565 entry keeps its level through the RGB444 reduction
 * @expected (v,v,v) -> 0xLLL for each of the 16 levels, all distinct
 *
 * The 12-bit path stores packRgb565ToRgb444(entry). For a neutral entry whose
 * 5/6/5 fields all carry the same 4-bit level, the reduction must stay neutral
 * and keep exactly that level: a channel shift or a misplaced nibble makes a
 * grey ramp tinted or collapses two levels onto one, which is the failure a
 * palette of near-neutral colours shows first.
 */
void test_palette565_neutral_levels_survive_444(void) {
    bool seen[16] = {false};
    for (int level = 0; level < 16; ++level) {
        const uint8_t v = static_cast<uint8_t>(level * 16 + level);
        const uint16_t rgb565 =
            static_cast<uint16_t>(((v & 0xF8) << 8) | ((v & 0xFC) << 3) | (v >> 3));
        const uint16_t rgb444 = packRgb565ToRgb444(rgb565);
        const uint16_t expected =
            static_cast<uint16_t>((level << 8) | (level << 4) | level);
        TEST_ASSERT_EQUAL_HEX16(expected, rgb444);
        TEST_ASSERT_FALSE_MESSAGE(seen[level], "neutral level collapsed");
        seen[level] = true;
    }
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // packRgb565ToRgb444
    RUN_TEST(test_pack444_black);
    RUN_TEST(test_pack444_white);
    RUN_TEST(test_pack444_full_red);
    RUN_TEST(test_pack444_full_green);
    RUN_TEST(test_pack444_full_blue);
    RUN_TEST(test_pack444_result_is_12_bits);
    RUN_TEST(test_pack444_mixed_colour);

    // packRgb444Pair byte layout
    RUN_TEST(test_pair_layout_red_blue);
    RUN_TEST(test_pair_layout_is_continuous_nibble_stream);
    RUN_TEST(test_pair_layout_duplicated_pixel);
    RUN_TEST(test_pair_writes_exactly_three_bytes);
    RUN_TEST(test_pair_ignores_high_bits);

    // Bijectivity / channel resolution
    RUN_TEST(test_rgb332_to_rgb444_is_bijective);
    RUN_TEST(test_rgb332_red_has_8_levels);
    RUN_TEST(test_rgb332_green_has_8_levels);
    RUN_TEST(test_blue_channel_is_exactly_representable);
    RUN_TEST(test_pair_round_trips_for_all_palette_pairs);

    // constexpr
    RUN_TEST(test_pack444_is_constexpr);

    // 8bpp expansion table transforms (setPalette8)
    RUN_TEST(test_palette565_wire_swap_is_lossless);
    RUN_TEST(test_palette565_neutral_levels_survive_444);

    return UNITY_END();
}
