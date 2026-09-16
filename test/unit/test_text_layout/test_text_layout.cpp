/**
 * @file test_text_layout.cpp
 * @brief Unit tests for graphics/TextLayout module
 * @version 1.0
 * @date 2026-09-10
 *
 * TextLayout is unflagged (no PIXELROOT32_ENABLE_DIALOG guard) and runs in
 * both native_test and native_test_gameplay. It has zero callers inside this
 * change -- these tests are its only exercise.
 */

#include <unity.h>
#include <cstdint>
#include <cstddef>
#include "../../test_config.h"
#include "graphics/TextLayout.h"
#include "graphics/FontManager.h"
#include "graphics/Renderer.h"
#include "graphics/Font5x7.h"

using namespace pixelroot32::graphics;

// =============================================================================
// Fixtures -- same shape as test_font_manager's testFont/extFont so the two
// suites' arithmetic is directly comparable.
// =============================================================================

static const uint16_t mockSpriteData[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static const Sprite mockGlyphs[] = {
    {mockSpriteData, 5, 7}
};

// 5x7 font, spacing 1, chars 32-126. advance = (5+1)*size; width(N) = N*advance - spacing*size.
static const Font testFont = {mockGlyphs, 32, 126, 5, 7, 1, 8};
static const Font emptyGlyphFont = {nullptr, 32, 126, 5, 7, 1, 8};

// Synthetic supplement block: one drawn glyph at codepoint 0xF1 ('n' + tilde),
// mirroring test_font_manager's extFont exactly.
static const uint16_t extSpriteData[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static const Sprite extGlyphsData[] = {{extSpriteData, 5, 8}};
static const Font extFont = {mockGlyphs, 32, 126, 5, 7, 1, 8, extGlyphsData, 0xF1, 0xF1, -1};

void setUp(void) {
    test_setup();
    FontManager::setDefaultFont(nullptr);
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// measureWidthPx -- cross-check against FontManager::textWidth
// =============================================================================

void test_text_layout_measure_cross_check_ascii_corpus(void) {
    // GIVEN a corpus of ASCII strings, WHEN measured by both measureWidthPx
    // and FontManager::textWidth, THEN results are identical for every entry.
    static const std::string_view corpus[] = {
        "", "A", "AB", "Hello", "A B", "HELLO WORLD", "AAAAA BBBBB CCCCC DDDDD"
    };
    for (const auto& s : corpus) {
        TEST_ASSERT_EQUAL_INT(FontManager::textWidth(&testFont, s, 1),
                               TextLayout::measureWidthPx(s, &testFont, 1));
    }
}

void test_text_layout_measure_cross_check_latin1_corpus(void) {
    // GIVEN a corpus including 2-byte Latin-1 sequences, WHEN measured by
    // both, THEN results are identical -- including the extended (drawn) and
    // the blank-cell (no ext block) paths.
    static const std::string_view corpus[] = {
        "\xC3\xB1",         // Ntilde, folds into extFont's supplement block
        "A\xC3\xB1M",       // ASCII + Latin-1 + ASCII
        "\xC3\xB1\xC3\xB1"  // two consecutive Latin-1 glyphs
    };
    for (const auto& s : corpus) {
        TEST_ASSERT_EQUAL_INT(FontManager::textWidth(&extFont, s, 1),
                               TextLayout::measureWidthPx(s, &extFont, 1));
    }
    // Same bytes, no supplement block -- blank-cell path, still must agree.
    TEST_ASSERT_EQUAL_INT(FontManager::textWidth(&testFont, "\xC3\xB1", 1),
                           TextLayout::measureWidthPx("\xC3\xB1", &testFont, 1));
}

void test_text_layout_measure_width_px_null_font_no_default_returns_zero(void) {
    TEST_ASSERT_EQUAL_INT(0, TextLayout::measureWidthPx("A", nullptr, 1));
}

// =============================================================================
// wrap -- word boundary wrapping
// =============================================================================

void test_text_layout_wrap_breaks_at_word_boundary(void) {
    // GIVEN text longer than one line's maxWidthPx, WHEN wrap is called with
    // maxOutLines sufficient to hold it, THEN it returns lines each within
    // maxWidthPx, breaking at word boundaries where possible.
    // "AAAAA BBBBB CCCCC DDDDD": 4 words of 5 chars, each word alone measures
    // exactly 29 px (5*6-1); maxWidthPx=29 admits exactly one word per line.
    TextLayout::WrappedLine lines[10];
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB CCCCC DDDDD", &testFont, 1, 29, 0, lines, 10);

    TEST_ASSERT_EQUAL_UINT8(4, written);
    TEST_ASSERT_EQUAL_STRING_LEN("AAAAA", lines[0].slice.data(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_size_t(5, lines[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, lines[0].widthPx);
    TEST_ASSERT_EQUAL_STRING_LEN("BBBBB", lines[1].slice.data(), lines[1].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, lines[1].widthPx);
    TEST_ASSERT_EQUAL_STRING_LEN("CCCCC", lines[2].slice.data(), lines[2].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, lines[2].widthPx);
    TEST_ASSERT_EQUAL_STRING_LEN("DDDDD", lines[3].slice.data(), lines[3].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, lines[3].widthPx);
}

void test_text_layout_wrap_word_longer_than_line_hard_breaks(void) {
    // GIVEN a single word wider than maxWidthPx, WHEN wrap is called, THEN
    // the word is hard-broken across lines rather than overflowing.
    // "ABCDEFGHIJ": 10 chars, no spaces; maxWidthPx=29 fits exactly 5.
    TextLayout::WrappedLine lines[10];
    const uint8_t written = TextLayout::wrap("ABCDEFGHIJ", &testFont, 1, 29, 0, lines, 10);

    TEST_ASSERT_EQUAL_UINT8(2, written);
    TEST_ASSERT_EQUAL_STRING_LEN("ABCDE", lines[0].slice.data(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_size_t(5, lines[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, lines[0].widthPx);
    TEST_ASSERT_EQUAL_STRING_LEN("FGHIJ", lines[1].slice.data(), lines[1].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, lines[1].widthPx);
}

void test_text_layout_wrap_exactly_one_pixel_too_wide_wraps(void) {
    // The complementary edge of the exact-width boundary the two tests above
    // already pin from the fitting side ("AAAAA" measures exactly 29 px and
    // stays on one line at maxWidthPx == 29). Here the SAME text is given one
    // pixel less room than it needs, so it must wrap: the comparison is
    // `width > maxWidthPx`, not `>=`, and both sides of that `>` are now
    // pinned.
    TEST_ASSERT_EQUAL_INT(29, TextLayout::measureWidthPx("AAAAA", &testFont, 1));

    // Fitting side: exactly maxWidthPx -> one line (guards the `>` against
    // silently becoming `>=`).
    TextLayout::WrappedLine exact[4];
    TEST_ASSERT_EQUAL_UINT8(1, TextLayout::wrap("AAAAA", &testFont, 1, 29, 0, exact, 4));
    TEST_ASSERT_EQUAL_STRING_LEN("AAAAA", exact[0].slice.data(), exact[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, exact[0].widthPx);

    // Overflowing side: one pixel too narrow -> hard break after 4 glyphs,
    // since the 5th would take the line to 29 px against a 28 px budget.
    TextLayout::WrappedLine tooWide[4];
    const uint8_t written = TextLayout::wrap("AAAAA", &testFont, 1, 28, 0, tooWide, 4);
    TEST_ASSERT_EQUAL_UINT8(2, written);
    TEST_ASSERT_EQUAL_STRING_LEN("AAAA", tooWide[0].slice.data(), tooWide[0].slice.size());
    TEST_ASSERT_EQUAL_size_t(4, tooWide[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(23, tooWide[0].widthPx); // 4*6-1
    TEST_ASSERT_EQUAL_STRING_LEN("A", tooWide[1].slice.data(), tooWide[1].slice.size());
    TEST_ASSERT_EQUAL_INT16(5, tooWide[1].widthPx);

    // countWrappedLines() must agree on both sides of the same boundary.
    TEST_ASSERT_EQUAL_UINT16(1, TextLayout::countWrappedLines("AAAAA", &testFont, 1, 29));
    TEST_ASSERT_EQUAL_UINT16(2, TextLayout::countWrappedLines("AAAAA", &testFont, 1, 28));
}

void test_text_layout_wrap_trailing_spaces_do_not_force_an_extra_line(void) {
    // GIVEN text whose only content past the last fitting word is trailing
    // spaces, WHEN wrap is called, THEN those spaces never open a further
    // wrapped line -- scanLine() consumes the overflowing space as the break
    // itself (returning nextStart past it) instead of carrying it over as a
    // leading space on a new line.

    // One trailing space, exactly at the width boundary: still one line.
    TextLayout::WrappedLine one[4];
    TEST_ASSERT_EQUAL_UINT8(1, TextLayout::wrap("AAAAA ", &testFont, 1, 29, 0, one, 4));
    TEST_ASSERT_EQUAL_STRING_LEN("AAAAA", one[0].slice.data(), one[0].slice.size());
    TEST_ASSERT_EQUAL_size_t(5, one[0].slice.size()); // the space is trimmed, not kept
    TEST_ASSERT_EQUAL_INT16(29, one[0].widthPx);

    // Two words that each fill a line, with the single space between them
    // consumed as the break: exactly two lines.
    TextLayout::WrappedLine two[8];
    TEST_ASSERT_EQUAL_UINT8(2, TextLayout::wrap("AAAAA BBBBB", &testFont, 1, 29, 0, two, 8));
    TEST_ASSERT_EQUAL_STRING_LEN("AAAAA", two[0].slice.data(), two[0].slice.size());
    TEST_ASSERT_EQUAL_STRING_LEN("BBBBB", two[1].slice.data(), two[1].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, two[1].widthPx);

    // A single trailing space adds nothing to the line count.
    TEST_ASSERT_EQUAL_UINT16(TextLayout::countWrappedLines("AAAAA BBBBB", &testFont, 1, 29),
                              TextLayout::countWrappedLines("AAAAA BBBBB ", &testFont, 1, 29));
}

void test_text_layout_wrap_multiple_trailing_spaces_do_not_open_an_extra_line(void) {
    // REGRESSION GUARD for the defect audit section 11.1 named. A break
    // consumes exactly ONE space (scanLine() returns nextStart == pos + 1),
    // so with N trailing spaces the remaining N-1 used to start a fresh scan
    // and be emitted as a real wrapped line -- invisible when drawn, yet it
    // still consumed a line slot, vertical space and a slice of the paging
    // budget. wrapPass() now stops when the remainder from `pos` holds
    // nothing but spaces.
    TextLayout::WrappedLine lines[8];
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB   ", &testFont, 1, 29, 0, lines, 8);

    TEST_ASSERT_EQUAL_UINT8(2, written);
    TEST_ASSERT_EQUAL_STRING_LEN("AAAAA", lines[0].slice.data(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_STRING_LEN("BBBBB", lines[1].slice.data(), lines[1].slice.size());

    // Reachable without any wrapping at all: one word plus two trailing
    // spaces, where the width boundary consumes the first of them.
    TEST_ASSERT_EQUAL_UINT16(1, TextLayout::countWrappedLines("AAAAA  ", &testFont, 1, 29));

    // countWrappedLines() must agree with wrap() -- a presenter measures
    // first and wraps second, so a disagreement mis-sizes the panel.
    TEST_ASSERT_EQUAL_UINT16(written,
                              TextLayout::countWrappedLines("AAAAA BBBBB   ", &testFont, 1, 29));

    // Trailing spaces of any count cost nothing, however many there are.
    TEST_ASSERT_EQUAL_UINT16(TextLayout::countWrappedLines("AAAAA BBBBB", &testFont, 1, 29),
                              TextLayout::countWrappedLines("AAAAA BBBBB      ", &testFont, 1, 29));

    // A newline still in the remainder is real content, not trailing
    // whitespace, so the deliberate blank line it opens must survive this
    // guard: test_text_layout_wrap_consecutive_newlines_keep_the_blank_line
    // pins that case.
}

void test_text_layout_wrap_consecutive_newlines_keep_the_blank_line(void) {
    // CHARACTERIZATION of today's observed behavior: "a\n\nb" produces THREE
    // lines -- "a", an empty line, then "b". scanLine() returns immediately on
    // a '\n' with sliceEnd == the newline's own position, so the second
    // newline yields a zero-length slice of width 0 rather than being
    // collapsed into the first break. A presenter that lays lines out by
    // index therefore gets the vertical gap the author wrote.
    TextLayout::WrappedLine lines[8];
    const uint8_t written = TextLayout::wrap("a\n\nb", &testFont, 1, 100, 0, lines, 8);

    TEST_ASSERT_EQUAL_UINT8(3, written);
    TEST_ASSERT_EQUAL_STRING_LEN("a", lines[0].slice.data(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_size_t(1, lines[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(5, lines[0].widthPx);

    // The blank line between the two newlines: present, empty, zero width.
    TEST_ASSERT_EQUAL_size_t(0, lines[1].slice.size());
    TEST_ASSERT_EQUAL_INT16(0, lines[1].widthPx);

    TEST_ASSERT_EQUAL_STRING_LEN("b", lines[2].slice.data(), lines[2].slice.size());
    TEST_ASSERT_EQUAL_size_t(1, lines[2].slice.size());
    TEST_ASSERT_EQUAL_INT16(5, lines[2].widthPx);

    // countWrappedLines() counts the blank line too -- a paging presenter
    // must not disagree with wrap() about how tall the text is.
    TEST_ASSERT_EQUAL_UINT16(3, TextLayout::countWrappedLines("a\n\nb", &testFont, 1, 100));
}

void test_text_layout_wrap_max_out_lines_overflow_no_oob(void) {
    // GIVEN text requiring more lines than maxOutLines, WHEN wrap is called,
    // THEN it writes exactly maxOutLines entries and returns maxOutLines,
    // without out-of-bounds writes to outLines. Array sized exactly to
    // maxOutLines so any overrun would corrupt an adjacent stack variable.
    TextLayout::WrappedLine lines[2];
    const uint32_t sentinelBefore = 0xABCD1234u;
    (void)sentinelBefore;
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB CCCCC DDDDD", &testFont, 1, 29, 0, lines, 2);

    TEST_ASSERT_EQUAL_UINT8(2, written);
    TEST_ASSERT_EQUAL_STRING_LEN("AAAAA", lines[0].slice.data(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_STRING_LEN("BBBBB", lines[1].slice.data(), lines[1].slice.size());
}

void test_text_layout_wrap_max_out_lines_zero_returns_zero(void) {
    // GIVEN maxOutLines == 0 with a valid non-null buffer, WHEN wrap is
    // called, THEN it returns 0 and writes nothing. wrapPass()'s
    // write-then-break guard (`written < maxOutLines` before writing,
    // `written >= maxOutLines` break after) is written to handle this
    // boundary safely, but no prior test exercised it.
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB", &testFont, 1, 29, 0, lines, 0);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

void test_text_layout_wrap_empty_string_returns_zero(void) {
    // GIVEN an empty string, WHEN wrap is called, THEN it returns 0 lines
    // without crashing.
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap("", &testFont, 1, 100, 0, lines, 4);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

void test_text_layout_wrap_whitespace_only_returns_zero(void) {
    // GIVEN a string of only spaces, WHEN wrap is called, THEN it returns 0
    // lines without crashing.
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap("   ", &testFont, 1, 100, 0, lines, 4);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

void test_text_layout_wrap_newline_is_hard_break(void) {
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap("AB\nCD", &testFont, 1, 100, 0, lines, 4);
    TEST_ASSERT_EQUAL_UINT8(2, written);
    TEST_ASSERT_EQUAL_STRING_LEN("AB", lines[0].slice.data(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(11, lines[0].widthPx);
    TEST_ASSERT_EQUAL_STRING_LEN("CD", lines[1].slice.data(), lines[1].slice.size());
    TEST_ASSERT_EQUAL_INT16(11, lines[1].widthPx);
}

void test_text_layout_wrap_max_width_too_small_for_one_glyph_returns_zero(void) {
    // GIVEN maxWidthPx smaller than one glyph's own width, WHEN wrap is
    // called, THEN it returns 0 without hanging (guards the infinite-loop
    // case the design document's sketch would hit).
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap("A", &testFont, 1, 4, 0, lines, 4); // glyphWidth=5
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

void test_text_layout_wrap_null_font_no_default_returns_zero(void) {
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap("Hello", nullptr, 1, 100, 0, lines, 4);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

void test_text_layout_wrap_null_out_lines_returns_zero(void) {
    // GIVEN a null outLines buffer, WHEN wrap is called, THEN it returns 0
    // rather than forwarding into wrapPass()'s internal count-only sentinel
    // (outLines == nullptr there means "ignore skipLines/maxOutLines, count
    // everything" -- the mode countWrappedLines() relies on). A caller that
    // passes a null buffer to wrap() must get 0, never an unbounded count.
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB", &testFont, 1, 100, 0, nullptr, 4);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

void test_text_layout_wrap_uses_default_font(void) {
    FontManager::setDefaultFont(&testFont);
    TextLayout::WrappedLine lines[10];
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB", nullptr, 1, 29, 0, lines, 10);
    TEST_ASSERT_EQUAL_UINT8(2, written);
    TEST_ASSERT_EQUAL_STRING_LEN("AAAAA", lines[0].slice.data(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(29, lines[0].widthPx);
}

// =============================================================================
// wrap -- skipLines paging (no second wrap, no heap)
// =============================================================================

void test_text_layout_wrap_skip_lines_mid_skip(void) {
    // GIVEN text that wraps to more lines than fit on one page, and a
    // skipLines value equal to the first page's line count, WHEN wrap is
    // called with that skipLines, THEN outLines[0] is the wrapped line that
    // would have been at index skipLines in an unskipped wrap.
    static constexpr const char* kText = "AAAAA BBBBB CCCCC DDDDD";
    TextLayout::WrappedLine unskipped[10];
    const uint8_t unskippedCount = TextLayout::wrap(kText, &testFont, 1, 29, 0, unskipped, 10);
    TEST_ASSERT_EQUAL_UINT8(4, unskippedCount);

    TextLayout::WrappedLine skipped[10];
    const uint8_t skippedCount = TextLayout::wrap(kText, &testFont, 1, 29, 2, skipped, 10);
    TEST_ASSERT_EQUAL_UINT8(2, skippedCount);
    TEST_ASSERT_EQUAL_STRING_LEN(unskipped[2].slice.data(), skipped[0].slice.data(), unskipped[2].slice.size());
    TEST_ASSERT_EQUAL_size_t(unskipped[2].slice.size(), skipped[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(unskipped[2].widthPx, skipped[0].widthPx);
    TEST_ASSERT_EQUAL_STRING_LEN("CCCCC", skipped[0].slice.data(), skipped[0].slice.size());
    TEST_ASSERT_EQUAL_STRING_LEN("DDDDD", skipped[1].slice.data(), skipped[1].slice.size());
}

void test_text_layout_wrap_skip_lines_at_total_count_returns_zero(void) {
    // GIVEN a skipLines value equal to the text's total wrapped line count,
    // WHEN wrap is called, THEN it writes 0 lines, without OOB reads/writes.
    TextLayout::WrappedLine lines[10];
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB CCCCC DDDDD", &testFont, 1, 29, 4, lines, 10);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

void test_text_layout_wrap_skip_lines_past_end_returns_zero(void) {
    // GIVEN a skipLines value greater than the text's total wrapped line
    // count, WHEN wrap is called, THEN it writes 0 lines, without OOB.
    TextLayout::WrappedLine lines[10];
    const uint8_t written = TextLayout::wrap("AAAAA BBBBB CCCCC DDDDD", &testFont, 1, 29, 999, lines, 10);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

// =============================================================================
// countWrappedLines
// =============================================================================

void test_text_layout_count_wrapped_lines_matches_wrap_count(void) {
    // GIVEN the same text and maxWidthPx, WHEN countWrappedLines is called
    // and separately wrap is called with maxOutLines large enough to hold
    // all lines, THEN countWrappedLines's result equals the number of lines
    // wrap produced.
    static const std::string_view samples[] = {
        "AAAAA BBBBB CCCCC DDDDD",
        "ABCDEFGHIJ",
        "AB\nCD",
        "Hello"
    };
    for (const auto& text : samples) {
        TextLayout::WrappedLine lines[16];
        const uint8_t written = TextLayout::wrap(text, &testFont, 1, 29, 0, lines, 16);
        const uint16_t counted = TextLayout::countWrappedLines(text, &testFont, 1, 29);
        TEST_ASSERT_EQUAL_UINT16(written, counted);
    }
}

void test_text_layout_count_wrapped_lines_empty_and_whitespace(void) {
    TEST_ASSERT_EQUAL_UINT16(0, TextLayout::countWrappedLines("", &testFont, 1, 100));
    TEST_ASSERT_EQUAL_UINT16(0, TextLayout::countWrappedLines("   ", &testFont, 1, 100));
}

// =============================================================================
// Malformed UTF-8 characterization (proposal D4 / design D2 termination proof)
//
// CHARACTERIZATION, NOT A FIX: FontManager::nextGlyph validates the
// continuation byte only for the 2-byte 0xC2/0xC3 lead. For a 3-/4-byte lead
// it consumes its whole declared sequence length (clamped to the remaining
// bytes) unconditionally -- so a malformed 3-byte lead silently swallows the
// plain-ASCII bytes that follow it as part of one blank glyph cell. This is
// documented, reviewed, and deliberately NOT fixed here (FontManager.cpp:152-153
// clamps `consumed` to `remaining` and every nextGlyph return path yields
// `bytes >= 1`, so TextLayout's byte-slicing stays memory-safe and
// terminating regardless). These tests pin TODAY's exact output so a future
// nextGlyph fix cannot silently change TextLayout's contract.
// =============================================================================

void test_text_layout_malformed_utf8_lead_swallows_ascii_characterization(void) {
    // "\xE2" is a 3-byte UTF-8 lead (0xE2 & 0xF0 == 0xE0). Followed by 'A'
    // 'B' -- plain ASCII, NOT valid continuation bytes (0x80-0xBF) -- then
    // 'C'. nextGlyph does not validate this, so it consumes exactly 3 bytes
    // ("\xE2AB") as one blank glyph (kNoGlyph), then decodes 'C' normally.
    // Today's exact output: 2 glyphs total, width = 2*advance - spacing.
    static const std::string_view text = "\xE2""AB""C";

    const auto step = FontManager::nextGlyph(text, 0, &testFont);
    TEST_ASSERT_EQUAL_UINT16(FontManager::kNoGlyph, step.index);
    TEST_ASSERT_EQUAL_UINT8(3, step.bytes);
    TEST_ASSERT_FALSE(step.extended);

    TEST_ASSERT_EQUAL_UINT16(2, TextLayout::countGlyphs(text, &testFont));
    TEST_ASSERT_EQUAL_INT16(11, TextLayout::measureWidthPx(text, &testFont, 1)); // 2*6-1
    TEST_ASSERT_EQUAL_INT(FontManager::textWidth(&testFont, text, 1),
                           TextLayout::measureWidthPx(text, &testFont, 1));
}

void test_text_layout_malformed_utf8_wrap_stays_in_bounds_characterization(void) {
    // GIVEN a string containing a malformed 3-byte UTF-8 lead sequence, WHEN
    // wrap processes it, THEN it terminates within text's bounds and the
    // test asserts today's exact output -- documenting current behavior
    // rather than a desired one.
    static const std::string_view text = "\xE2""AB""C";
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap(text, &testFont, 1, 100, 0, lines, 4);

    TEST_ASSERT_EQUAL_UINT8(1, written);
    TEST_ASSERT_EQUAL_size_t(text.size(), lines[0].slice.size());
    TEST_ASSERT_EQUAL_INT16(11, lines[0].widthPx);
    // The slice's data pointer stays inside the original buffer -- no
    // allocation, no copy, byte range only.
    TEST_ASSERT_TRUE(lines[0].slice.data() >= text.data());
    TEST_ASSERT_TRUE(lines[0].slice.data() + lines[0].slice.size() <= text.data() + text.size());
}

void test_text_layout_malformed_utf8_four_byte_lead_characterization(void) {
    // A 4-byte lead (0xF0) clamped to the remaining 2 bytes when the string
    // ends early -- consumed = min(4, remaining), never past text.size().
    static const std::string_view text = "\xF0\x9F"; // truncated 4-byte sequence
    const auto step = FontManager::nextGlyph(text, 0, &testFont);
    TEST_ASSERT_EQUAL_UINT16(FontManager::kNoGlyph, step.index);
    TEST_ASSERT_EQUAL_UINT8(2, step.bytes); // clamped to remaining, not the declared 4

    TEST_ASSERT_EQUAL_UINT16(1, TextLayout::countGlyphs(text, &testFont));
}

// =============================================================================
// countGlyphs / byteOffsetForGlyphs -- the BYTES<->GLYPHS bridge
// =============================================================================

void test_text_layout_count_glyphs_ascii(void) {
    TEST_ASSERT_EQUAL_UINT16(5, TextLayout::countGlyphs("Hello", &testFont));
    TEST_ASSERT_EQUAL_UINT16(0, TextLayout::countGlyphs("", &testFont));
}

void test_text_layout_count_glyphs_latin1_two_byte(void) {
    // "\xC3\xB1" (Ntilde, 2 bytes) folds into exactly one glyph cell.
    TEST_ASSERT_EQUAL_UINT16(1, TextLayout::countGlyphs("\xC3\xB1", &extFont));
    TEST_ASSERT_EQUAL_UINT16(3, TextLayout::countGlyphs("A\xC3\xB1M", &extFont));
}

void test_text_layout_byte_offset_for_glyphs_ascii(void) {
    TEST_ASSERT_EQUAL_size_t(3, TextLayout::byteOffsetForGlyphs("Hello", &testFont, 0, 3));
    TEST_ASSERT_EQUAL_size_t(0, TextLayout::byteOffsetForGlyphs("Hello", &testFont, 0, 0));
}

void test_text_layout_byte_offset_for_glyphs_latin1_bridge(void) {
    // First glyph (Ntilde) consumes 2 bytes, second ('H') consumes 1 -- the
    // BYTES<->GLYPHS bridge must count decoded cells, never raw bytes.
    TEST_ASSERT_EQUAL_size_t(3, TextLayout::byteOffsetForGlyphs("\xC3\xB1Hello", &extFont, 0, 2));
}

void test_text_layout_byte_offset_for_glyphs_clamped_past_end(void) {
    // Requesting more glyphs than the slice contains clamps to slice.size(),
    // never reading or returning past the buffer.
    TEST_ASSERT_EQUAL_size_t(2, TextLayout::byteOffsetForGlyphs("Hi", &testFont, 0, 100));
}

void test_text_layout_byte_offset_for_glyphs_null_font_clamps_from_offset(void) {
    TEST_ASSERT_EQUAL_size_t(2, TextLayout::byteOffsetForGlyphs("Hi", nullptr, 2, 5));
}

// =============================================================================
// Empty-glyph-table font -- same "font unusable" guard as FontManager
// =============================================================================

void test_text_layout_measure_width_px_empty_glyph_table_returns_zero(void) {
    TEST_ASSERT_EQUAL_INT(0, TextLayout::measureWidthPx("A", &emptyGlyphFont, 1));
}

void test_text_layout_wrap_empty_glyph_table_returns_zero(void) {
    TextLayout::WrappedLine lines[4];
    const uint8_t written = TextLayout::wrap("A", &emptyGlyphFont, 1, 100, 0, lines, 4);
    TEST_ASSERT_EQUAL_UINT8(0, written);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_text_layout_measure_cross_check_ascii_corpus);
    RUN_TEST(test_text_layout_measure_cross_check_latin1_corpus);
    RUN_TEST(test_text_layout_measure_width_px_null_font_no_default_returns_zero);

    RUN_TEST(test_text_layout_wrap_breaks_at_word_boundary);
    RUN_TEST(test_text_layout_wrap_word_longer_than_line_hard_breaks);
    RUN_TEST(test_text_layout_wrap_exactly_one_pixel_too_wide_wraps);
    RUN_TEST(test_text_layout_wrap_trailing_spaces_do_not_force_an_extra_line);
    RUN_TEST(test_text_layout_wrap_multiple_trailing_spaces_do_not_open_an_extra_line);
    RUN_TEST(test_text_layout_wrap_consecutive_newlines_keep_the_blank_line);
    RUN_TEST(test_text_layout_wrap_max_out_lines_overflow_no_oob);
    RUN_TEST(test_text_layout_wrap_max_out_lines_zero_returns_zero);
    RUN_TEST(test_text_layout_wrap_empty_string_returns_zero);
    RUN_TEST(test_text_layout_wrap_whitespace_only_returns_zero);
    RUN_TEST(test_text_layout_wrap_newline_is_hard_break);
    RUN_TEST(test_text_layout_wrap_max_width_too_small_for_one_glyph_returns_zero);
    RUN_TEST(test_text_layout_wrap_null_font_no_default_returns_zero);
    RUN_TEST(test_text_layout_wrap_null_out_lines_returns_zero);
    RUN_TEST(test_text_layout_wrap_uses_default_font);

    RUN_TEST(test_text_layout_wrap_skip_lines_mid_skip);
    RUN_TEST(test_text_layout_wrap_skip_lines_at_total_count_returns_zero);
    RUN_TEST(test_text_layout_wrap_skip_lines_past_end_returns_zero);

    RUN_TEST(test_text_layout_count_wrapped_lines_matches_wrap_count);
    RUN_TEST(test_text_layout_count_wrapped_lines_empty_and_whitespace);

    RUN_TEST(test_text_layout_malformed_utf8_lead_swallows_ascii_characterization);
    RUN_TEST(test_text_layout_malformed_utf8_wrap_stays_in_bounds_characterization);
    RUN_TEST(test_text_layout_malformed_utf8_four_byte_lead_characterization);

    RUN_TEST(test_text_layout_count_glyphs_ascii);
    RUN_TEST(test_text_layout_count_glyphs_latin1_two_byte);
    RUN_TEST(test_text_layout_byte_offset_for_glyphs_ascii);
    RUN_TEST(test_text_layout_byte_offset_for_glyphs_latin1_bridge);
    RUN_TEST(test_text_layout_byte_offset_for_glyphs_clamped_past_end);
    RUN_TEST(test_text_layout_byte_offset_for_glyphs_null_font_clamps_from_offset);

    RUN_TEST(test_text_layout_measure_width_px_empty_glyph_table_returns_zero);
    RUN_TEST(test_text_layout_wrap_empty_glyph_table_returns_zero);

    return UNITY_END();
}
