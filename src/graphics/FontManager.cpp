/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/FontManager.h"
#include "graphics/Renderer.h"  // For Sprite definition

namespace pixelroot32::graphics {

// Static member initialization
const Font* FontManager::defaultFont = nullptr;

void FontManager::setDefaultFont(const Font* font) {
    defaultFont = font;
}

const Font* FontManager::getDefaultFont() {
    return defaultFont;
}

int16_t FontManager::textWidth(const Font* font, const char* text, uint8_t size) {
    if (!text) {
        return 0;
    }
    return textWidth(font, std::string_view(text), size);
}

int16_t FontManager::textWidth(const Font* font, std::string_view text, uint8_t size) {
    if (text.empty()) {
        return 0;
    }

    const Font* activeFont = font ? font : defaultFont;
    if (!activeFont || !activeFont->glyphs) {
        return 0;
    }

    int16_t width = 0;
    
    for (char c : text) {
        if (isCharSupported(c, activeFont)) {
            width += (activeFont->glyphWidth + activeFont->spacing) * size;
        } else {
            // For unsupported characters, use glyph width as fallback
            width += (activeFont->glyphWidth + activeFont->spacing) * size;
        }
    }

    // Subtract last spacing (no spacing after last character)
    if (width > 0) {
        width -= activeFont->spacing * size;
    }

    return width;
}

uint8_t FontManager::getGlyphIndex(char c, const Font* font) {
    const Font* activeFont = font ? font : defaultFont;
    
    if (!activeFont) {
        return 255; // Invalid index
    }

    uint8_t charCode = static_cast<uint8_t>(c);
    
    if (charCode < activeFont->firstChar || charCode > activeFont->lastChar) {
        return 255; // Character out of range
    }

    return charCode - activeFont->firstChar;
}

bool FontManager::isCharSupported(char c, const Font* font) {
    const Font* activeFont = font ? font : defaultFont;
    
    if (!activeFont) {
        return false;
    }

    uint8_t charCode = static_cast<uint8_t>(c);
    return (charCode >= activeFont->firstChar && charCode <= activeFont->lastChar);
}

} // namespace pixelroot32::graphics
