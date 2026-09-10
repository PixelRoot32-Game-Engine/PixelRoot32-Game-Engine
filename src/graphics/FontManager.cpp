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

    // Drive the same decoder Renderer::drawText uses: one cell per decoded
    // glyph (base or supplement), never one per byte, so measurement can
    // never disagree with what is actually drawn.
    for (size_t i = 0; i < text.size();) {
        const GlyphStep step = nextGlyph(text, i, activeFont);
        i += step.bytes;
        width += (activeFont->glyphWidth + activeFont->spacing) * size;
    }

    // Subtract last spacing (no spacing after last character)
    if (width > 0) {
        width -= activeFont->spacing * size;
    }

    return width;
}

uint16_t FontManager::getGlyphIndex(char c, const Font* font) {
    const Font* activeFont = font ? font : defaultFont;

    if (!activeFont) {
        return kNoGlyph; // Invalid index
    }

    const uint8_t charCode = static_cast<uint8_t>(c);

    if (charCode >= activeFont->firstChar && charCode <= activeFont->lastChar) {
        return static_cast<uint16_t>(charCode - activeFont->firstChar);
    }

    if (activeFont->extGlyphs != nullptr &&
        charCode >= activeFont->extFirstChar && charCode <= activeFont->extLastChar) {
        return static_cast<uint16_t>(charCode - activeFont->extFirstChar);
    }

    return kNoGlyph; // Character out of range
}

bool FontManager::isCharSupported(char c, const Font* font) {
    const Font* activeFont = font ? font : defaultFont;

    if (!activeFont) {
        return false;
    }

    uint8_t charCode = static_cast<uint8_t>(c);
    return (charCode >= activeFont->firstChar && charCode <= activeFont->lastChar);
}

bool FontManager::isCodepointSupported(uint16_t codepoint, const Font* font) {
    const Font* activeFont = font ? font : defaultFont;
    if (!activeFont) {
        return false;
    }

    if (codepoint >= activeFont->firstChar && codepoint <= activeFont->lastChar) {
        return true;
    }
    return activeFont->extGlyphs != nullptr &&
        codepoint >= activeFont->extFirstChar && codepoint <= activeFont->extLastChar;
}

FontManager::GlyphStep FontManager::nextGlyph(std::string_view text, size_t pos, const Font* font) {
    const Font* activeFont = font ? font : defaultFont;

    if (pos >= text.size()) {
        return {kNoGlyph, 1, false};
    }

    const uint8_t lead = static_cast<uint8_t>(text[pos]);

    if (lead < 0x80) {
        // Existing base-range path -- delegates to getGlyphIndex so ASCII
        // results are provably identical to the pre-change implementation.
        return {getGlyphIndex(text[pos], activeFont), 1, false};
    }

    if (lead == 0xC2 || lead == 0xC3) {
        if (pos + 1 < text.size()) {
            const uint8_t cont = static_cast<uint8_t>(text[pos + 1]);
            if (cont >= 0x80 && cont <= 0xBF) {
                const uint8_t cp = static_cast<uint8_t>(((lead & 0x1Fu) << 6) | (cont & 0x3Fu));
                const bool extended = activeFont && activeFont->extGlyphs != nullptr &&
                    cp >= activeFont->extFirstChar && cp <= activeFont->extLastChar;
                const uint16_t index = extended
                    ? static_cast<uint16_t>(cp - activeFont->extFirstChar)
                    : getGlyphIndex(static_cast<char>(cp), activeFont);
                return {index, 2, extended};
            }
        }
        // Missing/invalid continuation: the lead byte alone is undrawable;
        // resume decoding at the next byte without consuming one that isn't there.
        return {kNoGlyph, 1, false};
    }

    // Any other multi-byte lead (3- or 4-byte UTF-8, or an out-of-range
    // 2-byte lead) consumes its whole declared sequence length, clamped to
    // the string, and always yields exactly one blank cell.
    uint8_t seqLen = 1;
    if ((lead & 0xE0u) == 0xC0u) {
        seqLen = 2;
    } else if ((lead & 0xF0u) == 0xE0u) {
        seqLen = 3;
    } else if ((lead & 0xF8u) == 0xF0u) {
        seqLen = 4;
    }

    if (seqLen == 1) {
        // Stray continuation byte (0x80-0xBF) or an invalid lead.
        return {kNoGlyph, 1, false};
    }

    const size_t remaining = text.size() - pos;
    const uint8_t consumed = static_cast<uint8_t>(seqLen < remaining ? seqLen : remaining);
    return {kNoGlyph, consumed, false};
}

} // namespace pixelroot32::graphics
