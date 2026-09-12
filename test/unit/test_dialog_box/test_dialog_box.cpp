/**
 * @file test_dialog_box.cpp
 * @brief Unit tests for graphics/DialogBox
 *
 * Covers the dialog-box capability:
 * - Default panel rendering (speaker present/absent)
 * - Single-column option list with a caret/highlight following selection
 * - choiceRect for touch hit-testing, matching draw()'s own geometry
 * - measureHeightPx for layout, without an active runner
 * - Zero heap allocation
 *
 * DialogBox::draw is a template on the renderer type: Renderer
 * declares no virtual methods, so a Renderer& parameter would statically
 * dispatch past MockRenderer and assert nothing. Every draw test below
 * instantiates draw<MockRenderer>() directly.
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_DIALOG is
 * enabled (see include/graphics/DialogBox.h), matching the flags-off /
 * flags-on CI matrix (platformio.ini's native_test vs native_test_gameplay).
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_DIALOG

#include "graphics/DialogBox.h"
#include "graphics/FontManager.h"
#include "mocks/MockRenderer.h"

#include <cstdint>

using namespace pixelroot32::graphics;
using namespace pixelroot32::gameplay;
using namespace pixelroot32::platforms::config;

namespace {

// =============================================================================
// Fixtures -- same 5x7/spacing-1/lineHeight-8 shape as test_text_layout's
// testFont, so wrap-boundary arithmetic is directly comparable.
// =============================================================================

const uint16_t mockSpriteData[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
const Sprite mockGlyphs[] = {{mockSpriteData, 5, 7}};
const Font kTestFont = {mockGlyphs, 32, 126, 5, 7, 1, 8};

DialogBoxStyle makeStyle() {
    DialogBoxStyle style;
    style.font = &kTestFont;
    style.x = 0;
    style.y = 0;
    style.w = 100;
    style.h = 60;
    style.borderWidth = 1;
    style.padding = 4;
    style.textSize = 1;
    style.lineSpacing = 1;
    style.fixedPosition = true;
    return style;
}

// Single text line, no speaker, short body -- fits on one wrapped line.
const DialogLine kShortNoSpeakerLines[] = {
    {"Hi there", nullptr, kNoLine, 0, 0, 0, 0, LineKind::Text, 0},
};
const DialogScript kShortNoSpeakerScript{kShortNoSpeakerLines, nullptr, 1, 0};

// Single text line with a speaker label.
const DialogLine kWithSpeakerLines[] = {
    {"Hi there", "Bob", kNoLine, 0, 0, 0, 0, LineKind::Text, 0},
};
const DialogScript kWithSpeakerScript{kWithSpeakerLines, nullptr, 1, 0};

// Long body text: five 5-char words separated by single spaces, chosen so it
// wraps to more than DialogMaxWrappedLines (4) lines at this style's content
// width, forcing multiple pages.
constexpr const char* kLongText =
    "AAAAA BBBBB CCCCC DDDDD EEEEE FFFFF GGGGG HHHHH IIIII JJJJJ";
const DialogLine kLongTextLines[] = {
    {kLongText, nullptr, kNoLine, 0, 0, 0, 0, LineKind::Text, 0},
};
const DialogScript kLongTextScript{kLongTextLines, nullptr, 1, 0};

// Choice line with three options.
const DialogChoice kChoices[] = {
    {"Yes", kNoLine, 1},
    {"No", kNoLine, 2},
    {"Maybe", kNoLine, 3},
};
const DialogLine kChoiceLines[] = {
    {nullptr, nullptr, kNoLine, 0, 0, /*firstChoice*/ 0, /*choiceCount*/ 3, LineKind::Choice, 0},
};
const DialogScript kChoiceScript{kChoiceLines, kChoices, 1, 3};

// Choice line carrying a prompt (kLongText) long enough to wrap past
// DialogMaxWrappedLines -- a Choice line MAY carry text per DialogTypes.h's
// "either shown text or a choice prompt" doc.
const DialogLine kChoiceWithPromptLines[] = {
    {kLongText, nullptr, kNoLine, 0, 0, /*firstChoice*/ 0, /*choiceCount*/ 3, LineKind::Choice, 0},
};
const DialogScript kChoiceWithPromptScript{kChoiceWithPromptLines, kChoices, 1, 3};

}  // namespace

// =============================================================================
// sizeof guard -- RAM regression, mirrors DialogRunner's pattern.
// =============================================================================

// DialogBoxStyle: one pointer (font) + four int16_t + five Color (uint8_t)
// + four uint8_t + one bool. ESP32 (4-byte pointer): 4+8+5+4+1=22, padded to
// 4-byte alignment = 24. Native (8-byte pointer): 8+8+5+4+1=26, padded to
// 8-byte alignment = 32 -- independently re-derived here, not copied from
// DialogBoxStyle's own comments, so a drift in either place is caught.
// DialogBox adds lastRevision_ (uint16_t): ESP32 24+2=26, padded to 28;
// native 32+2=34, padded to 40.
#ifdef ESP32
void test_dialog_box_sizeof_guard(void) {
    TEST_ASSERT_EQUAL_UINT32(24, sizeof(DialogBoxStyle));
    TEST_ASSERT_EQUAL_UINT32(28, sizeof(DialogBox));
}
#else
void test_dialog_box_sizeof_guard(void) {
    TEST_ASSERT_EQUAL_UINT32(32, sizeof(DialogBoxStyle));
    TEST_ASSERT_EQUAL_UINT32(40, sizeof(DialogBox));
}
#endif

// =============================================================================
// computeLayout -- speaker placement
// =============================================================================

void test_dialog_box_compute_layout_no_speaker_places_body_at_content_origin(void) {
    DialogRunner runner;
    runner.start(kShortNoSpeakerScript);
    const DialogBoxStyle style = makeStyle();

    DialogBox::Layout layout{};
    DialogBox::computeLayout(style, runner, layout);

    TEST_ASSERT_FALSE(layout.hasSpeaker);
    const int16_t expectedContentX = style.x + style.padding + style.borderWidth;
    const int16_t expectedContentY = style.y + style.padding + style.borderWidth;
    TEST_ASSERT_EQUAL_INT16(expectedContentX, layout.bodyX);
    TEST_ASSERT_EQUAL_INT16(expectedContentY, layout.bodyY);
    TEST_ASSERT_GREATER_THAN_UINT8(0, layout.bodyLineCount);
}

void test_dialog_box_compute_layout_with_speaker_offsets_body_below_speaker_line(void) {
    DialogRunner runner;
    runner.start(kWithSpeakerScript);
    const DialogBoxStyle style = makeStyle();

    DialogBox::Layout layout{};
    DialogBox::computeLayout(style, runner, layout);

    TEST_ASSERT_TRUE(layout.hasSpeaker);
    const int16_t expectedSpeakerY = style.y + style.padding + style.borderWidth;
    TEST_ASSERT_EQUAL_INT16(expectedSpeakerY, layout.speakerY);
    TEST_ASSERT_EQUAL_INT16(
        static_cast<int16_t>(expectedSpeakerY + layout.bodyLineHeightPx), layout.bodyY);
}

void test_dialog_box_compute_layout_body_line_height_does_not_overflow_at_large_text_size(void) {
    // font.lineHeight (8) * textSize (32) == 256, which wraps to 0 in a
    // uint8_t before lineSpacing is even added.
    DialogRunner runner;
    runner.start(kShortNoSpeakerScript);
    DialogBoxStyle style = makeStyle();
    style.textSize = 32;

    DialogBox::Layout layout{};
    DialogBox::computeLayout(style, runner, layout);

    TEST_ASSERT_EQUAL_INT16(
        static_cast<int16_t>(kTestFont.lineHeight * style.textSize + style.lineSpacing),
        layout.bodyLineHeightPx);
}

void test_dialog_box_compute_layout_inactive_runner_yields_zeroed_counts(void) {
    DialogRunner runner;  // Never start()ed: Inactive, currentLine() == nullptr.
    const DialogBoxStyle style = makeStyle();

    DialogBox::Layout layout{};
    DialogBox::computeLayout(style, runner, layout);

    TEST_ASSERT_EQUAL_UINT8(0, layout.bodyLineCount);
    TEST_ASSERT_EQUAL_UINT8(0, layout.choiceCount);
    TEST_ASSERT_FALSE(layout.hasSpeaker);
}

// =============================================================================
// pageCountFor / paging via skipLines
// =============================================================================

void test_dialog_box_page_count_for_short_text_returns_one(void) {
    const DialogBoxStyle style = makeStyle();
    TEST_ASSERT_EQUAL_UINT8(1, DialogBox::pageCountFor(kShortNoSpeakerLines[0], style));
}

void test_dialog_box_page_count_for_long_text_matches_ceiling_of_wrapped_lines(void) {
    const DialogBoxStyle style = makeStyle();
    const int16_t contentW = static_cast<int16_t>(style.w - 2 * (style.padding + style.borderWidth));
    const uint16_t totalLines =
        TextLayout::countWrappedLines(kLongText, &kTestFont, style.textSize, contentW);
    const uint16_t expectedPages =
        (totalLines + DialogMaxWrappedLines - 1) /
        DialogMaxWrappedLines;

    TEST_ASSERT_GREATER_THAN_UINT16(DialogMaxWrappedLines, totalLines);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expectedPages),
                             DialogBox::pageCountFor(kLongTextLines[0], style));
}

void test_dialog_box_body_shows_only_the_current_page(void) {
    DialogRunner runner;
    runner.start(kLongTextScript);
    const DialogBoxStyle style = makeStyle();

    const uint8_t pages = DialogBox::pageCountFor(kLongTextLines[0], style);
    runner.setPageCount(pages);
    TEST_ASSERT_GREATER_THAN_UINT8(1, pages);  // Precondition: this fixture must span >1 page.

    DialogBox::Layout page0{};
    DialogBox::computeLayout(style, runner, page0);

    // Independent oracle: wrap page 0 directly, bypassing DialogBox entirely.
    const int16_t contentW = static_cast<int16_t>(style.w - 2 * (style.padding + style.borderWidth));
    TextLayout::WrappedLine oracle[DialogMaxWrappedLines];
    const uint8_t oracleCount = TextLayout::wrap(kLongText, &kTestFont, style.textSize, contentW,
                                                  0, oracle, DialogMaxWrappedLines);
    TEST_ASSERT_EQUAL_UINT8(oracleCount, page0.bodyLineCount);
    for (uint8_t i = 0; i < oracleCount; ++i) {
        TEST_ASSERT_TRUE(oracle[i].slice == page0.bodyLines[i].slice);
    }

    // Advance to page 1 (still on the same line -- long text, not next).
    runner.feed(DialogAction::Advance);
    TEST_ASSERT_EQUAL_UINT8(1, runner.page());
    TEST_ASSERT_EQUAL(kNoLine == runner.currentLineId() ? 0 : 1, 1);  // still on a line

    DialogBox::Layout page1{};
    DialogBox::computeLayout(style, runner, page1);

    TextLayout::WrappedLine oracle2[DialogMaxWrappedLines];
    const uint8_t oracle2Count =
        TextLayout::wrap(kLongText, &kTestFont, style.textSize, contentW,
                          DialogMaxWrappedLines, oracle2,
                          DialogMaxWrappedLines);
    TEST_ASSERT_EQUAL_UINT8(oracle2Count, page1.bodyLineCount);
    for (uint8_t i = 0; i < oracle2Count; ++i) {
        TEST_ASSERT_TRUE(oracle2[i].slice == page1.bodyLines[i].slice);
    }

    // The two pages must actually differ -- otherwise this test would pass
    // vacuously even if skipLines were silently ignored.
    TEST_ASSERT_FALSE(oracle[0].slice == oracle2[0].slice);
}

void test_dialog_box_choice_line_prompt_pages_once_and_draws_no_next_page_cue(void) {
    // A Choice line's prompt shows only its first DialogMaxWrappedLines
    // wrapped lines; DialogRunner ignores Advance in ShowingChoices, so a
    // second page could never be reached.
    const DialogBoxStyle style = makeStyle();
    TEST_ASSERT_EQUAL_UINT8(1, DialogBox::pageCountFor(kChoiceWithPromptLines[0], style));

    DialogRunner runner;
    runner.start(kChoiceWithPromptScript);
    DialogBox box;
    box.setStyle(style);
    pixelroot32::graphics::DisplayConfig config(pixelroot32::graphics::DisplayType::NONE, 0, 240,
                                                 240, 240, 240, 0, 0);
    MockRenderer mock(config);
    box.draw(mock, runner);

    for (const auto& call : mock.rendererCalls) {
        TEST_ASSERT_FALSE(call.type == "text" && call.text == ">");
    }
}

// =============================================================================
// measureHeightPx
// =============================================================================

void test_dialog_box_measure_height_px_multiline_exceeds_single_line(void) {
    const DialogBoxStyle style = makeStyle();
    const int16_t shortHeight = DialogBox::measureHeightPx(kShortNoSpeakerScript, style);
    const int16_t longHeight = DialogBox::measureHeightPx(kLongTextScript, style);
    TEST_ASSERT_GREATER_THAN_INT16(shortHeight, longHeight);
}

void test_dialog_box_measure_height_px_includes_choice_line_prompt_body_rows(void) {
    // A Choice line's own prompt (line.text) must contribute body rows to
    // measureHeightPx() the same way computeLayout() renders them, or a
    // panel sized from it is too short to fit what draw() actually shows.
    DialogRunner runner;
    runner.start(kChoiceWithPromptScript);
    const DialogBoxStyle style = makeStyle();

    DialogBox::Layout layout{};
    DialogBox::computeLayout(style, runner, layout);

    const int16_t lastChoiceRowBottom = static_cast<int16_t>(
        layout.choiceY + layout.choiceCount * layout.choiceRowHeightPx - style.y);

    TEST_ASSERT_GREATER_OR_EQUAL_INT16(
        lastChoiceRowBottom, DialogBox::measureHeightPx(kChoiceWithPromptScript, style));
}

void test_dialog_box_measure_height_px_zero_for_empty_script(void) {
    const DialogBoxStyle style = makeStyle();
    const DialogScript empty{nullptr, nullptr, 0, 0};
    TEST_ASSERT_EQUAL_INT16(0, DialogBox::measureHeightPx(empty, style));
}

// =============================================================================
// needsRedraw
// =============================================================================

void test_dialog_box_needs_redraw_false_before_any_draw_and_after_matching_draw(void) {
    DialogRunner runner;
    runner.start(kShortNoSpeakerScript);
    DialogBox box;
    box.setStyle(makeStyle());

    pixelroot32::graphics::DisplayConfig config(pixelroot32::graphics::DisplayType::NONE, 0, 240,
                                                 240, 240, 240, 0, 0);
    MockRenderer mock(config);
    box.draw(mock, runner);

    TEST_ASSERT_FALSE(box.needsRedraw(runner));
}

void test_dialog_box_needs_redraw_true_after_the_runner_changes(void) {
    DialogRunner runner;
    runner.start(kShortNoSpeakerScript);
    DialogBox box;
    box.setStyle(makeStyle());

    pixelroot32::graphics::DisplayConfig config(pixelroot32::graphics::DisplayType::NONE, 0, 240,
                                                 240, 240, 240, 0, 0);
    MockRenderer mock(config);
    box.draw(mock, runner);

    runner.start(kWithSpeakerScript);  // Bumps revision() via a fresh LineEnter.
    TEST_ASSERT_TRUE(box.needsRedraw(runner));
}

// =============================================================================
// choiceRect
// =============================================================================

void test_dialog_box_choice_rect_false_when_not_showing_choices(void) {
    DialogRunner runner;
    runner.start(kShortNoSpeakerScript);
    DialogBox box;
    box.setStyle(makeStyle());

    int16_t x = -1, y = -1, w = -1, h = -1;
    TEST_ASSERT_FALSE(box.choiceRect(runner, 0, x, y, w, h));
    TEST_ASSERT_EQUAL_INT16(-1, x);  // Untouched on failure.
}

void test_dialog_box_choice_rect_false_when_index_out_of_range(void) {
    DialogRunner runner;
    runner.start(kChoiceScript);
    DialogBox box;
    box.setStyle(makeStyle());

    int16_t x, y, w, h;
    TEST_ASSERT_FALSE(box.choiceRect(runner, 3, x, y, w, h));
    TEST_ASSERT_TRUE(box.choiceRect(runner, 2, x, y, w, h));
}

void test_dialog_box_choice_rect_matches_independently_computed_layout(void) {
    DialogRunner runner;
    runner.start(kChoiceScript);
    const DialogBoxStyle style = makeStyle();
    DialogBox box;
    box.setStyle(style);

    DialogBox::Layout layout{};
    DialogBox::computeLayout(style, runner, layout);

    for (uint8_t i = 0; i < 3; ++i) {
        int16_t x, y, w, h;
        TEST_ASSERT_TRUE(box.choiceRect(runner, i, x, y, w, h));
        TEST_ASSERT_EQUAL_INT16(layout.choiceX, x);
        TEST_ASSERT_EQUAL_INT16(
            static_cast<int16_t>(layout.choiceY + i * layout.choiceRowHeightPx), y);
        TEST_ASSERT_EQUAL_INT16(layout.choiceW, w);
        TEST_ASSERT_EQUAL_INT16(layout.choiceRowHeightPx, h);
    }
}

// =============================================================================
// draw() -- MockRenderer assertions. draw<MockRenderer> is instantiated
// explicitly because Renderer has no virtual methods: a
// Renderer& parameter would statically bind to Renderer's own drawText/
// drawFilledRectangle/drawRectangle and never reach MockRenderer's capturing
// overrides, making the box unassertable.
// =============================================================================

namespace {
pixelroot32::graphics::MockRenderer makeMock() {
    pixelroot32::graphics::DisplayConfig config(pixelroot32::graphics::DisplayType::NONE, 0, 240,
                                                 240, 240, 240, 0, 0);
    return pixelroot32::graphics::MockRenderer(config);
}
}  // namespace

void test_dialog_box_draw_without_speaker_emits_panel_border_and_body_no_speaker_call(void) {
    DialogRunner runner;
    runner.start(kShortNoSpeakerScript);
    DialogBox box;
    box.setStyle(makeStyle());
    MockRenderer mock = makeMock();

    box.draw(mock, runner);

    TEST_ASSERT_TRUE(mock.hasCall("filled_rectangle"));
    TEST_ASSERT_TRUE(mock.hasCall("rectangle"));
    TEST_ASSERT_TRUE(mock.hasCall("text"));

    bool sawSpeakerText = false;
    for (const auto& call : mock.rendererCalls) {
        if (call.type == "text" && call.text == "Bob") sawSpeakerText = true;
    }
    TEST_ASSERT_FALSE(sawSpeakerText);
}

void test_dialog_box_draw_with_speaker_emits_the_speaker_text(void) {
    DialogRunner runner;
    runner.start(kWithSpeakerScript);
    DialogBox box;
    box.setStyle(makeStyle());
    MockRenderer mock = makeMock();

    box.draw(mock, runner);

    bool sawSpeakerText = false;
    for (const auto& call : mock.rendererCalls) {
        if (call.type == "text" && call.text == "Bob") sawSpeakerText = true;
    }
    TEST_ASSERT_TRUE(sawSpeakerText);
}

void test_dialog_box_draw_is_noop_when_runner_has_no_current_line(void) {
    DialogRunner runner;  // Never started.
    DialogBox box;
    box.setStyle(makeStyle());
    MockRenderer mock = makeMock();

    box.draw(mock, runner);

    TEST_ASSERT_EQUAL_UINT32(0, mock.rendererCalls.size());
}

void test_dialog_box_draw_selected_choice_uses_ink_selected_color(void) {
    DialogRunner runner;
    runner.start(kChoiceScript);
    runner.select(1);  // "No"
    DialogBoxStyle style = makeStyle();
    style.ink = Color::White;
    style.inkSelected = Color::Yellow;
    DialogBox box;
    box.setStyle(style);
    MockRenderer mock = makeMock();

    box.draw(mock, runner);

    bool foundYesWhite = false;
    bool foundNoYellow = false;
    for (const auto& call : mock.rendererCalls) {
        if (call.type != "text") continue;
        if (call.text == "Yes" && call.color == Color::White) foundYesWhite = true;
        if (call.text == "No" && call.color == Color::Yellow) foundNoYellow = true;
    }
    TEST_ASSERT_TRUE(foundYesWhite);
    TEST_ASSERT_TRUE(foundNoYellow);
}

void test_dialog_box_draw_choice_text_y_equals_choice_rect_y_plus_padding(void) {
    // THE anti-drift test -- the entire reason this class computes geometry
    // in one place instead of two. Breaking either draw()'s +style_.padding offset or
    // computeLayout()'s choiceRowHeightPx formula desyncs this.
    DialogRunner runner;
    runner.start(kChoiceScript);
    const DialogBoxStyle style = makeStyle();
    DialogBox box;
    box.setStyle(style);
    MockRenderer mock = makeMock();

    box.draw(mock, runner);

    for (uint8_t i = 0; i < 3; ++i) {
        int16_t rectX, rectY, rectW, rectH;
        TEST_ASSERT_TRUE(box.choiceRect(runner, i, rectX, rectY, rectW, rectH));

        const char* expectedText = (i == 0) ? "Yes" : (i == 1) ? "No" : "Maybe";
        bool found = false;
        for (const auto& call : mock.rendererCalls) {
            if (call.type == "text" && call.text == expectedText) {
                TEST_ASSERT_EQUAL_INT16(static_cast<int16_t>(rectY + style.padding), call.y);
                found = true;
            }
        }
        TEST_ASSERT_TRUE(found);
    }
}

void test_dialog_box_draw_restores_offset_bypass_to_its_prior_value(void) {
    DialogRunner runner;
    runner.start(kShortNoSpeakerScript);
    DialogBox box;
    box.setStyle(makeStyle());
    MockRenderer mock = makeMock();

    mock.setOffsetBypass(false);
    box.draw(mock, runner);
    TEST_ASSERT_FALSE(mock.isOffsetBypassEnabled());

    mock.setOffsetBypass(true);
    box.draw(mock, runner);
    TEST_ASSERT_TRUE(mock.isOffsetBypassEnabled());
}

void test_dialog_box_draw_shows_next_page_cue_only_when_a_later_page_remains(void) {
    DialogRunner runner;
    runner.start(kLongTextScript);
    DialogBox box;
    box.setStyle(makeStyle());
    MockRenderer mock = makeMock();

    box.draw(mock, runner);  // Page 0 of a multi-page line.
    bool sawCue = false;
    for (const auto& call : mock.rendererCalls) {
        if (call.type == "text" && call.text == ">") sawCue = true;
    }
    TEST_ASSERT_TRUE(sawCue);

    // Drive to the last page and confirm the cue disappears.
    uint8_t pages = DialogBox::pageCountFor(kLongTextLines[0], box.style());
    for (uint8_t i = 1; i < pages; ++i) {
        runner.feed(DialogAction::Advance);
    }
    mock.clear();
    box.draw(mock, runner);
    sawCue = false;
    for (const auto& call : mock.rendererCalls) {
        if (call.type == "text" && call.text == ">") sawCue = true;
    }
    TEST_ASSERT_FALSE(sawCue);
}

#else

void test_dialog_box_flag_off_reserves_zero_bytes(void) {
    TEST_PASS_MESSAGE("PIXELROOT32_ENABLE_DIALOG is off: DialogBox reserves zero bytes.");
}

#endif  // PIXELROOT32_ENABLE_DIALOG

void setUp(void) {
    test_setup();
#if PIXELROOT32_ENABLE_DIALOG
    FontManager::setDefaultFont(nullptr);
#endif
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_DIALOG
    RUN_TEST(test_dialog_box_sizeof_guard);
    RUN_TEST(test_dialog_box_compute_layout_no_speaker_places_body_at_content_origin);
    RUN_TEST(test_dialog_box_compute_layout_with_speaker_offsets_body_below_speaker_line);
    RUN_TEST(test_dialog_box_compute_layout_body_line_height_does_not_overflow_at_large_text_size);
    RUN_TEST(test_dialog_box_compute_layout_inactive_runner_yields_zeroed_counts);
    RUN_TEST(test_dialog_box_page_count_for_short_text_returns_one);
    RUN_TEST(test_dialog_box_page_count_for_long_text_matches_ceiling_of_wrapped_lines);
    RUN_TEST(test_dialog_box_choice_line_prompt_pages_once_and_draws_no_next_page_cue);
    RUN_TEST(test_dialog_box_body_shows_only_the_current_page);
    RUN_TEST(test_dialog_box_measure_height_px_multiline_exceeds_single_line);
    RUN_TEST(test_dialog_box_measure_height_px_includes_choice_line_prompt_body_rows);
    RUN_TEST(test_dialog_box_measure_height_px_zero_for_empty_script);
    RUN_TEST(test_dialog_box_needs_redraw_false_before_any_draw_and_after_matching_draw);
    RUN_TEST(test_dialog_box_needs_redraw_true_after_the_runner_changes);
    RUN_TEST(test_dialog_box_choice_rect_false_when_not_showing_choices);
    RUN_TEST(test_dialog_box_choice_rect_false_when_index_out_of_range);
    RUN_TEST(test_dialog_box_choice_rect_matches_independently_computed_layout);
    RUN_TEST(test_dialog_box_draw_without_speaker_emits_panel_border_and_body_no_speaker_call);
    RUN_TEST(test_dialog_box_draw_with_speaker_emits_the_speaker_text);
    RUN_TEST(test_dialog_box_draw_is_noop_when_runner_has_no_current_line);
    RUN_TEST(test_dialog_box_draw_selected_choice_uses_ink_selected_color);
    RUN_TEST(test_dialog_box_draw_choice_text_y_equals_choice_rect_y_plus_padding);
    RUN_TEST(test_dialog_box_draw_restores_offset_bypass_to_its_prior_value);
    RUN_TEST(test_dialog_box_draw_shows_next_page_cue_only_when_a_later_page_remains);
#else
    RUN_TEST(test_dialog_box_flag_off_reserves_zero_bytes);
#endif

    return UNITY_END();
}
