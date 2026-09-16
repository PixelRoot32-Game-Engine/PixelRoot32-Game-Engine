/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "platforms/PlatformDefaults.h"
#if PIXELROOT32_ENABLE_DIALOG
#include "gameplay/DialogRunner.h"
#include "graphics/Color.h"
#include "graphics/Font.h"
#include "graphics/TextLayout.h"
#include "platforms/EngineConfig.h"
#include <cstdint>

namespace pixelroot32::graphics {

/**
 * @struct DialogBoxStyle
 * @brief Every visual and layout knob DialogBox needs to draw a panel.
 *
 * Pointer first, tags last (the same field-packing convention
 * DialogRunner and DialogTypes follow, copied from StateMachine): `font`
 * leads, the 15 scalar/enum fields follow.
 *
 * Colours: `panel`, `border`, `ink`, `inkDim` and `inkSelected` are Color
 * names, not RGB565 values. DialogBox::draw() hands them to the renderer,
 * which resolves each one through a palette at draw time, exactly as for
 * any primitive or text. In single-palette mode that is the palette set by
 * setPalette() or setCustomPalette(). In dual-palette mode it is the sprite
 * palette, unless the renderer's render context is
 * PaletteContext::Background during the draw; Scene::draw() sets that
 * context only while drawing entities on render layer 0 and clears it
 * afterwards. A palette that stores 0x0000 in one of these slots draws that
 * element black: with the defaults, a zeroed Yellow slot makes the selected
 * choice invisible on the Black panel. Set these fields to slots the game's
 * palette defines, or keep the default slots (Black, White, Gray, Yellow)
 * populated in that palette. This is not hypothetical -- legend_of_clone
 * shipped exactly that palette. `choiceCaret` is the answer: DialogBox marks
 * the selected row with that glyph as well as with `inkSelected`, and draws
 * the caret in `ink`, NOT in `inkSelected`, on purpose. Selection therefore
 * has two independent signals, and the caret's own slot is the one the body
 * text already proves is visible; a caret drawn in `inkSelected` would
 * disappear in the very case it exists to cover. Setting `choiceCaret` to 0
 * disables the caret and gives back the pre-caret geometry exactly, at the
 * cost of returning selection to a single point of failure.
 *
 * Sizing: `padding` is applied once inside the border on every side, and
 * again above and below the text of every choice row. The height one line
 * needs, with L = font lineHeight x textSize, is:
 *
 * @code
 * bodyRowPx   = L + lineSpacing
 * choiceRowPx = L + 2 * padding
 * height      = 2 * (borderWidth + padding)
 *             + (speaker != nullptr ? bodyRowPx : 0)
 *             + bodyRows * bodyRowPx
 *             + (willPage ? bodyRowPx : 0)
 *             + choiceRows * choiceRowPx
 * @endcode
 *
 * `bodyRows` is the line's wrapped text row count, capped at
 * config::DialogMaxWrappedLines, wrapping at w - 2 * (borderWidth +
 * padding). `willPage` holds for a non-Choice line whose text wraps past
 * that cap, and reserves one row for the next-page cue. `choiceRows` is a
 * Choice line's declared choiceCount capped at config::DialogMaxChoices,
 * and 0 for any other kind. Padding therefore adds
 * 2 * padding * (1 + choiceRows) px, and it also narrows the wrap width,
 * which can add body rows. DialogBox::measureHeightPx() returns this height
 * for the tallest line of a script; compare it with the area the box must
 * fit.
 *
 * A non-zero `choiceCaret` also reserves a gutter to the left of every
 * option, as wide as the two-character slice {caret, ' '} at this font and
 * textSize (Layout::caretGutterPx). It costs no height, but it narrows the
 * room an option's text has: choice text is NOT wrapped, so a label that no
 * longer fits simply runs past the panel's inner edge. Keep the longest
 * option shorter than contentWidth - caretGutterPx, or set `choiceCaret`
 * to 0. The row rect choiceRect() reports is unaffected and still spans the
 * gutter, so the whole row stays tappable.
 */
struct DialogBoxStyle {
    const Font* font          = nullptr;  ///< nullptr uses FontManager's default.
    int16_t     x             = 0;
    int16_t     y             = 0;
    int16_t     w             = 0;
    int16_t     h             = 0;
    Color       panel         = Color::Black;   ///< Palette-resolved at draw time; see the colour note above.
    Color       border        = Color::White;   ///< Palette-resolved at draw time.
    Color       ink           = Color::White;   ///< Speaker, body and unselected choices. Palette-resolved.
    Color       inkDim        = Color::Gray;    ///< Next-page cue. Palette-resolved.
    Color       inkSelected   = Color::Yellow;  ///< Selected choice. Palette-resolved; a zeroed slot hides it.
    uint8_t     borderWidth   = 1;
    uint8_t     padding       = 4;              ///< Inside the border, and above and below each choice row.
    uint8_t     textSize      = 1;
    uint8_t     lineSpacing   = 1;      ///< Extra px between wrapped body lines.
    char        choiceCaret   = '>';    ///< Marks the selected choice. 0 disables the caret and its gutter.
    bool        fixedPosition = true;   ///< true: setOffsetBypass(true) while drawing, ignoring the camera.
};

/**
 * @class DialogBox
 * @brief Optional default dialog panel, driven by a DialogRunner.
 *
 * Deliberately NOT a UIElement: both demos build with UI_SYSTEM=0, and
 * UILabel::text / UILayout::elements are the engine's only two heap sites.
 * With UI_SYSTEM=1 there are two panel paths -- DialogBox is the dialog
 * path, UIPanel is the structured-HUD path. Zero heap allocation in every
 * method.
 */
class DialogBox {
public:
    /**
     * @brief Every pixel coordinate the box uses, computed once.
     *
     * draw() and choiceRect() both consume this via computeLayout(), so
     * hit-test and drawn geometry cannot drift apart. measureHeightPx() has
     * no DialogRunner to build one from, so it shares computeLayout()'s
     * row-height/content-height helpers (DialogBox.cpp) instead, keeping
     * the two in agreement. Stack-local, zero heap.
     *
     * Deliberately left without a Doxygen struct tag: scripts/generate_api_docs.py
     * resolves a documented method against class_spans[-1], the most
     * recently tagged type rather than the enclosing one, so tagging a
     * nested type orphans every method documented after it (see
     * TextLayout::WrappedLine for the same note). Tag this struct only
     * together with a generator fix, and regenerate docs/api/generated/ to
     * confirm.
     */
    struct Layout {
        int16_t panelX, panelY, panelW, panelH;
        int16_t speakerX, speakerY;   ///< Valid when hasSpeaker.
        int16_t bodyX, bodyY;         ///< Top-left of body line 0.
        int16_t choiceX, choiceY;     ///< Top-left of choice row 0, gutter included.
        int16_t choiceW;              ///< Row width (panel inner width), gutter included.
        int16_t caretGutterPx;        ///< Width reserved for the caret; 0 when it is disabled.
        int16_t choiceTextX;          ///< choiceX + caretGutterPx. Where option text starts.
        int16_t bodyLineHeightPx;
        int16_t choiceRowHeightPx;
        uint8_t bodyLineCount;        ///< Rows valid in bodyLines.
        uint8_t choiceCount;          ///< Already clamped to DialogMaxChoices.
        uint8_t pageCount;            ///< Total pages of the current line's text.
        uint8_t page;                 ///< Zero-based current page index.
        bool    hasSpeaker;
        TextLayout::WrappedLine bodyLines[platforms::config::DialogMaxWrappedLines];
    };

    /**
     * @brief Sets the visual/layout style used by every subsequent call.
     * @param style The style to apply.
     */
    void setStyle(const DialogBoxStyle& style) { style_ = style; }

    /**
     * @brief The style currently in effect.
     * @return A const reference to the active DialogBoxStyle.
     */
    [[nodiscard]] const DialogBoxStyle& style() const { return style_; }

    /**
     * @brief THE single geometry producer. Pure, static, no Renderer, no mutation.
     * @param style The style to lay out against.
     * @param runner The runner to read the current line, page and choice state from.
     * @param outLayout Written with every coordinate draw()/choiceRect() need.
     *
     * Wraps the current line's text for `runner.page()` via
     * TextLayout::wrap(..., skipLines = page * DialogMaxWrappedLines, ...).
     * When the runner has no current line (Inactive or Finished), every
     * count in `outLayout` is zeroed and no wrap runs -- there is nothing
     * to lay out.
     */
    static void computeLayout(const DialogBoxStyle&         style,
                               const gameplay::DialogRunner& runner,
                               Layout&                       outLayout);

    /**
     * @brief Draws the panel, border, speaker, body page and option list.
     * @tparam RendererT The renderer type to draw through.
     * @param renderer The renderer to issue draw calls against.
     * @param runner The runner to present. Read for layout; setPageCount()
     *        is the only mutation this method makes.
     *
     * Templated on the renderer type because Renderer declares NO virtual
     * methods: a Renderer& parameter would statically dispatch past
     * MockRenderer and make this class unassertable. Zero vtable, zero
     * heap. Takes the runner NON-const solely to call
     * runner.setPageCount() with the count only this class can compute
     * (pageCountFor()); it never changes state and never emits an event.
     * A no-op when the runner has no current line.
     */
    template <typename RendererT>
    void draw(RendererT& renderer, gameplay::DialogRunner& runner);

    /**
     * @brief Whether the runner changed since the last draw().
     * @param runner The runner to check.
     * @return true when `runner.revision()` differs from the value observed
     *         by the last draw() call.
     *
     * An optimisation hint only -- draw() is always safe to call
     * unconditionally. Compares revision() by inequality; never orders it.
     */
    [[nodiscard]] bool needsRedraw(const gameplay::DialogRunner& runner) const;

    /**
     * @brief Rect of option `index`, for touch hit-testing.
     * @param runner The runner to read choice state from.
     * @param index Zero-based option index.
     * @param outXPx Written with the row's top-left X.
     * @param outYPx Written with the row's top-left Y.
     * @param outWPx Written with the row's width.
     * @param outHPx Written with the row's height.
     * @return false when `index >= choiceCount()` or the runner is not
     *         showing choices; outputs are untouched. true otherwise.
     *
     * Derived from the SAME computeLayout() call draw() uses, so hit-test
     * and draw geometry cannot drift apart.
     */
    [[nodiscard]] bool choiceRect(const gameplay::DialogRunner& runner,
                                   uint8_t                       index,
                                   int16_t&                      outXPx,
                                   int16_t&                      outYPx,
                                   int16_t&                      outWPx,
                                   int16_t&                      outHPx) const;

    /**
     * @brief Pages the given line needs at this style.
     * @param line The line to measure.
     * @param style The style to wrap against.
     * @return The number of pages `line`'s body text needs at `style`'s
     *         width, at least 1; always exactly 1 for a `LineKind::Choice`
     *         line, since DialogRunner ignores Advance while ShowingChoices.
     *         Explicit path for games that draw the panel themselves.
     */
    [[nodiscard]] static uint8_t pageCountFor(const gameplay::DialogLine& line,
                                               const DialogBoxStyle&       style);

    /**
     * @brief Minimum panel height in px for the tallest line in `script`.
     * @param script The script to measure every line of.
     * @param style The style to measure against.
     * @return The minimum panel height, in pixels, that fits the tallest
     *         single page any line in `script` can produce.
     * @note This is the way to check that a style fits a fixed area such
     *       as a HUD strip before choosing DialogBoxStyle::h, which draw()
     *       uses as-is without clipping. The per-line height formula, and
     *       how `padding` enters it, is in the DialogBoxStyle description.
     */
    [[nodiscard]] static int16_t measureHeightPx(const gameplay::DialogScript& script,
                                                  const DialogBoxStyle&         style);

private:
    DialogBoxStyle style_{};
    uint16_t       lastRevision_ = 0xFFFF;  ///< Sentinel; compared by inequality only.
};

/// RAM regression guard, mirroring DialogRunner's static_assert. Expressed
/// against sizeof(void*) so one formula covers ESP32 (32-bit) and 64-bit
/// native: DialogBoxStyle's single pointer plus lastRevision_ never needs
/// more than 2 pointer-widths of slack beyond a fixed 24-byte core.
/// Actual: 28 B on ESP32, 40 B on 64-bit native (test_dialog_box_sizeof_guard
/// pins both exactly). Growing this struct must be a conscious bump of the
/// threshold below, not a silent drift.
static_assert(sizeof(DialogBox) <= 2 * sizeof(void*) + 24,
              "DialogBox exceeds its RAM budget (2*sizeof(void*)+24 bytes); "
              "if this growth is intentional, raise the threshold above and "
              "update test_dialog_box_sizeof_guard");

template <typename RendererT>
void DialogBox::draw(RendererT& renderer, gameplay::DialogRunner& runner) {
    const gameplay::DialogLine* line = runner.currentLine();
    if (line == nullptr) {
        return;
    }

    const bool wasBypass = renderer.isOffsetBypassEnabled();
    if (style_.fixedPosition) {
        renderer.setOffsetBypass(true);
    }

    // Only this class can wrap the current line's text, so only this class
    // can know how many pages it needs -- the runner is headless (no Font,
    // no panel width) and cannot derive this itself. computeLayout() below
    // reads back whatever setPageCount() just settled on.
    runner.setPageCount(pageCountFor(*line, style_));

    Layout layout{};
    computeLayout(style_, runner, layout);

    renderer.drawFilledRectangle(layout.panelX, layout.panelY, layout.panelW, layout.panelH,
                                  style_.panel);
    for (uint8_t b = 0; b < style_.borderWidth; ++b) {
        renderer.drawRectangle(static_cast<int16_t>(layout.panelX + b),
                                static_cast<int16_t>(layout.panelY + b),
                                static_cast<int16_t>(layout.panelW - 2 * b),
                                static_cast<int16_t>(layout.panelH - 2 * b), style_.border);
    }

    if (layout.hasSpeaker) {
        renderer.drawText(std::string_view(line->speaker), layout.speakerX, layout.speakerY,
                           style_.ink, style_.textSize, style_.font);
    }

    for (uint8_t i = 0; i < layout.bodyLineCount; ++i) {
        renderer.drawText(layout.bodyLines[i].slice, layout.bodyX,
                           static_cast<int16_t>(layout.bodyY + i * layout.bodyLineHeightPx),
                           style_.ink, style_.textSize, style_.font);
    }

    // "Next page" cue: only meaningful once there is body text to page
    // through, and only while a later page remains.
    if (layout.bodyLineCount > 0 &&
        static_cast<uint8_t>(layout.page + 1) < layout.pageCount) {
        renderer.drawText(std::string_view(">"), layout.bodyX,
                           static_cast<int16_t>(layout.bodyY +
                                                 layout.bodyLineCount * layout.bodyLineHeightPx),
                           style_.inkDim, style_.textSize, style_.font);
    }

    for (uint8_t i = 0; i < layout.choiceCount; ++i) {
        const gameplay::DialogChoice* choice = runner.choice(i);
        const std::string_view text = (choice != nullptr && choice->text != nullptr)
                                           ? std::string_view(choice->text)
                                           : std::string_view{};
        const Color color = (i == runner.selectedChoice()) ? style_.inkSelected : style_.ink;
        // Row height reserves style_.padding above AND below the glyphs
        // (see computeLayout()'s choiceRowHeightPx), so the text baseline
        // sits one padding down from the row's top -- exactly what
        // choiceRect() reports as that row's top. This is the relationship
        // the anti-drift test pins: a future change to either offset alone
        // would desync draw() from choiceRect() and fail it.
        const int16_t rowTextY = static_cast<int16_t>(
            layout.choiceY + i * layout.choiceRowHeightPx + style_.padding);

        if (style_.choiceCaret != 0 && i == runner.selectedChoice()) {
            // Drawn in style_.ink, NOT style_.inkSelected -- deliberately.
            // The caret exists so that selection has a second signal that is
            // independent of the inkSelected palette slot: legend_of_clone
            // shipped a sprite palette with a zeroed Yellow slot, and the
            // selected choice rendered black-on-black, leaving the player no
            // way to see what was selected. A caret sharing the very colour
            // slot it insures against would insure nothing; the body text
            // above already proves `ink` resolves to a visible slot.
            renderer.drawText(std::string_view(&style_.choiceCaret, 1), layout.choiceX, rowTextY,
                               style_.ink, style_.textSize, style_.font);
        }

        renderer.drawText(text, layout.choiceTextX, rowTextY, color, style_.textSize,
                           style_.font);
    }

    if (style_.fixedPosition) {
        renderer.setOffsetBypass(wasBypass);
    }

    lastRevision_ = runner.revision();
}

} // namespace pixelroot32::graphics
#endif // PIXELROOT32_ENABLE_DIALOG
