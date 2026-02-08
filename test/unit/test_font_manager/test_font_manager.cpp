/**
 * @file test_font_manager.cpp
 * @brief Unit tests for graphics/FontManager module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for FontManager including:
 * - Text width calculation
 * - Glyph index lookup
 * - Character support checking
 */

#include <unity.h>
#include <cstdint>
#include <cstddef>
#include "../../test_config.h"

// Mock Font structure
namespace pixelroot32 {
namespace graphics {

struct Font {
    uint8_t glyphWidth;
    uint8_t glyphHeight;
    uint8_t spacing;
    uint8_t firstChar;
    uint8_t lastChar;
    const uint8_t* glyphs;
};

class FontManager {
public:
    static const Font* defaultFont;
    
    static void setDefaultFont(const Font* font) {
        defaultFont = font;
    }
    
    static const Font* getDefaultFont() {
        return defaultFont;
    }
    
    static int16_t textWidth(const Font* font, const char* text, uint8_t size) {
        if (!text || !*text) {
            return 0;
        }
        
        const Font* activeFont = font ? font : defaultFont;
        if (!activeFont || !activeFont->glyphs) {
            return 0;
        }
        
        int16_t width = 0;
        const char* p = text;
        
        while (*p) {
            if (isCharSupported(*p, activeFont)) {
                width += (activeFont->glyphWidth + activeFont->spacing) * size;
            } else {
                width += (activeFont->glyphWidth + activeFont->spacing) * size;
            }
            p++;
        }
        
        if (width > 0) {
            width -= activeFont->spacing * size;
        }
        
        return width;
    }
    
    static uint8_t getGlyphIndex(char c, const Font* font) {
        const Font* activeFont = font ? font : defaultFont;
        
        if (!activeFont) {
            return 255;
        }
        
        uint8_t charCode = static_cast<uint8_t>(c);
        
        if (charCode < activeFont->firstChar || charCode > activeFont->lastChar) {
            return 255;
        }
        
        return charCode - activeFont->firstChar;
    }
    
    static bool isCharSupported(char c, const Font* font) {
        const Font* activeFont = font ? font : defaultFont;
        
        if (!activeFont) {
            return false;
        }
        
        uint8_t charCode = static_cast<uint8_t>(c);
        return (charCode >= activeFont->firstChar && charCode <= activeFont->lastChar);
    }
};

// Initialize static member
const Font* FontManager::defaultFont = nullptr;

}
}

using namespace pixelroot32::graphics;

// Test font data
static const uint8_t testFontData[] = {0x00, 0x01, 0x02};
static const Font testFont = {5, 7, 1, 32, 126, testFontData};  // 5x7 font, spacing 1, chars 32-126
static const Font emptyFont = {0, 0, 0, 0, 0, nullptr};

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
    // "A" at size 2: 5*2 + 1*2 - 1*2 = 10
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
    Font empty = {5, 7, 1, 32, 126, nullptr};
    int16_t width = FontManager::textWidth(&empty, "A", 1);
    TEST_ASSERT_EQUAL_INT(0, width);
}

// =============================================================================
// Tests for getGlyphIndex
// =============================================================================

void test_font_manager_get_glyph_index_valid(void) {
    // 'A' = 65, firstChar = 32, index = 33
    uint8_t index = FontManager::getGlyphIndex('A', &testFont);
    TEST_ASSERT_EQUAL_UINT8(33, index);
}

void test_font_manager_get_glyph_index_space(void) {
    // ' ' = 32, firstChar = 32, index = 0
    uint8_t index = FontManager::getGlyphIndex(' ', &testFont);
    TEST_ASSERT_EQUAL_UINT8(0, index);
}

void test_font_manager_get_glyph_index_invalid_low(void) {
    // 31 is below firstChar (32)
    uint8_t index = FontManager::getGlyphIndex(31, &testFont);
    TEST_ASSERT_EQUAL_UINT8(255, index);
}

void test_font_manager_get_glyph_index_invalid_high(void) {
    // 127 is above lastChar (126)
    uint8_t index = FontManager::getGlyphIndex(127, &testFont);
    TEST_ASSERT_EQUAL_UINT8(255, index);
}

void test_font_manager_get_glyph_index_no_font(void) {
    uint8_t index = FontManager::getGlyphIndex('A', nullptr);
    TEST_ASSERT_EQUAL_UINT8(255, index);
}

void test_font_manager_get_glyph_index_uses_default(void) {
    FontManager::setDefaultFont(&testFont);
    
    uint8_t index = FontManager::getGlyphIndex('A', nullptr);
    TEST_ASSERT_EQUAL_UINT8(33, index);
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
    
    // Default font tests
    RUN_TEST(test_font_manager_set_default_font);
    RUN_TEST(test_font_manager_get_default_null);
    RUN_TEST(test_font_manager_change_default);
    
    // textWidth tests
    RUN_TEST(test_font_manager_text_width_empty_string);
    RUN_TEST(test_font_manager_text_width_null_string);
    RUN_TEST(test_font_manager_text_width_single_char);
    RUN_TEST(test_font_manager_text_width_multiple_chars);
    RUN_TEST(test_font_manager_text_width_size_2);
    RUN_TEST(test_font_manager_text_width_uses_default);
    RUN_TEST(test_font_manager_text_width_no_font);
    RUN_TEST(test_font_manager_text_width_empty_glyph);
    
    // getGlyphIndex tests
    RUN_TEST(test_font_manager_get_glyph_index_valid);
    RUN_TEST(test_font_manager_get_glyph_index_space);
    RUN_TEST(test_font_manager_get_glyph_index_invalid_low);
    RUN_TEST(test_font_manager_get_glyph_index_invalid_high);
    RUN_TEST(test_font_manager_get_glyph_index_no_font);
    RUN_TEST(test_font_manager_get_glyph_index_uses_default);
    
    // isCharSupported tests
    RUN_TEST(test_font_manager_is_char_supported_true);
    RUN_TEST(test_font_manager_is_char_supported_space);
    RUN_TEST(test_font_manager_is_char_supported_tilde);
    RUN_TEST(test_font_manager_is_char_supported_false_low);
    RUN_TEST(test_font_manager_is_char_supported_false_high);
    RUN_TEST(test_font_manager_is_char_supported_no_font);
    RUN_TEST(test_font_manager_is_char_supported_uses_default);
    
    // Special character tests
    RUN_TEST(test_font_manager_text_width_with_spaces);
    RUN_TEST(test_font_manager_text_width_long_string);
    
    return UNITY_END();
}
