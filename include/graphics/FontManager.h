/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "Font.h"
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
    static int16_t textWidth(const Font* font, std::string_view text, uint8_t size = 1);

    /**
     * @brief Gets the glyph index for a character code.
     * @param c The character code.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     * @return Glyph index (0-based) if character is in font range, or 255 if not found.
     */
    static uint8_t getGlyphIndex(char c, const Font* font = nullptr);

    /**
     * @brief Checks if a character is supported by a font.
     * @param c The character code.
     * @param font Pointer to the font to check. If nullptr, uses the default font.
     * @return true if the character is in the font's range, false otherwise.
     */
    static bool isCharSupported(char c, const Font* font = nullptr);

private:
    static const Font* defaultFont;
};

} // namespace pixelroot32::graphics
