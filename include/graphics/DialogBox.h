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
 * leads, the 14 scalar/enum fields follow.
 */
struct DialogBoxStyle {
    const Font* font          = nullptr;  ///< nullptr uses FontManager's default.
    int16_t     x             = 0;
    int16_t     y             = 0;
    int16_t     w             = 0;
    int16_t     h             = 0;
    Color       panel         = Color::Black;
    Color       border        = Color::White;
    Color       ink           = Color::White;
    Color       inkDim        = Color::Gray;
    Color       inkSelected   = Color::Yellow;
    uint8_t     borderWidth   = 1;
    uint8_t     padding       = 4;
    uint8_t     textSize      = 1;
    uint8_t     lineSpacing   = 1;      ///< Extra px between wrapped body lines.
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
        int16_t choiceX, choiceY;     ///< Top-left of choice row 0.
        int16_t choiceW;              ///< Row width (panel inner width).
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
        renderer.drawText(text, layout.choiceX,
                           static_cast<int16_t>(layout.choiceY + i * layout.choiceRowHeightPx +
                                                 style_.padding),
                           color, style_.textSize, style_.font);
    }

    if (style_.fixedPosition) {
        renderer.setOffsetBypass(wasBypass);
    }

    lastRevision_ = runner.revision();
}

} // namespace pixelroot32::graphics
#endif // PIXELROOT32_ENABLE_DIALOG
