/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "graphics/Font.h"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pixelroot32::graphics {

/// Wrap/measure over FontManager. Stateless, allocation-free, no Renderer.
///
/// UNIT CONTRACT -- measure and index in GLYPHS, slice in BYTES.
/// Every std::string_view is a BYTE range. Every *Px value is pixels. Every
/// *glyphCount is decoded cells, one per FontManager::nextGlyph step, never
/// one per byte. Multi-byte UTF-8 therefore has byteLength > glyphCount.
class TextLayout {
public:
    /// One wrapped line: a non-owning BYTE slice of the caller's text plus the
    /// width the wrap pass already measured. Carrying widthPx here is
    /// deliberate: it removes any reason for a caller to re-measure, which is
    /// how draw geometry and hit-test geometry drift apart.
    struct WrappedLine {
        std::string_view slice;    ///< BYTES. Sub-view of the input; never owns.
        int16_t          widthPx;  ///< Pixel width of `slice` at the wrap's `size`.
    };

    /// Wraps `text` into `outLines`, skipping the first `skipLines` wrapped
    /// lines (paging: pass page * linesPerPage).
    /// Honors '\n' as a hard break. Breaks a word longer than maxWidthPx at the
    /// last GLYPH boundary that fits -- never mid-glyph.
    /// @return linesWritten: rows written to `outLines` (<= maxOutLines).
    ///         0 when the font is unusable, text is empty, maxWidthPx cannot
    ///         hold one glyph, or skipLines is past the end.
    [[nodiscard]] static uint8_t wrap(std::string_view text,
                                      const Font*      font,
                                      uint8_t          size,
                                      int16_t          maxWidthPx,
                                      uint16_t         skipLines,
                                      WrappedLine*     outLines,
                                      uint8_t          maxOutLines);

    /// Total wrapped lines `text` needs. Same algorithm as wrap(), no output.
    /// Use for page counts without wrapping twice.
    [[nodiscard]] static uint16_t countWrappedLines(std::string_view text,
                                                    const Font*      font,
                                                    uint8_t          size,
                                                    int16_t          maxWidthPx);

    /// Pixel width of a BYTE slice. Thin forward to FontManager::textWidth --
    /// deliberately not a reimplementation, so it cannot disagree with what
    /// Renderer::drawText actually draws. The cross-check test pins that this
    /// stays a forward.
    [[nodiscard]] static int16_t measureWidthPx(std::string_view slice,
                                                const Font*      font,
                                                uint8_t          size);

    /// Decoded cells in a BYTE slice, one per nextGlyph step. Never bytes.
    [[nodiscard]] static uint16_t countGlyphs(std::string_view slice,
                                              const Font*      font);

    /// Byte offset of the glyph boundary `glyphCount` glyphs after
    /// `fromByteOffset`, clamped to slice.size(). The BYTES<->GLYPHS bridge:
    /// the only sanctioned way to turn a glyph count into a slice length.
    [[nodiscard]] static size_t byteOffsetForGlyphs(std::string_view slice,
                                                    const Font*      font,
                                                    size_t           fromByteOffset,
                                                    uint16_t         glyphCount);
};

} // namespace pixelroot32::graphics
