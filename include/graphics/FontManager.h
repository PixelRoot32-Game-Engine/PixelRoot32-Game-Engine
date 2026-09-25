/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "Font.h"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pixelroot32::graphics {

/**
 * @class FontManager
 * @brief Static utility class for managing bitmap fonts.
 *
 * FontManager provides functions to:
 * - Set and retrieve the default font
 * - Calculate text width for layout purposes
 * - Convert character codes to glyph indices
 *
 * The default font is used when no font is explicitly specified
 * in rendering calls.
 */
class FontManager {
public:
    /**
     * @brief Sets the default font used for text rendering.
     * @param font Pointer to a Font structure. Must remain valid for the lifetime of its use.
     *             Pass nullptr to clear the default font (not recommended).
     */
    static void setDefaultFont(const Font* font);

    /**
     * @brief Gets the current default font.
     * @return Pointer to the default font, or nullptr if no font is set.
     */
    static const Font* getDefaultFont();

    /**
     * @brief Calculates the width in pixels of a text string when rendered.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     * @param text The text string to measure.
     * @param size Text size multiplier (1 = normal, 2 = double size, etc.).
     * @return Width in pixels, or 0 if font is invalid or text is empty.
     */
    static int16_t textWidth(const Font* font, const char* text, uint8_t size = 1);

    /**
     * @brief Calculates the width in pixels of a text string when rendered.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     * @param text The text string to measure.
     * @param size Text size multiplier (1 = normal, 2 = double size, etc.).
     * @return Width in pixels, or 0 if font is invalid or text is empty.
     */
    static int16_t textWidth(const Font* font, std::string_view text, uint8_t size = 1);

    /**
     * @brief Sentinel returned by getGlyphIndex() when no glyph is found.
     *
     * Widened to uint16_t (rather than reusing the maximum uint8_t value) so a
     * legitimately wide font whose glyph table produces index 255 is never
     * confused with "not found" -- see getGlyphIndex().
     */
    static constexpr uint16_t kNoGlyph = 0xFFFF;

    /**
     * @brief Gets the glyph index for a character code.
     * @param c The character code.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     * @return Glyph index (0-based) if character is in font range, or `kNoGlyph` if not found.
     */
    static uint16_t getGlyphIndex(char c, const Font* font = nullptr);

    /**
     * @brief Checks if a character is supported by a font.
     * @param c The character code.
     * @param font Pointer to the font to check. If nullptr, uses the default font.
     * @return true if the character is in the font's range, false otherwise.
     * @note Evaluates a single byte only. Does not decode multi-byte UTF-8
     *       sequences -- a lead byte passed alone is evaluated as that byte
     *       value, not as the start of a sequence. Use isCodepointSupported()
     *       or nextGlyph() for multi-byte-aware queries.
     */
    static bool isCharSupported(char c, const Font* font = nullptr);

    /**
     * @brief Checks if a decoded codepoint is supported by a font.
     * @param codepoint The decoded codepoint (base ASCII or Latin-1 supplement).
     * @param font Pointer to the font to check. If nullptr, uses the default font.
     * @return true if the codepoint is in the font's base or supplement range.
     */
    static bool isCodepointSupported(uint16_t codepoint, const Font* font = nullptr);

    /**
     * @brief One decoded glyph step: the result of consuming one UTF-8 sequence.
     */
    struct GlyphStep {
        uint16_t index;    ///< Index into glyphs[] or extGlyphs[]; kNoGlyph if undrawable
        uint8_t  bytes;    ///< Bytes consumed from `text` at `pos` (1-4)
        bool     extended; ///< true when `index` addresses extGlyphs (apply extYOffset)
    };

    /**
     * @brief Decodes one glyph position from `text` starting at `pos`.
     *
     * Shared by Renderer::drawText and FontManager::textWidth so measurement
     * and drawing can never drift apart. Folds a 0xC2/0xC3 lead byte plus a
     * valid continuation into codepoint 0xA0-0xFF; any other multi-byte lead
     * consumes its whole sequence and yields exactly one blank cell. Bytes
     * < 0x80 take the existing single-byte path, unconditionally.
     *
     * @param text The full string being decoded.
     * @param pos Byte offset to start decoding at.
     * @param font Font whose base/supplement ranges resolve the codepoint.
     * @return The decoded step; `bytes` is always >= 1.
     */
    static GlyphStep nextGlyph(std::string_view text, size_t pos, const Font* font);

private:
    static const Font* defaultFont;
};

} // namespace pixelroot32::graphics
