/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once

#include <cstdint>

namespace pixelroot32::graphics {

// Forward declaration
struct Sprite;

/**
 * @struct Font
 * @brief Descriptor for a bitmap font using 1bpp sprites.
 *
 * A Font contains an array of Sprite structures, one for each character
 * in the font's character set. Each glyph is rendered as a 1bpp sprite,
 * allowing consistent rendering across platforms.
 *
 * The font uses fixed-width glyphs for simplicity and performance.
 * All glyphs share the same width and height, with spacing between
 * characters controlled by the `spacing` field.
 *
 * @note Font data should be stored in flash memory (const/constexpr)
 *       to minimize RAM usage on embedded systems.
 * @note Include "Renderer.h" when using Font in implementation files
 *       to get the full Sprite definition.
 */
struct Font {
    const Sprite* glyphs;      ///< Array of sprites, one per character (indexed by character code - firstChar)
    uint8_t firstChar;         ///< First character code in the font (e.g., 32 for space ' ')
    uint8_t lastChar;          ///< Last character code in the font (e.g., 126 for '~')
    uint8_t glyphWidth;        ///< Fixed width of each glyph in pixels
    uint8_t glyphHeight;       ///< Fixed height of each glyph in pixels
    uint8_t spacing;           ///< Horizontal spacing between characters in pixels
    uint8_t lineHeight;        ///< Total line height including vertical spacing (glyphHeight + vertical spacing)
};

} // namespace pixelroot32::graphics
