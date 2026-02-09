/**
 * @file test_color.cpp
 * @brief Unit tests for graphics/Color module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for Color system including:
 * - Color enum values
 * - Palette type definitions
 * - Color constants
 */

#include <unity.h>
#include "graphics/Color.h"
#include "../../test_config.h"

using namespace pixelroot32::graphics;

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
// Tests for Color enum - Basic colors
// =============================================================================

/**
 * @test Black color value
 * @expected Black = 0
 */
void test_color_black(void) {
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(Color::Black));
}

/**
 * @test White color value
 * @expected White = 1
 */
void test_color_white(void) {
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(Color::White));
}

/**
 * @test Navy color value
 * @expected Navy = 2
 */
void test_color_navy(void) {
    TEST_ASSERT_EQUAL_UINT8(2, static_cast<uint8_t>(Color::Navy));
}

/**
 * @test Blue color value
 * @expected Blue = 3
 */
void test_color_blue(void) {
    TEST_ASSERT_EQUAL_UINT8(3, static_cast<uint8_t>(Color::Blue));
}

/**
 * @test Cyan color value
 * @expected Cyan = 4
 */
void test_color_cyan(void) {
    TEST_ASSERT_EQUAL_UINT8(4, static_cast<uint8_t>(Color::Cyan));
}

/**
 * @test DarkGreen color value
 * @expected DarkGreen = 5
 */
void test_color_dark_green(void) {
    TEST_ASSERT_EQUAL_UINT8(5, static_cast<uint8_t>(Color::DarkGreen));
}

/**
 * @test Green color value
 * @expected Green = 6
 */
void test_color_green(void) {
    TEST_ASSERT_EQUAL_UINT8(6, static_cast<uint8_t>(Color::Green));
}

/**
 * @test LightGreen color value
 * @expected LightGreen = 7
 */
void test_color_light_green(void) {
    TEST_ASSERT_EQUAL_UINT8(7, static_cast<uint8_t>(Color::LightGreen));
}

/**
 * @test Yellow color value
 * @expected Yellow = 8
 */
void test_color_yellow(void) {
    TEST_ASSERT_EQUAL_UINT8(8, static_cast<uint8_t>(Color::Yellow));
}

/**
 * @test Orange color value
 * @expected Orange = 9
 */
void test_color_orange(void) {
    TEST_ASSERT_EQUAL_UINT8(9, static_cast<uint8_t>(Color::Orange));
}

/**
 * @test LightRed color value
 * @expected LightRed = 10
 */
void test_color_light_red(void) {
    TEST_ASSERT_EQUAL_UINT8(10, static_cast<uint8_t>(Color::LightRed));
}

/**
 * @test Red color value
 * @expected Red = 11
 */
void test_color_red(void) {
    TEST_ASSERT_EQUAL_UINT8(11, static_cast<uint8_t>(Color::Red));
}

/**
 * @test DarkRed color value
 * @expected DarkRed = 12
 */
void test_color_dark_red(void) {
    TEST_ASSERT_EQUAL_UINT8(12, static_cast<uint8_t>(Color::DarkRed));
}

/**
 * @test Purple color value
 * @expected Purple = 13
 */
void test_color_purple(void) {
    TEST_ASSERT_EQUAL_UINT8(13, static_cast<uint8_t>(Color::Purple));
}

/**
 * @test Magenta color value
 * @expected Magenta = 14
 */
void test_color_magenta(void) {
    TEST_ASSERT_EQUAL_UINT8(14, static_cast<uint8_t>(Color::Magenta));
}

/**
 * @test Gray color value
 * @expected Gray = 15
 */
void test_color_gray(void) {
    TEST_ASSERT_EQUAL_UINT8(15, static_cast<uint8_t>(Color::Gray));
}

// =============================================================================
// Tests for Color enum - Aliases
// =============================================================================

/**
 * @test DarkBlue is alias for Navy
 * @expected DarkBlue == Navy
 */
void test_color_darkblue_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Navy), 
                      static_cast<uint8_t>(Color::DarkBlue));
}

/**
 * @test LightBlue is alias for Blue
 * @expected LightBlue == Blue
 */
void test_color_lightblue_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Blue), 
                      static_cast<uint8_t>(Color::LightBlue));
}

/**
 * @test Teal is alias for Cyan
 * @expected Teal == Cyan
 */
void test_color_teal_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Cyan), 
                      static_cast<uint8_t>(Color::Teal));
}

/**
 * @test Olive is alias for DarkGreen
 * @expected Olive == DarkGreen
 */
void test_color_olive_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::DarkGreen), 
                      static_cast<uint8_t>(Color::Olive));
}

/**
 * @test Gold is alias for Yellow
 * @expected Gold == Yellow
 */
void test_color_gold_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Yellow), 
                      static_cast<uint8_t>(Color::Gold));
}

/**
 * @test Brown is alias for DarkRed
 * @expected Brown == DarkRed
 */
void test_color_brown_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::DarkRed), 
                      static_cast<uint8_t>(Color::Brown));
}

/**
 * @test Pink is alias for Magenta
 * @expected Pink == Magenta
 */
void test_color_pink_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Magenta), 
                      static_cast<uint8_t>(Color::Pink));
}

/**
 * @test LightPurple is alias for Magenta
 * @expected LightPurple == Magenta
 */
void test_color_lightpurple_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Magenta), 
                      static_cast<uint8_t>(Color::LightPurple));
}

/**
 * @test Maroon is alias for DarkRed
 * @expected Maroon == DarkRed
 */
void test_color_maroon_alias(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::DarkRed), 
                      static_cast<uint8_t>(Color::Maroon));
}

/**
 * @test Various gray aliases are all Gray
 * @expected MidGray, LightGray, DarkGray, Silver == Gray
 */
void test_color_gray_aliases(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Gray), 
                      static_cast<uint8_t>(Color::MidGray));
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Gray), 
                      static_cast<uint8_t>(Color::LightGray));
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Gray), 
                      static_cast<uint8_t>(Color::DarkGray));
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Gray), 
                      static_cast<uint8_t>(Color::Silver));
}

// =============================================================================
// Tests for Color enum - Special colors
// =============================================================================

/**
 * @test Transparent color value
 * @expected Transparent = 255
 */
void test_color_transparent(void) {
    TEST_ASSERT_EQUAL_UINT8(255, static_cast<uint8_t>(Color::Transparent));
}

/**
 * @test DebugRed is alias for Red
 * @expected DebugRed == Red
 */
void test_color_debug_red(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Red), 
                      static_cast<uint8_t>(Color::DebugRed));
}

/**
 * @test DebugGreen is alias for Green
 * @expected DebugGreen == Green
 */
void test_color_debug_green(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Green), 
                      static_cast<uint8_t>(Color::DebugGreen));
}

/**
 * @test DebugBlue is alias for Blue
 * @expected DebugBlue == Blue
 */
void test_color_debug_blue(void) {
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Color::Blue), 
                      static_cast<uint8_t>(Color::DebugBlue));
}

// =============================================================================
// Tests for PaletteType enum
// =============================================================================

/**
 * @test PaletteType enum values
 * @expected Each palette type has unique value
 */
void test_palette_type_values(void) {
    // Just verify they compile and are distinct
    PaletteType palettes[] = {
        PaletteType::NES,
        PaletteType::GB,
        PaletteType::GBC,
        PaletteType::PICO8,
        PaletteType::PR32
    };
    
    // Check all are distinct (simple check)
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT_NOT_EQUAL(static_cast<int>(palettes[i]), 
                                  static_cast<int>(palettes[j]));
        }
    }
}

// =============================================================================
// Tests for PaletteContext enum
// =============================================================================

/**
 * @test PaletteContext values
 * @expected Background = 0, Sprite = 1
 */
void test_palette_context_values(void) {
    TEST_ASSERT_EQUAL(0, static_cast<int>(PaletteContext::Background));
    TEST_ASSERT_EQUAL(1, static_cast<int>(PaletteContext::Sprite));
}

// =============================================================================
// Tests for Constants
// =============================================================================

/**
 * @test PALETTE_SIZE constant
 * @expected PALETTE_SIZE = 16
 */
void test_palette_size_constant(void) {
    TEST_ASSERT_EQUAL_UINT8(16, PALETTE_SIZE);
}

/**
 * @test Color enum fits in uint8_t
 * @expected All values 0-15 fit in uint8_t
 */
void test_color_fits_in_uint8(void) {
    TEST_ASSERT_EQUAL_INT(1, sizeof(Color));
}

/**
 * @test All standard colors are within palette size
 * @expected All standard colors < 16
 */
void test_standard_colors_within_palette(void) {
    Color standardColors[] = {
        Color::Black, Color::White, Color::Navy, Color::Blue,
        Color::Cyan, Color::DarkGreen, Color::Green, Color::LightGreen,
        Color::Yellow, Color::Orange, Color::LightRed, Color::Red,
        Color::DarkRed, Color::Purple, Color::Magenta, Color::Gray
    };
    
    for (int i = 0; i < 16; i++) {
        uint8_t value = static_cast<uint8_t>(standardColors[i]);
        TEST_ASSERT_LESS_THAN_UINT8(16, value);
    }
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    // Basic color tests
    RUN_TEST(test_color_black);
    RUN_TEST(test_color_white);
    RUN_TEST(test_color_navy);
    RUN_TEST(test_color_blue);
    RUN_TEST(test_color_cyan);
    RUN_TEST(test_color_dark_green);
    RUN_TEST(test_color_green);
    RUN_TEST(test_color_light_green);
    RUN_TEST(test_color_yellow);
    RUN_TEST(test_color_orange);
    RUN_TEST(test_color_light_red);
    RUN_TEST(test_color_red);
    RUN_TEST(test_color_dark_red);
    RUN_TEST(test_color_purple);
    RUN_TEST(test_color_magenta);
    RUN_TEST(test_color_gray);
    
    // Alias tests
    RUN_TEST(test_color_darkblue_alias);
    RUN_TEST(test_color_lightblue_alias);
    RUN_TEST(test_color_teal_alias);
    RUN_TEST(test_color_olive_alias);
    RUN_TEST(test_color_gold_alias);
    RUN_TEST(test_color_brown_alias);
    RUN_TEST(test_color_pink_alias);
    RUN_TEST(test_color_lightpurple_alias);
    RUN_TEST(test_color_maroon_alias);
    RUN_TEST(test_color_gray_aliases);
    
    // Special color tests
    RUN_TEST(test_color_transparent);
    RUN_TEST(test_color_debug_red);
    RUN_TEST(test_color_debug_green);
    RUN_TEST(test_color_debug_blue);
    
    // Palette tests
    RUN_TEST(test_palette_type_values);
    RUN_TEST(test_palette_context_values);
    
    // Constant tests
    RUN_TEST(test_palette_size_constant);
    RUN_TEST(test_color_fits_in_uint8);
    RUN_TEST(test_standard_colors_within_palette);
    
    return UNITY_END();
}
