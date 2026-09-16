/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/TextLayout.h"
#include "graphics/FontManager.h"

#include <algorithm>
#include <cstdint>

namespace pixelroot32::graphics {

namespace {

/// One line's scan result: where its visible content ends, where the next
/// line starts (skipping a consumed break character), and the pixel width
/// already measured for [lineStart, sliceEnd).
struct LineScan {
    size_t  sliceEnd;
    size_t  nextStart;
    int16_t widthPx;
};

/// Scans one wrapped line starting at `lineStart`. Peeks the raw byte for
/// '\n' and ' ' before decoding (design D2) -- both are always single-byte
/// ASCII (bytes < 0x80 can never occur inside a multi-byte UTF-8 sequence),
/// so this is UTF-8-safe with no decode. Every glyph that is kept advances
/// through FontManager::nextGlyph, the single decoder TextLayout never
/// re-derives.
LineScan scanLine(std::string_view text, size_t lineStart, const Font* font,
                   int16_t advance, int16_t spacingPx, int16_t maxWidthPx) {
    const size_t len = text.size();
    size_t pos = lineStart;

    size_t  lastSpace          = std::string_view::npos;
    size_t  lastSpaceLineEnd   = lineStart;
    size_t  lastSpaceNextStart = lineStart;
    int16_t widthAtLastSpace   = 0;

    uint32_t glyphCount = 0;
    int16_t  rawWidth   = 0;

    while (pos < len) {
        const uint8_t c = static_cast<uint8_t>(text[pos]);

        if (c == '\n') {
            return {pos, pos + 1,
                    glyphCount > 0 ? static_cast<int16_t>(rawWidth - spacingPx) : int16_t{0}};
        }

        const int16_t newRawWidth     = static_cast<int16_t>(rawWidth + advance);
        const int16_t newDisplayWidth = static_cast<int16_t>(newRawWidth - spacingPx);

        // The first glyph of any line is guaranteed to fit -- wrapPass()
        // checks glyphWidthPx <= maxWidthPx up front for the whole call --
        // so glyphCount > 0 here means real progress was already made.
        if (glyphCount > 0 && newDisplayWidth > maxWidthPx) {
            if (c == ' ') {
                // The overflowing character is itself the boundary: trim it,
                // never carry it as a leading space on the next line.
                return {pos, pos + 1, static_cast<int16_t>(rawWidth - spacingPx)};
            }
            if (lastSpace != std::string_view::npos) {
                return {lastSpaceLineEnd, lastSpaceNextStart, widthAtLastSpace};
            }
            // Hard break: word longer than the line.
            return {pos, pos, static_cast<int16_t>(rawWidth - spacingPx)};
        }

        if (c == ' ' && glyphCount > 0) {
            // Do not treat a leading space as a break candidate -- an empty
            // line before an unbreakable word is worse than hard-breaking
            // the word itself.
            lastSpace          = pos;
            lastSpaceLineEnd   = pos;
            lastSpaceNextStart = pos + 1;
            widthAtLastSpace   = static_cast<int16_t>(rawWidth - spacingPx);
        }

        const auto step = FontManager::nextGlyph(text, pos, font);
        rawWidth = newRawWidth;
        ++glyphCount;
        pos += step.bytes;
    }

    return {pos, pos, glyphCount > 0 ? static_cast<int16_t>(rawWidth - spacingPx) : int16_t{0}};
}

/// Shared wrap/count pass -- one linear scan, never run twice for the same
/// call. `outLines == nullptr` counts every wrapped line with no skip and no
/// cap (countWrappedLines()). `outLines != nullptr` skips `skipLines` lines
/// before writing and stops at `maxOutLines` (wrap()). `font` must already
/// be resolved (never nullptr going in; callers resolve the default once).
uint16_t wrapPass(std::string_view text, const Font* font, uint8_t size,
                   int16_t maxWidthPx, uint16_t skipLines,
                   TextLayout::WrappedLine* outLines, uint16_t maxOutLines) {
    if (!font || !font->glyphs) {
        return 0;
    }

    const int16_t glyphWidthPx = static_cast<int16_t>(font->glyphWidth * size);
    if (maxWidthPx < glyphWidthPx) {
        return 0;
    }

    // Whitespace-only input (no newline) wraps to zero lines, matching the
    // empty-string case -- the "Empty and whitespace-only input" scenario
    // accepts either "0 lines" or "a single empty line"; this picks the
    // 0-lines branch for both, consistently.
    if (text.empty() || text.find_first_not_of(' ') == std::string_view::npos) {
        return 0;
    }

    const int16_t advance   = static_cast<int16_t>((font->glyphWidth + font->spacing) * size);
    const int16_t spacingPx = static_cast<int16_t>(font->spacing * size);

    uint16_t     lineIndex = 0;
    uint16_t     written   = 0;
    size_t       pos       = 0;
    const size_t len       = text.size();

    while (pos < len) {
        // A remainder made only of spaces draws nothing, so it must not open
        // a wrapped line. scanLine() consumes exactly ONE space at a break
        // (nextStart == pos + 1), so N trailing spaces leave N-1 to start a
        // fresh scan; without this guard that scan emits an invisible line
        // which still costs a line slot, vertical space and part of a
        // presenter's paging budget -- in a DialogBox capped at
        // DialogMaxWrappedLines rows it can force a page holding nothing but
        // the next-page cue. This is the whole-input whitespace guard above,
        // applied to every post-break remainder. A newline still in the
        // remainder is real content (it opens a deliberate blank line) and
        // find_first_not_of(' ') sees it, so that case is untouched.
        if (text.find_first_not_of(' ', pos) == std::string_view::npos) {
            break;
        }

        const LineScan scan = scanLine(text, pos, font, advance, spacingPx, maxWidthPx);

        if (outLines == nullptr) {
            ++lineIndex;
        } else {
            if (lineIndex >= skipLines && written < maxOutLines) {
                outLines[written].slice   = text.substr(pos, scan.sliceEnd - pos);
                outLines[written].widthPx = scan.widthPx;
                ++written;
            }
            ++lineIndex;
            if (written >= maxOutLines) {
                break;
            }
        }

        pos = scan.nextStart;
    }

    return outLines == nullptr ? lineIndex : written;
}

} // namespace

uint8_t TextLayout::wrap(std::string_view text, const Font* font, uint8_t size,
                          int16_t maxWidthPx, uint16_t skipLines,
                          WrappedLine* outLines, uint8_t maxOutLines) {
    // A null outLines must never reach wrapPass(): there, outLines == nullptr
    // is an internal sentinel meaning "count-only mode" (used exclusively by
    // countWrappedLines()), which ignores skipLines/maxOutLines and returns
    // the unbounded total line count instead of 0.
    if (outLines == nullptr) {
        return 0;
    }
    const Font* activeFont = font ? font : FontManager::getDefaultFont();
    return static_cast<uint8_t>(
        wrapPass(text, activeFont, size, maxWidthPx, skipLines, outLines, maxOutLines));
}

uint16_t TextLayout::countWrappedLines(std::string_view text, const Font* font, uint8_t size,
                                        int16_t maxWidthPx) {
    const Font* activeFont = font ? font : FontManager::getDefaultFont();
    return wrapPass(text, activeFont, size, maxWidthPx, 0, nullptr, 0);
}

int16_t TextLayout::measureWidthPx(std::string_view slice, const Font* font, uint8_t size) {
    // Thin forward to FontManager::textWidth -- deliberately not a
    // reimplementation (design D1), so TextLayout can never disagree with
    // what Renderer::drawText actually draws.
    return FontManager::textWidth(font, slice, size);
}

uint16_t TextLayout::countGlyphs(std::string_view slice, const Font* font) {
    const Font* activeFont = font ? font : FontManager::getDefaultFont();
    if (!activeFont || !activeFont->glyphs) {
        return 0;
    }
    uint16_t count = 0;
    for (size_t i = 0; i < slice.size();) {
        const auto step = FontManager::nextGlyph(slice, i, activeFont);
        i += step.bytes;
        ++count;
    }
    return count;
}

size_t TextLayout::byteOffsetForGlyphs(std::string_view slice, const Font* font,
                                        size_t fromByteOffset, uint16_t glyphCount) {
    const Font* activeFont = font ? font : FontManager::getDefaultFont();
    size_t pos = std::min(fromByteOffset, slice.size());
    if (!activeFont || !activeFont->glyphs) {
        return pos;
    }
    for (uint16_t i = 0; i < glyphCount && pos < slice.size(); ++i) {
        const auto step = FontManager::nextGlyph(slice, pos, activeFont);
        pos += step.bytes;
    }
    return std::min(pos, slice.size());
}

} // namespace pixelroot32::graphics
