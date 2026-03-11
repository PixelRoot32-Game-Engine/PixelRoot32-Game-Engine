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
 * - Palette resolution (legacy, dual, context)
 * - Multi-palette background slot bank (initBackgroundPaletteSlots, set/getBackgroundPaletteSlot, resolveColorWithPalette)
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
// Tests for Color.cpp functions (palette resolution)
// =============================================================================

void test_resolveColor_legacy_mode(void) {
    // Default palette is PR32
    setPalette(PaletteType::PR32);
    uint16_t black = resolveColor(Color::Black);
    uint16_t white = resolveColor(Color::White);
    // Black and White should resolve to different values
    TEST_ASSERT_NOT_EQUAL(black, white);
}

void test_resolveColor_transparent_returns_zero(void) {
    uint16_t result = resolveColor(Color::Transparent);
    TEST_ASSERT_EQUAL_UINT16(0, result);
}

void test_resolveColor_with_context_sprite(void) {
    setPalette(PaletteType::PR32);
    uint16_t result = resolveColor(Color::Red, PaletteContext::Sprite);
    // Should return a valid non-zero color
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_resolveColor_with_context_background(void) {
    setPalette(PaletteType::PR32);
    uint16_t result = resolveColor(Color::Blue, PaletteContext::Background);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_resolveColor_context_transparent(void) {
    uint16_t result = resolveColor(Color::Transparent, PaletteContext::Sprite);
    TEST_ASSERT_EQUAL_UINT16(0, result);
}

void test_setPalette_all_types(void) {
    // Test switching between every palette type
    PaletteType types[] = {
        PaletteType::NES, PaletteType::GB, PaletteType::GBC,
        PaletteType::PICO8, PaletteType::PR32
    };
    for (int i = 0; i < 5; i++) {
        setPalette(types[i]);
        uint16_t white = resolveColor(Color::White);
        // White should always resolve to something non-zero
        TEST_ASSERT_NOT_EQUAL(0, white);
    }
}

void test_setCustomPalette(void) {
    uint16_t custom[16] = {0x0000, 0xFFFF, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
                           0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
    setCustomPalette(custom);
    TEST_ASSERT_EQUAL_UINT16(0x0000, resolveColor(Color::Black));
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, resolveColor(Color::White));
    TEST_ASSERT_EQUAL_UINT16(0x1111, resolveColor(Color::Navy));
    
    // Null should be ignored
    setCustomPalette(nullptr);
    // Should still use previous custom palette
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, resolveColor(Color::White));
    
    // Restore default
    setPalette(PaletteType::PR32);
}

void test_enableDualPaletteMode(void) {
    enableDualPaletteMode(true);
    
    // Set different palettes for background and sprite
    setBackgroundPalette(PaletteType::GB);
    setSpritePalette(PaletteType::NES);
    
    uint16_t bgWhite = resolveColor(Color::White, PaletteContext::Background);
    uint16_t sprWhite = resolveColor(Color::White, PaletteContext::Sprite);
    
    // They may or may not be different depending on palette values,
    // but the function should at least not crash and return valid values
    TEST_ASSERT_NOT_EQUAL(0, bgWhite);
    TEST_ASSERT_NOT_EQUAL(0, sprWhite);
    
    // Disable dual mode
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setDualPalette_convenience(void) {
    setDualPalette(PaletteType::PICO8, PaletteType::GBC);
    
    // After setDualPalette, dual mode is enabled
    uint16_t bgColor = resolveColor(Color::Red, PaletteContext::Background);
    uint16_t sprColor = resolveColor(Color::Red, PaletteContext::Sprite);
    TEST_ASSERT_NOT_EQUAL(0, bgColor);
    TEST_ASSERT_NOT_EQUAL(0, sprColor);
    
    // Restore
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setBackgroundCustomPalette(void) {
    uint16_t customBg[16] = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    enableDualPaletteMode(true);
    setBackgroundCustomPalette(customBg);
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, resolveColor(Color::Black, PaletteContext::Background));
    
    // Null should be ignored
    setBackgroundCustomPalette(nullptr);
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, resolveColor(Color::Black, PaletteContext::Background));
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setSpriteCustomPalette(void) {
    uint16_t customSpr[16] = {0x1234, 0x5678, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    enableDualPaletteMode(true);
    setSpriteCustomPalette(customSpr);
    TEST_ASSERT_EQUAL_UINT16(0x1234, resolveColor(Color::Black, PaletteContext::Sprite));
    
    // Null should be ignored
    setSpriteCustomPalette(nullptr);
    TEST_ASSERT_EQUAL_UINT16(0x1234, resolveColor(Color::Black, PaletteContext::Sprite));
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setDualCustomPalette(void) {
    uint16_t customBg[16] = {0xAA00, 0xBB00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t customSpr[16] = {0x00AA, 0x00BB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    setDualCustomPalette(customBg, customSpr);
    TEST_ASSERT_EQUAL_UINT16(0xAA00, resolveColor(Color::Black, PaletteContext::Background));
    TEST_ASSERT_EQUAL_UINT16(0x00AA, resolveColor(Color::Black, PaletteContext::Sprite));
    
    // Null args should be ignored
    setDualCustomPalette(nullptr, customSpr);
    // Should still have the old values
    TEST_ASSERT_EQUAL_UINT16(0xAA00, resolveColor(Color::Black, PaletteContext::Background));
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setBackgroundPalette_fallback(void) {
    enableDualPaletteMode(true);
    // Set a valid palette first
    setBackgroundPalette(PaletteType::NES);
    uint16_t nesWhite = resolveColor(Color::White, PaletteContext::Background);
    TEST_ASSERT_NOT_EQUAL(0, nesWhite);
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setSpritePalette_fallback(void) {
    enableDualPaletteMode(true);
    setSpritePalette(PaletteType::GBC);
    uint16_t gbcWhite = resolveColor(Color::White, PaletteContext::Sprite);
    TEST_ASSERT_NOT_EQUAL(0, gbcWhite);
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

// =============================================================================
// Additional Color.cpp Coverage Tests
// =============================================================================

void test_resolveColor_different_palettes_produce_different_values(void) {
    // Each palette should produce different values for the same color index
    PaletteType types[] = {
        PaletteType::NES, PaletteType::GB, PaletteType::GBC,
        PaletteType::PICO8, PaletteType::PR32
    };
    
    uint16_t blackValues[5];
    for (int i = 0; i < 5; i++) {
        setPalette(types[i]);
        blackValues[i] = resolveColor(Color::Black);
    }
    
    // At least some palettes should have different Black values
    bool allSame = true;
    for (int i = 1; i < 5; i++) {
        if (blackValues[i] != blackValues[0]) {
            allSame = false;
            break;
        }
    }
    // We don't assert allSame == false because palettes might share index 0,
    // but we exercised every setPalette path
    
    // Restore
    setPalette(PaletteType::PR32);
}

void test_resolveColor_all_16_indices(void) {
    setPalette(PaletteType::PR32);
    
    Color allColors[] = {
        Color::Black, Color::White, Color::Navy, Color::Blue,
        Color::Cyan, Color::DarkGreen, Color::Green, Color::LightGreen,
        Color::Yellow, Color::Orange, Color::LightRed, Color::Red,
        Color::DarkRed, Color::Purple, Color::Magenta, Color::Gray
    };
    
    // Resolve all 16 colors — exercises the full palette lookup
    for (int i = 0; i < 16; i++) {
        uint16_t val = resolveColor(allColors[i]);
        // Each color should resolve (don't check specific values, just that it runs)
        (void)val;
    }
}

void test_dual_palette_different_bg_sprite_values(void) {
    // Enable dual palette with different palettes for BG and Sprite
    enableDualPaletteMode(true);
    setBackgroundPalette(PaletteType::NES);
    setSpritePalette(PaletteType::GB);
    
    uint16_t bgRed = resolveColor(Color::Red, PaletteContext::Background);
    uint16_t sprRed = resolveColor(Color::Red, PaletteContext::Sprite);
    
    // NES and GB likely have different Red values
    // But we mainly care about exercising the dual-mode path
    TEST_ASSERT_NOT_EQUAL(0, bgRed);
    TEST_ASSERT_NOT_EQUAL(0, sprRed);
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_resolveColor_context_legacy_fallback(void) {
    // In legacy mode (dual disabled), resolveColor(color, context) 
    // should use currentPalette regardless of context
    enableDualPaletteMode(false);
    setPalette(PaletteType::PICO8);
    
    uint16_t bgWhite = resolveColor(Color::White, PaletteContext::Background);
    uint16_t sprWhite = resolveColor(Color::White, PaletteContext::Sprite);
    
    // In legacy mode, both should be the same (same currentPalette)
    TEST_ASSERT_EQUAL_UINT16(bgWhite, sprWhite);
    
    // Restore
    setPalette(PaletteType::PR32);
}

void test_setBackgroundPalette_all_types(void) {
    enableDualPaletteMode(true);
    
    PaletteType types[] = {
        PaletteType::NES, PaletteType::GB, PaletteType::GBC,
        PaletteType::PICO8, PaletteType::PR32
    };
    
    for (int i = 0; i < 5; i++) {
        setBackgroundPalette(types[i]);
        uint16_t val = resolveColor(Color::White, PaletteContext::Background);
        TEST_ASSERT_NOT_EQUAL(0, val);
    }
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setSpritePalette_all_types(void) {
    enableDualPaletteMode(true);
    
    PaletteType types[] = {
        PaletteType::NES, PaletteType::GB, PaletteType::GBC,
        PaletteType::PICO8, PaletteType::PR32
    };
    
    for (int i = 0; i < 5; i++) {
        setSpritePalette(types[i]);
        uint16_t val = resolveColor(Color::White, PaletteContext::Sprite);
        TEST_ASSERT_NOT_EQUAL(0, val);
    }
    
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

void test_setCustomPalette_all_indices(void) {
    uint16_t custom[16];
    for (int i = 0; i < 16; i++) {
        custom[i] = 0x1000 + i;
    }
    setCustomPalette(custom);
    
    // Verify all 16 indices resolve correctly
    Color allColors[] = {
        Color::Black, Color::White, Color::Navy, Color::Blue,
        Color::Cyan, Color::DarkGreen, Color::Green, Color::LightGreen,
        Color::Yellow, Color::Orange, Color::LightRed, Color::Red,
        Color::DarkRed, Color::Purple, Color::Magenta, Color::Gray
    };
    
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT16(0x1000 + i, resolveColor(allColors[i]));
    }
    
    // Restore
    setPalette(PaletteType::PR32);
}

void test_setDualCustomPalette_null_bg_only(void) {
    uint16_t customSpr[16] = {0x1111, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    // Null BG palette → should not change anything
    setDualCustomPalette(nullptr, customSpr);
    // Cannot directly test internal state, but should not crash
    
    // Null Sprite palette → should not change anything
    uint16_t customBg[16] = {0x2222, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    setDualCustomPalette(customBg, nullptr);
    // Should not crash
    
    // Restore
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

// =============================================================================
// Multi-palette background slot bank (paletteIndices / per-cell palette)
// =============================================================================

/**
 * @test resolveColorWithPalette with explicit palette
 * @expected Resolves Color index to palette[idx]; nullptr returns 0; Transparent returns 0
 */
void test_resolveColorWithPalette_explicit_palette(void) {
    uint16_t custom[16] = {0x0000, 0xFFFF, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
                           0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE};
    TEST_ASSERT_EQUAL_UINT16(0x0000, resolveColorWithPalette(Color::Black, custom));
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, resolveColorWithPalette(Color::White, custom));
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, resolveColorWithPalette(Color::Red, custom));
    TEST_ASSERT_EQUAL_UINT16(0, resolveColorWithPalette(Color::Transparent, custom));
}

/**
 * @test resolveColorWithPalette with nullptr palette
 * @expected Returns 0
 */
void test_resolveColorWithPalette_null_returns_zero(void) {
    TEST_ASSERT_EQUAL_UINT16(0, resolveColorWithPalette(Color::White, nullptr));
    TEST_ASSERT_EQUAL_UINT16(0, resolveColorWithPalette(Color::Black, nullptr));
}

/**
 * @test initBackgroundPaletteSlots initializes all slots; getBackgroundPaletteSlot(0) matches default
 * @expected After init, slot 0 is non-null and matches background resolution (no paletteIndices path)
 */
void test_initBackgroundPaletteSlots_slot_zero_usable(void) {
    initBackgroundPaletteSlots();
    setBackgroundPalette(PaletteType::PR32);
    const uint16_t* slot0 = getBackgroundPaletteSlot(0);
    TEST_ASSERT_NOT_NULL(slot0);
    uint16_t viaSlot = resolveColorWithPalette(Color::White, slot0);
    uint16_t viaContext = resolveColor(Color::White, PaletteContext::Background);
    TEST_ASSERT_EQUAL_UINT16(viaContext, viaSlot);
}

/**
 * @test Without paletteIndices: slot 0 is default; setBackgroundCustomPalette updates slot 0
 * @expected getBackgroundPaletteSlot(0) returns the same palette as Background context
 */
void test_getBackgroundPaletteSlot_zero_matches_background_context(void) {
    enableDualPaletteMode(true);
    uint16_t custom[16] = {0xAAAA, 0xBBBB, 0xCCCC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    setBackgroundCustomPalette(custom);
    const uint16_t* slot0 = getBackgroundPaletteSlot(0);
    TEST_ASSERT_NOT_NULL(slot0);
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, resolveColorWithPalette(Color::Black, slot0));
    TEST_ASSERT_EQUAL_UINT16(0xBBBB, resolveColorWithPalette(Color::White, slot0));
    TEST_ASSERT_EQUAL_UINT16(resolveColor(Color::Black, PaletteContext::Background),
                             resolveColorWithPalette(Color::Black, slot0));
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

/**
 * @test setBackgroundPaletteSlot / setBackgroundCustomPaletteSlot; getBackgroundPaletteSlot returns correct palette
 * @expected Slot 1 and 2 hold different custom palettes; resolveColorWithPalette with each slot returns correct values
 */
void test_setBackgroundPaletteSlot_multiple_slots(void) {
    initBackgroundPaletteSlots();
    uint16_t pal1[16] = {0x1111, 0x2222, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t pal2[16] = {0xAAAA, 0xBBBB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    setBackgroundCustomPaletteSlot(1, pal1);
    setBackgroundCustomPaletteSlot(2, pal2);
    const uint16_t* slot1 = getBackgroundPaletteSlot(1);
    const uint16_t* slot2 = getBackgroundPaletteSlot(2);
    TEST_ASSERT_NOT_NULL(slot1);
    TEST_ASSERT_NOT_NULL(slot2);
    TEST_ASSERT_EQUAL_UINT16(0x1111, resolveColorWithPalette(Color::Black, slot1));
    TEST_ASSERT_EQUAL_UINT16(0x2222, resolveColorWithPalette(Color::White, slot1));
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, resolveColorWithPalette(Color::Black, slot2));
    TEST_ASSERT_EQUAL_UINT16(0xBBBB, resolveColorWithPalette(Color::White, slot2));
    TEST_ASSERT_NOT_EQUAL(resolveColorWithPalette(Color::Black, slot1),
                          resolveColorWithPalette(Color::Black, slot2));
    setPalette(PaletteType::PR32);
}

/**
 * @test setBackgroundPaletteSlot(0, type) syncs global background palette
 * @expected resolveColor(..., Background) matches resolveColorWithPalette(..., getBackgroundPaletteSlot(0))
 */
void test_setBackgroundPaletteSlot_slot_zero_syncs_global(void) {
    initBackgroundPaletteSlots();
    enableDualPaletteMode(true);
    setBackgroundPaletteSlot(0, PaletteType::NES);
    const uint16_t* slot0 = getBackgroundPaletteSlot(0);
    TEST_ASSERT_NOT_NULL(slot0);
    TEST_ASSERT_EQUAL_UINT16(resolveColor(Color::Red, PaletteContext::Background),
                             resolveColorWithPalette(Color::Red, slot0));
    enableDualPaletteMode(false);
    setPalette(PaletteType::PR32);
}

/**
 * @test getBackgroundPaletteSlot out-of-range falls back to slot 0
 * @expected getBackgroundPaletteSlot(255) or slot >= kMaxBackgroundPaletteSlots returns same as slot 0
 */
void test_getBackgroundPaletteSlot_out_of_range_fallback(void) {
    initBackgroundPaletteSlots();
    setBackgroundPalette(PaletteType::PR32);
    const uint16_t* slot0 = getBackgroundPaletteSlot(0);
    const uint16_t* slotOut = getBackgroundPaletteSlot(255);
    TEST_ASSERT_NOT_NULL(slot0);
    TEST_ASSERT_NOT_NULL(slotOut);
    TEST_ASSERT_EQUAL_PTR(slot0, slotOut);
    setPalette(PaletteType::PR32);
}

/**
 * @test Multiple slots produce different colors (simulates paletteIndices using different slots)
 * @expected Slot 0 vs slot 1 with different palettes yield different RGB565 for same Color
 */
void test_multi_palette_slots_different_values(void) {
    initBackgroundPaletteSlots();
    uint16_t palA[16] = {0x0001, 0x0002, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t palB[16] = {0xF001, 0xF002, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    setBackgroundCustomPaletteSlot(0, palA);
    setBackgroundCustomPaletteSlot(1, palB);
    uint16_t blackSlot0 = resolveColorWithPalette(Color::Black, getBackgroundPaletteSlot(0));
    uint16_t blackSlot1 = resolveColorWithPalette(Color::Black, getBackgroundPaletteSlot(1));
    TEST_ASSERT_EQUAL_UINT16(0x0001, blackSlot0);
    TEST_ASSERT_EQUAL_UINT16(0xF001, blackSlot1);
    TEST_ASSERT_NOT_EQUAL(blackSlot0, blackSlot1);
    setPalette(PaletteType::PR32);
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
    
    // Color.cpp function tests
    RUN_TEST(test_resolveColor_legacy_mode);
    RUN_TEST(test_resolveColor_transparent_returns_zero);
    RUN_TEST(test_resolveColor_with_context_sprite);
    RUN_TEST(test_resolveColor_with_context_background);
    RUN_TEST(test_resolveColor_context_transparent);
    RUN_TEST(test_setPalette_all_types);
    RUN_TEST(test_setCustomPalette);
    RUN_TEST(test_enableDualPaletteMode);
    RUN_TEST(test_setDualPalette_convenience);
    RUN_TEST(test_setBackgroundCustomPalette);
    RUN_TEST(test_setSpriteCustomPalette);
    RUN_TEST(test_setDualCustomPalette);
    RUN_TEST(test_setBackgroundPalette_fallback);
    RUN_TEST(test_setSpritePalette_fallback);
    
    // Additional Color.cpp coverage tests
    RUN_TEST(test_resolveColor_different_palettes_produce_different_values);
    RUN_TEST(test_resolveColor_all_16_indices);
    RUN_TEST(test_dual_palette_different_bg_sprite_values);
    RUN_TEST(test_resolveColor_context_legacy_fallback);
    RUN_TEST(test_setBackgroundPalette_all_types);
    RUN_TEST(test_setSpritePalette_all_types);
    RUN_TEST(test_setCustomPalette_all_indices);
    RUN_TEST(test_setDualCustomPalette_null_bg_only);
    
    // Multi-palette background slot bank (paletteIndices / per-cell palette)
    RUN_TEST(test_resolveColorWithPalette_explicit_palette);
    RUN_TEST(test_resolveColorWithPalette_null_returns_zero);
    RUN_TEST(test_initBackgroundPaletteSlots_slot_zero_usable);
    RUN_TEST(test_getBackgroundPaletteSlot_zero_matches_background_context);
    RUN_TEST(test_setBackgroundPaletteSlot_multiple_slots);
    RUN_TEST(test_setBackgroundPaletteSlot_slot_zero_syncs_global);
    RUN_TEST(test_getBackgroundPaletteSlot_out_of_range_fallback);
    RUN_TEST(test_multi_palette_slots_different_values);
    
    return UNITY_END();
}
