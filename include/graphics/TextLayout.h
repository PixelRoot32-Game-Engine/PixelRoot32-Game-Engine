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

/**
 * @class TextLayout
 * @brief Wrap/measure over FontManager. Stateless, allocation-free, no Renderer.
 *
 * UNIT CONTRACT -- measure and index in GLYPHS, slice in BYTES.
 * Every std::string_view is a BYTE range. Every *Px value is pixels. Every
 * *glyphCount is decoded cells, one per FontManager::nextGlyph step, never
 * one per byte. Multi-byte UTF-8 therefore has byteLength > glyphCount.
 */
class TextLayout {
public:
    /**
     * @brief One wrapped line: a non-owning BYTE slice plus its already-measured width.
     *
     * Deliberately left without a Doxygen struct tag.
     * scripts/generate_api_docs.py
     * resolves a documented method against class_spans[-1], the most
     * recently tagged type rather than the enclosing one, so tagging a
     * nested type orphans every method documented after it -- all five
     * statics below then render as bare signatures with no description,
     * parameters or return value. FontManager::GlyphStep is untagged for
     * the same reason. Tag this struct only together with a generator
     * fix, and regenerate docs/api/generated/ to confirm.
     *
     * Carrying widthPx here is deliberate: it removes any reason for a
     * caller to re-measure, which is how draw geometry and hit-test
     * geometry drift apart.
     */
    struct WrappedLine {
        std::string_view slice;    ///< BYTES. Sub-view of the input; never owns.
        int16_t          widthPx;  ///< Pixel width of `slice` at the wrap's `size`.
    };

    /**
     * @brief Wraps `text` into `outLines`, skipping the first `skipLines` wrapped lines.
     *
     * Honors '\n' as a hard break. Breaks a word longer than maxWidthPx at
     * the last GLYPH boundary that fits -- never mid-glyph. `skipLines` is
     * computed within this same single wrap pass (paging: pass
     * page * linesPerPage), so a presenter can render page N with no
     * second wrap and no heap.
     *
     * @param text The full BYTE range to wrap; never mutated.
     * @param font Font to measure with; nullptr resolves FontManager's default.
     * @param size Glyph scale factor applied to `font`'s cell metrics.
     * @param maxWidthPx Maximum pixel width a wrapped line may occupy.
     * @param skipLines Number of wrapped lines to skip before writing to `outLines`.
     * @param outLines Caller-owned buffer to write wrapped lines into.
     * @param maxOutLines Capacity of `outLines`; never exceeded.
     * @return linesWritten: rows written to `outLines` (<= maxOutLines).
     *         0 when the font is unusable, text is empty or whitespace-only,
     *         maxWidthPx cannot hold one glyph, skipLines is past the end,
     *         outLines is null, or maxOutLines is 0.
     */
    [[nodiscard]] static uint8_t wrap(std::string_view text,
                                      const Font*      font,
                                      uint8_t          size,
                                      int16_t          maxWidthPx,
                                      uint16_t         skipLines,
                                      WrappedLine*     outLines,
                                      uint8_t          maxOutLines);

    /**
     * @brief Total wrapped lines `text` needs at `maxWidthPx`, without writing any output.
     *
     * Same algorithm as wrap(), so callers can compute a page count
     * without wrapping twice.
     *
     * @param text The full BYTE range to measure.
     * @param font Font to measure with; nullptr resolves FontManager's default.
     * @param size Glyph scale factor applied to `font`'s cell metrics.
     * @param maxWidthPx Maximum pixel width a wrapped line may occupy.
     * @return Total number of lines `text` would wrap to.
     */
    [[nodiscard]] static uint16_t countWrappedLines(std::string_view text,
                                                    const Font*      font,
                                                    uint8_t          size,
                                                    int16_t          maxWidthPx);

    /**
     * @brief Pixel width of a BYTE slice.
     *
     * Thin forward to FontManager::textWidth -- deliberately not a
     * reimplementation, so it cannot disagree with what Renderer::drawText
     * actually draws. The cross-check test pins that this stays a forward.
     *
     * @param slice The BYTE range to measure.
     * @param font Font to measure with; nullptr resolves FontManager's default.
     * @param size Glyph scale factor applied to `font`'s cell metrics.
     * @return Pixel width of `slice` at `size`.
     */
    [[nodiscard]] static int16_t measureWidthPx(std::string_view slice,
                                                const Font*      font,
                                                uint8_t          size);

    /**
     * @brief Decoded cells in a BYTE slice, one per FontManager::nextGlyph step.
     *
     * Never counts bytes: a multi-byte UTF-8 slice has byteLength greater
     * than this return value, per the class's unit contract.
     *
     * @param slice The BYTE range to count.
     * @param font Font whose decode rules resolve each glyph; nullptr resolves FontManager's default.
     * @return Number of decoded glyphs in `slice`.
     */
    [[nodiscard]] static uint16_t countGlyphs(std::string_view slice,
                                              const Font*      font);

    /**
     * @brief Byte offset of the glyph boundary `glyphCount` glyphs after `fromByteOffset`.
     *
     * The BYTES<->GLYPHS bridge: the only sanctioned way to turn a glyph
     * count into a slice length, clamped to slice.size().
     *
     * @param slice The BYTE range to walk.
     * @param font Font whose decode rules resolve each glyph; nullptr resolves FontManager's default.
     * @param fromByteOffset Byte offset to start counting glyphs from.
     * @param glyphCount Number of glyphs to advance past `fromByteOffset`.
     * @return Byte offset of the resulting glyph boundary, clamped to slice.size().
     */
    [[nodiscard]] static size_t byteOffsetForGlyphs(std::string_view slice,
                                                    const Font*      font,
                                                    size_t           fromByteOffset,
                                                    uint16_t         glyphCount);
};

} // namespace pixelroot32::graphics
