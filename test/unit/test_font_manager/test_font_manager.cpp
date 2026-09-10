/**
 * @file test_font_manager.cpp
 * @brief Unit tests for graphics/FontManager module
 * @version 1.1
 * @date 2026-02-08
 * 
 * Tests for FontManager using real classes.
 */

#include <unity.h>
#include <cstdint>
#include <cstddef>
#include "../../test_config.h"
#include "graphics/FontManager.h"
#include "graphics/Renderer.h"

using namespace pixelroot32::graphics;

// Mock Sprite data
static const uint16_t mockSpriteData[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static const Sprite mockGlyphs[] = {
    {mockSpriteData, 5, 7}
};

// Test font data
static const Font testFont = {mockGlyphs, 32, 126, 5, 7, 1, 8};  // 5x7 font, spacing 1, chars 32-126
static const Font emptyFont = {nullptr, 0, 0, 0, 0, 0, 0};

void setUp(void) {
    test_setup();
    FontManager::setDefaultFont(nullptr);
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for setDefaultFont/getDefaultFont
// =============================================================================

void test_font_manager_set_default_font(void) {
    FontManager::setDefaultFont(&testFont);
    
    TEST_ASSERT_EQUAL(&testFont, FontManager::getDefaultFont());
}

void test_font_manager_get_default_null(void) {
    TEST_ASSERT_NULL(FontManager::getDefaultFont());
}

void test_font_manager_change_default(void) {
    FontManager::setDefaultFont(&testFont);
    FontManager::setDefaultFont(&emptyFont);
    
    TEST_ASSERT_EQUAL(&emptyFont, FontManager::getDefaultFont());
}

// =============================================================================
// Tests for textWidth
// =============================================================================

void test_font_manager_text_width_empty_string(void) {
    int16_t width = FontManager::textWidth(&testFont, "", 1);
    TEST_ASSERT_EQUAL_INT(0, width);
}

void test_font_manager_text_width_null_string(void) {
    int16_t width = FontManager::textWidth(&testFont, nullptr, 1);
    TEST_ASSERT_EQUAL_INT(0, width);
}

void test_font_manager_text_width_single_char(void) {
    // 5 width + 1 spacing = 6, minus spacing at end = 5
    int16_t width = FontManager::textWidth(&testFont, "A", 1);
    TEST_ASSERT_EQUAL_INT(5, width);
}

void test_font_manager_text_width_multiple_chars(void) {
    // "AB": (5+1) + (5+1) - 1 = 11
    int16_t width = FontManager::textWidth(&testFont, "AB", 1);
    TEST_ASSERT_EQUAL_INT(11, width);
}

void test_font_manager_text_width_size_2(void) {
    // "A" at size 2: (5*2 + 1*2) - 1*2 = 10
    int16_t width = FontManager::textWidth(&testFont, "A", 2);
    TEST_ASSERT_EQUAL_INT(10, width);
}

void test_font_manager_text_width_uses_default(void) {
    FontManager::setDefaultFont(&testFont);
    
    int16_t width = FontManager::textWidth(nullptr, "A", 1);
    TEST_ASSERT_EQUAL_INT(5, width);
}

void test_font_manager_text_width_no_font(void) {
    int16_t width = FontManager::textWidth(nullptr, "A", 1);
    TEST_ASSERT_EQUAL_INT(0, width);
}

void test_font_manager_text_width_empty_glyph(void) {
    Font empty = {nullptr, 32, 126, 5, 7, 1, 8};
    int16_t width = FontManager::textWidth(&empty, "A", 1);
    TEST_ASSERT_EQUAL_INT(0, width);
}

// =============================================================================
// Tests for getGlyphIndex
// =============================================================================

void test_font_manager_get_glyph_index_valid(void) {
    // 'A' = 65, firstChar = 32, index = 33
    uint16_t index = FontManager::getGlyphIndex('A', &testFont);
    TEST_ASSERT_EQUAL_UINT8(33, index);
}

void test_font_manager_get_glyph_index_space(void) {
    // ' ' = 32, firstChar = 32, index = 0
    uint16_t index = FontManager::getGlyphIndex(' ', &testFont);
    TEST_ASSERT_EQUAL_UINT8(0, index);
}

void test_font_manager_get_glyph_index_invalid_low(void) {
    // 31 is below firstChar (32)
    uint16_t index = FontManager::getGlyphIndex(31, &testFont);
    // Was TEST_ASSERT_EQUAL_UINT8(255, index) -- passed for the wrong reason
    // after the uint8_t -> uint16_t widening, because uint8_t(0xFFFF) == 0xFF.
    // Asserting the full-width sentinel proves the real contract.
    TEST_ASSERT_EQUAL_UINT16(FontManager::kNoGlyph, index);
}

void test_font_manager_get_glyph_index_invalid_high(void) {
    // 127 is above lastChar (126)
    uint16_t index = FontManager::getGlyphIndex(127, &testFont);
    TEST_ASSERT_EQUAL_UINT16(FontManager::kNoGlyph, index);
}

void test_font_manager_get_glyph_index_no_font(void) {
    uint16_t index = FontManager::getGlyphIndex('A', nullptr);
    TEST_ASSERT_EQUAL_UINT16(FontManager::kNoGlyph, index);
}

void test_font_manager_get_glyph_index_uses_default(void) {
    FontManager::setDefaultFont(&testFont);

    uint16_t index = FontManager::getGlyphIndex('A', nullptr);
    TEST_ASSERT_EQUAL_UINT8(33, index);
}

void test_font_manager_legacy_255_does_not_collide_with_sentinel(void) {
    // A Font whose base range reaches byte 255 legitimately produces glyph
    // index 255 (charCode - firstChar = 255 - 0). Before the uint16_t
    // widening, that valid index was bit-identical to the uint8_t "not found"
    // sentinel (also 255), so a caller had no way to tell them apart. This
    // proves index 255 and FontManager::kNoGlyph are now distinct values.
    static const Sprite wideRangeGlyphs[] = {{mockSpriteData, 5, 7}};
    const Font wideRangeFont = {wideRangeGlyphs, 0, 255, 5, 7, 1, 8};

    uint16_t index = FontManager::getGlyphIndex(static_cast<char>(255), &wideRangeFont);

    TEST_ASSERT_EQUAL_UINT16(255, index);
    TEST_ASSERT_NOT_EQUAL_UINT16(FontManager::kNoGlyph, index);
}

void test_font_manager_out_of_bounds_flash_read_guard(void) {
    // Regression for the exact defect this change fixes: Renderer.cpp used to
    // gate the "unsupported glyph" branch with `if (glyphIndex == 255)`. Once
    // getGlyphIndex returns uint16_t, a stale `255` literal no longer equals
    // the widened sentinel (0xFFFF), so an unsupported codepoint would fall
    // through as "found" and index glyphs[0xFFFF] on a table that only has
    // one entry -- an out-of-bounds flash read. This asserts the real guard
    // value (kNoGlyph), not the old literal, is what a caller must compare
    // against.
    static const Sprite narrowGlyphs[] = {{mockSpriteData, 5, 7}};
    const Font narrowFont = {narrowGlyphs, 0, 0, 5, 7, 1, 8};  // only codepoint 0 is valid

    uint16_t index = FontManager::getGlyphIndex(static_cast<char>(1), &narrowFont);

    TEST_ASSERT_EQUAL_UINT16(FontManager::kNoGlyph, index);
    TEST_ASSERT_NOT_EQUAL_UINT16(255, index);
}

// =============================================================================
// Tests for isCharSupported
// =============================================================================

void test_font_manager_is_char_supported_true(void) {
    TEST_ASSERT_TRUE(FontManager::isCharSupported('A', &testFont));
}

void test_font_manager_is_char_supported_space(void) {
    TEST_ASSERT_TRUE(FontManager::isCharSupported(' ', &testFont));
}

void test_font_manager_is_char_supported_tilde(void) {
    TEST_ASSERT_TRUE(FontManager::isCharSupported('~', &testFont));
}

void test_font_manager_is_char_supported_false_low(void) {
    TEST_ASSERT_FALSE(FontManager::isCharSupported(31, &testFont));
}

void test_font_manager_is_char_supported_false_high(void) {
    TEST_ASSERT_FALSE(FontManager::isCharSupported(127, &testFont));
}

void test_font_manager_is_char_supported_no_font(void) {
    TEST_ASSERT_FALSE(FontManager::isCharSupported('A', nullptr));
}

void test_font_manager_is_char_supported_uses_default(void) {
    FontManager::setDefaultFont(&testFont);
    
    TEST_ASSERT_TRUE(FontManager::isCharSupported('A', nullptr));
}

// =============================================================================
// Tests for special characters
// =============================================================================

void test_font_manager_text_width_with_spaces(void) {
    // "A B": (5+1) + (5+1) + (5+1) - 1 = 17
    int16_t width = FontManager::textWidth(&testFont, "A B", 1);
    TEST_ASSERT_EQUAL_INT(17, width);
}

void test_font_manager_text_width_long_string(void) {
    // "Hello": 5 chars * (5+1) - 1 = 29
    int16_t width = FontManager::textWidth(&testFont, "Hello", 1);
    TEST_ASSERT_EQUAL_INT(29, width);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    RUN_TEST(test_font_manager_set_default_font);
    RUN_TEST(test_font_manager_get_default_null);
    RUN_TEST(test_font_manager_change_default);
    
    RUN_TEST(test_font_manager_text_width_empty_string);
    RUN_TEST(test_font_manager_text_width_null_string);
    RUN_TEST(test_font_manager_text_width_single_char);
    RUN_TEST(test_font_manager_text_width_multiple_chars);
    RUN_TEST(test_font_manager_text_width_size_2);
    RUN_TEST(test_font_manager_text_width_uses_default);
    RUN_TEST(test_font_manager_text_width_no_font);
    RUN_TEST(test_font_manager_text_width_empty_glyph);
    
    RUN_TEST(test_font_manager_get_glyph_index_valid);
    RUN_TEST(test_font_manager_get_glyph_index_space);
    RUN_TEST(test_font_manager_get_glyph_index_invalid_low);
    RUN_TEST(test_font_manager_get_glyph_index_invalid_high);
    RUN_TEST(test_font_manager_get_glyph_index_no_font);
    RUN_TEST(test_font_manager_get_glyph_index_uses_default);
    RUN_TEST(test_font_manager_legacy_255_does_not_collide_with_sentinel);
    RUN_TEST(test_font_manager_out_of_bounds_flash_read_guard);

    RUN_TEST(test_font_manager_is_char_supported_true);
    RUN_TEST(test_font_manager_is_char_supported_space);
    RUN_TEST(test_font_manager_is_char_supported_tilde);
    RUN_TEST(test_font_manager_is_char_supported_false_low);
    RUN_TEST(test_font_manager_is_char_supported_false_high);
    RUN_TEST(test_font_manager_is_char_supported_no_font);
    RUN_TEST(test_font_manager_is_char_supported_uses_default);
    
    RUN_TEST(test_font_manager_text_width_with_spaces);
    RUN_TEST(test_font_manager_text_width_long_string);
    
    return UNITY_END();
}
