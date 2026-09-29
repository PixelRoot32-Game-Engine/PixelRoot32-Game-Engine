/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/DialogBox.h"

#if PIXELROOT32_ENABLE_DIALOG

#include "graphics/FontManager.h"
#include "graphics/Renderer.h"  // Sprite definition: DialogLine::portrait is opaque in DialogTypes.h.

namespace pixelroot32::graphics {

namespace {

/// Resolves style.font, falling back to FontManager's default -- the same
/// resolution TextLayout itself performs at every entry point.
inline const Font* resolveFont(const DialogBoxStyle& style) {
    return style.font ? style.font : FontManager::getDefaultFont();
}

/// Height of one line of text at this style. int16_t: at textSize >= 32
/// with an 8px font, font.lineHeight * textSize already exceeds 255.
inline int16_t textLineHeightPx(const Font& font, const DialogBoxStyle& style) {
    return static_cast<int16_t>(font.lineHeight * style.textSize);
}

/// Usable interior width: panel width minus the border on both sides and
/// the padding on both sides. Never negative.
inline int16_t contentWidthPx(const DialogBoxStyle& style) {
    const int16_t inset = static_cast<int16_t>(2 * (style.padding + style.borderWidth));
    return (style.w > inset) ? static_cast<int16_t>(style.w - inset) : int16_t{0};
}

/// True when the line carries any portrait format. The runner never calls
/// this; it is DialogBox's read of DialogLine's three portrait pointers.
/// A format compiled out (Enable2BppSprites/Enable4BppSprites off) reads as
/// absent: Renderer::drawSprite() is a no-op for it, so sizing a text
/// column around an invisible portrait would be the worse inconsistency.
inline bool lineHasAnyPortrait(const gameplay::DialogLine& line) {
    if (line.portrait != nullptr) return true;
    if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
        if (line.portrait2bpp != nullptr) return true;
    }
    if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
        if (line.portrait4bpp != nullptr) return true;
    }
    return false;
}

/// Effective portrait: style switch AND line data AND the fixed size box.
/// False reproduces the portrait-less geometry bit for bit in every
/// consumer below. Declared after portraitWidthPx/portraitHeightPx, which
/// it reads.
inline bool hasPortrait(const gameplay::DialogLine& line, const DialogBoxStyle& style);

/// Portrait pixel width on a line, 0 when the line carries none. Callers
/// gate on hasPortrait() for the portraitsEnabled switch; this helper only
/// reads the winning pointer. Priority matches draw(): 4bpp, then 2bpp,
/// then 1bpp, so measured and drawn widths cannot disagree. The gap
/// between portrait and text column is one style.padding, shared with the
/// text-column computation below so the two cannot disagree.
inline int16_t portraitWidthPx(const gameplay::DialogLine& line) {
    if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
        if (line.portrait4bpp != nullptr) return static_cast<int16_t>(line.portrait4bpp->width);
    }
    if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
        if (line.portrait2bpp != nullptr) return static_cast<int16_t>(line.portrait2bpp->width);
    }
    if (line.portrait != nullptr) return static_cast<int16_t>(line.portrait->width);
    return 0;
}

/// Portrait pixel height, same priority as the width above.
inline int16_t portraitHeightPx(const gameplay::DialogLine& line) {
    if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
        if (line.portrait4bpp != nullptr) return static_cast<int16_t>(line.portrait4bpp->height);
    }
    if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
        if (line.portrait2bpp != nullptr) return static_cast<int16_t>(line.portrait2bpp->height);
    }
    if (line.portrait != nullptr) return static_cast<int16_t>(line.portrait->height);
    return 0;
}

inline bool hasPortrait(const gameplay::DialogLine& line, const DialogBoxStyle& style) {
    if (!style.portraitsEnabled || !lineHasAnyPortrait(line)) return false;
    const int16_t box = static_cast<int16_t>(style.portraitSize);
    return portraitWidthPx(line) <= box && portraitHeightPx(line) <= box;
}

inline int16_t portraitBlockPx(const gameplay::DialogLine& line, const DialogBoxStyle& style) {
    if (!hasPortrait(line, style)) return 0;
    return static_cast<int16_t>(portraitWidthPx(line) + style.padding);
}

/// Body wrap width for a line: the content width narrowed by the portrait
/// block when one is present. Never negative; a zero width wraps to zero
/// body lines (TextLayout cannot hold one glyph), it never underflows.
inline int16_t textWidthPx(const gameplay::DialogLine& line, const DialogBoxStyle& style,
                            int16_t contentW) {
    const int16_t textW = static_cast<int16_t>(contentW - portraitBlockPx(line, style));
    return (textW > 0) ? textW : int16_t{0};
}

/// bodyLineHeightPx/choiceRowHeightPx -- derived here ONCE so computeLayout()
/// and measureHeightPx() cannot disagree on either.
struct RowHeights {
    int16_t bodyLineHeightPx;
    int16_t choiceRowHeightPx;
};

inline RowHeights rowHeightsFor(const Font& font, const DialogBoxStyle& style) {
    const int16_t lineHeightPx = textLineHeightPx(font, style);
    return RowHeights{static_cast<int16_t>(lineHeightPx + style.lineSpacing),
                       static_cast<int16_t>(lineHeightPx + 2 * style.padding)};
}

/// A line's content height (speaker row + up to DialogMaxWrappedLines body
/// rows + up to DialogMaxChoices choice rows), with no DialogRunner needed.
/// Body rows come from `line.text` regardless of `line.kind`, matching
/// computeLayout()'s own wrap() call -- a Choice line's prompt is body text
/// like any other and must contribute rows here too. `contentW` is the FULL
/// panel interior width; the portrait narrowing (textWidthPx) applies
/// inside, so callers pass one width for both. When the line carries a
/// portrait, the content is the TALLER of the portrait block and the text
/// block -- the portrait sits beside the text column, never below it.
inline int16_t lineContentHeightPx(const gameplay::DialogLine& line, const Font& font,
                                    const DialogBoxStyle& style, int16_t contentW,
                                    const RowHeights& rows) {
    const std::string_view text =
        (line.text != nullptr) ? std::string_view(line.text) : std::string_view{};
    int16_t bodyRows = 0;
    bool willPage = false;
    if (!text.empty()) {
        const int16_t textW = textWidthPx(line, style, contentW);
        const uint16_t totalLines = TextLayout::countWrappedLines(text, &font, style.textSize, textW);
        bodyRows = static_cast<int16_t>((totalLines < platforms::config::DialogMaxWrappedLines)
                                             ? totalLines
                                             : platforms::config::DialogMaxWrappedLines);
        willPage = (line.kind != gameplay::LineKind::Choice) &&
                   (totalLines > platforms::config::DialogMaxWrappedLines);
    }

    uint8_t choiceRows = (line.kind == gameplay::LineKind::Choice) ? line.choiceCount : uint8_t{0};
    if (choiceRows > platforms::config::DialogMaxChoices) {
        choiceRows = platforms::config::DialogMaxChoices;
    }

    int16_t height = static_cast<int16_t>(bodyRows * rows.bodyLineHeightPx +
                                           choiceRows * rows.choiceRowHeightPx);
    if (willPage) {
        height = static_cast<int16_t>(height + rows.bodyLineHeightPx);
    }
    if (line.speaker != nullptr) {
        height = static_cast<int16_t>(height + rows.bodyLineHeightPx);
    }
    if (hasPortrait(line, style) && portraitHeightPx(line) > height) {
        height = portraitHeightPx(line);
    }
    return height;
}

}  // namespace

void DialogBox::computeLayout(const DialogBoxStyle&         style,
                               const gameplay::DialogRunner& runner,
                               Layout&                       outLayout) {
    outLayout = Layout{};
    outLayout.panelX = style.x;
    outLayout.panelY = style.y;
    outLayout.panelW = style.w;
    outLayout.panelH = style.h;

    const Font* font = resolveFont(style);
    const gameplay::DialogLine* line = runner.currentLine();
    if (font == nullptr || font->glyphs == nullptr || line == nullptr) {
        // Nothing usable to lay out: an unusable font, or a runner that is
        // Inactive/Finished. Every count above already defaulted to 0/false
        // via the aggregate init, so there is nothing further to zero.
        return;
    }

    const int16_t contentX = static_cast<int16_t>(style.x + style.padding + style.borderWidth);
    const int16_t contentW = contentWidthPx(style);
    const RowHeights rowHeights = rowHeightsFor(*font, style);

    outLayout.bodyLineHeightPx = rowHeights.bodyLineHeightPx;
    outLayout.choiceRowHeightPx = rowHeights.choiceRowHeightPx;

    outLayout.hasSpeaker = (line->speaker != nullptr);

    // Portrait block: top of the content area, always (only the side
    // mirrors). The text column shifts to the other side and narrows by
    // the block; with no portrait every origin below lands exactly where
    // it did before portraits existed.
    outLayout.hasPortrait = hasPortrait(*line, style);
    outLayout.portraitRight =
        outLayout.hasPortrait && ((line->flags & gameplay::kLineFlagPortraitRight) != 0);
    const int16_t block = portraitBlockPx(*line, style);
    const int16_t textX =
        static_cast<int16_t>(contentX + (outLayout.hasPortrait && !outLayout.portraitRight ? block : 0));
    const int16_t textW = textWidthPx(*line, style, contentW);
    if (outLayout.hasPortrait) {
        outLayout.portraitW = portraitWidthPx(*line);
        outLayout.portraitH = portraitHeightPx(*line);
        outLayout.portraitY = static_cast<int16_t>(style.y + style.padding + style.borderWidth);
        outLayout.portraitX = static_cast<int16_t>(
            outLayout.portraitRight ? contentX + contentW - outLayout.portraitW : contentX);
    }
    // Trailing edge of the text column: the detail column right-aligns
    // here rather than at the panel edge, so a right-side portrait never
    // sits under a price.
    outLayout.detailRightX = static_cast<int16_t>(textX + textW);

    int16_t cursorY = static_cast<int16_t>(style.y + style.padding + style.borderWidth);
    if (outLayout.hasSpeaker) {
        outLayout.speakerX = textX;
        outLayout.speakerY = cursorY;
        cursorY = static_cast<int16_t>(cursorY + outLayout.bodyLineHeightPx);
    }

    outLayout.bodyX = textX;
    outLayout.bodyY = cursorY;
    outLayout.page = runner.page();
    outLayout.pageCount = runner.pageCount();

    const std::string_view text =
        (line->text != nullptr) ? std::string_view(line->text) : std::string_view{};
    if (!text.empty()) {
        const uint16_t skipLines =
            static_cast<uint16_t>(outLayout.page) * platforms::config::DialogMaxWrappedLines;
        outLayout.bodyLineCount = TextLayout::wrap(text, font, style.textSize, textW, skipLines,
                                                     outLayout.bodyLines,
                                                     platforms::config::DialogMaxWrappedLines);
    }

    outLayout.choiceX = contentX;
    outLayout.choiceW = contentW;
    // The caret lives in the text column, NOT at choiceX: with a left-side
    // portrait choiceX sits under the face (it stays the full row origin
    // for hit-testing). Without a portrait both origins coincide, so this
    // changes nothing there.
    outLayout.caretX = textX;
    // choiceX/choiceW stay the FULL row rect: choiceRect() reports them for
    // touch hit-testing, and the whole row -- caret gutter AND portrait
    // area included -- must stay tappable. Only the text origin moves.
    if (style.choiceCaret != 0) {
        // The gutter is the caret plus one separating space, measured through
        // the same font path drawText() uses, so it cannot disagree with the
        // glyph actually drawn at choiceX.
        const char caretBuf[2] = {style.choiceCaret, ' '};
        outLayout.caretGutterPx =
            TextLayout::measureWidthPx(std::string_view(caretBuf, 2), font, style.textSize);
    }
    // Option text starts past the caret gutter, measured from the caret
    // column -- identical to choiceX + gutter without a portrait.
    outLayout.choiceTextX = static_cast<int16_t>(outLayout.caretX + outLayout.caretGutterPx);
    outLayout.choiceY =
        static_cast<int16_t>(cursorY + outLayout.bodyLineCount * outLayout.bodyLineHeightPx);
    outLayout.choiceCount = runner.choiceCount();
}

bool DialogBox::needsRedraw(const gameplay::DialogRunner& runner) const {
    return runner.revision() != lastRevision_;
}

bool DialogBox::choiceRect(const gameplay::DialogRunner& runner, uint8_t index, int16_t& outXPx,
                            int16_t& outYPx, int16_t& outWPx, int16_t& outHPx) const {
    if (runner.state() != gameplay::DialogState::ShowingChoices) {
        return false;
    }
    if (index >= runner.choiceCount()) {
        return false;
    }

    // Same producer draw() uses -- see computeLayout()'s Doxygen for why
    // this is the whole point of this class.
    Layout layout{};
    computeLayout(style_, runner, layout);

    outXPx = layout.choiceX;
    outYPx = static_cast<int16_t>(layout.choiceY + index * layout.choiceRowHeightPx);
    outWPx = layout.choiceW;
    outHPx = layout.choiceRowHeightPx;
    return true;
}

uint8_t DialogBox::pageCountFor(const gameplay::DialogLine& line, const DialogBoxStyle& style) {
    if (line.kind == gameplay::LineKind::Choice) {
        // DialogRunner ignores Advance while ShowingChoices, so a second
        // page could never be reached.
        return 1;
    }

    const Font* font = resolveFont(style);
    if (font == nullptr || font->glyphs == nullptr) {
        return 1;
    }

    const std::string_view text =
        (line.text != nullptr) ? std::string_view(line.text) : std::string_view{};
    if (text.empty()) {
        return 1;
    }

    const int16_t contentW = contentWidthPx(style);
    const int16_t textW = textWidthPx(line, style, contentW);
    const uint16_t totalLines =
        TextLayout::countWrappedLines(text, font, style.textSize, textW);
    if (totalLines == 0) {
        return 1;
    }

    const uint16_t perPage =
        (platforms::config::DialogMaxWrappedLines > 0) ? platforms::config::DialogMaxWrappedLines : 1;
    uint16_t pages = static_cast<uint16_t>((totalLines + perPage - 1) / perPage);
    if (pages == 0) {
        pages = 1;
    }
    if (pages > 0xFF) {
        pages = 0xFF;
    }
    return static_cast<uint8_t>(pages);
}

int16_t DialogBox::measureHeightPx(const gameplay::DialogScript& script,
                                    const DialogBoxStyle&         style) {
    const Font* font = resolveFont(style);
    if (font == nullptr || font->glyphs == nullptr || script.lines == nullptr ||
        script.lineCount == 0) {
        return 0;
    }

    const int16_t contentW = contentWidthPx(style);
    const RowHeights rowHeights = rowHeightsFor(*font, style);
    const int16_t frame = static_cast<int16_t>(2 * (style.padding + style.borderWidth));

    int16_t tallest = 0;
    for (uint16_t i = 0; i < script.lineCount; ++i) {
        const int16_t total = static_cast<int16_t>(
            lineContentHeightPx(script.lines[i], *font, style, contentW, rowHeights) + frame);
        if (total > tallest) {
            tallest = total;
        }
    }

    return tallest;
}

}  // namespace pixelroot32::graphics

#endif  // PIXELROOT32_ENABLE_DIALOG
