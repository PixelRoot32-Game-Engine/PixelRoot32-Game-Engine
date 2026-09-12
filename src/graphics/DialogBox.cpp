/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/DialogBox.h"

#if PIXELROOT32_ENABLE_DIALOG

#include "graphics/FontManager.h"

namespace pixelroot32::graphics {

namespace {

/// Resolves style.font, falling back to FontManager's default -- the same
/// resolution TextLayout itself performs at every entry point.
inline const Font* resolveFont(const DialogBoxStyle& style) {
    return style.font ? style.font : FontManager::getDefaultFont();
}

/// Height of one line of text at this style: the font's own line height
/// (already includes intra-glyph vertical spacing) times the style's size
/// multiplier.
inline uint8_t textLineHeightPx(const Font& font, const DialogBoxStyle& style) {
    return static_cast<uint8_t>(font.lineHeight * style.textSize);
}

/// Usable interior width: panel width minus the border on both sides and
/// the padding on both sides. Never negative.
inline int16_t contentWidthPx(const DialogBoxStyle& style) {
    const int16_t inset = static_cast<int16_t>(2 * (style.padding + style.borderWidth));
    return (style.w > inset) ? static_cast<int16_t>(style.w - inset) : int16_t{0};
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
    const uint8_t lineHeightPx = textLineHeightPx(*font, style);

    outLayout.bodyLineHeightPx = static_cast<uint8_t>(lineHeightPx + style.lineSpacing);
    // Row height reserves style.padding above AND below the row's text --
    // draw()'s choice loop and this figure must always agree; see the
    // anti-drift note beside draw()'s choice text call in DialogBox.h.
    outLayout.choiceRowHeightPx = static_cast<uint8_t>(lineHeightPx + 2 * style.padding);

    outLayout.hasSpeaker = (line->speaker != nullptr);

    int16_t cursorY = static_cast<int16_t>(style.y + style.padding + style.borderWidth);
    if (outLayout.hasSpeaker) {
        outLayout.speakerX = contentX;
        outLayout.speakerY = cursorY;
        cursorY = static_cast<int16_t>(cursorY + outLayout.bodyLineHeightPx);
    }

    outLayout.bodyX = contentX;
    outLayout.bodyY = cursorY;
    outLayout.page = runner.page();
    outLayout.pageCount = runner.pageCount();

    const std::string_view text =
        (line->text != nullptr) ? std::string_view(line->text) : std::string_view{};
    if (!text.empty()) {
        const uint16_t skipLines =
            static_cast<uint16_t>(outLayout.page) * platforms::config::DialogMaxWrappedLines;
        outLayout.bodyLineCount = TextLayout::wrap(text, font, style.textSize, contentW, skipLines,
                                                     outLayout.bodyLines,
                                                     platforms::config::DialogMaxWrappedLines);
    }

    outLayout.choiceX = contentX;
    outLayout.choiceW = contentW;
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
    const uint16_t totalLines =
        TextLayout::countWrappedLines(text, font, style.textSize, contentW);
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
    const uint8_t lineHeightPx = textLineHeightPx(*font, style);
    const uint8_t bodyLineHeightPx = static_cast<uint8_t>(lineHeightPx + style.lineSpacing);
    const uint8_t choiceRowHeightPx = static_cast<uint8_t>(lineHeightPx + 2 * style.padding);
    const int16_t frame = static_cast<int16_t>(2 * (style.padding + style.borderWidth));

    int16_t tallest = 0;
    for (uint16_t i = 0; i < script.lineCount; ++i) {
        const gameplay::DialogLine& line = script.lines[i];
        int16_t contentHeight = 0;

        if (line.kind == gameplay::LineKind::Choice) {
            uint8_t rows = line.choiceCount;
            if (rows > platforms::config::DialogMaxChoices) {
                rows = platforms::config::DialogMaxChoices;
            }
            contentHeight = static_cast<int16_t>(rows * choiceRowHeightPx);
        } else if (line.kind == gameplay::LineKind::Text) {
            const std::string_view text =
                (line.text != nullptr) ? std::string_view(line.text) : std::string_view{};
            if (!text.empty()) {
                const uint16_t totalLines =
                    TextLayout::countWrappedLines(text, font, style.textSize, contentW);
                const uint16_t rows = (totalLines < platforms::config::DialogMaxWrappedLines)
                                           ? totalLines
                                           : platforms::config::DialogMaxWrappedLines;
                contentHeight = static_cast<int16_t>(rows * bodyLineHeightPx);
            }
        }
        // LineKind::End shows nothing; contentHeight stays 0.

        if (line.speaker != nullptr) {
            contentHeight = static_cast<int16_t>(contentHeight + bodyLineHeightPx);
        }

        const int16_t total = static_cast<int16_t>(contentHeight + frame);
        if (total > tallest) {
            tallest = total;
        }
    }

    return tallest;
}

}  // namespace pixelroot32::graphics

#endif  // PIXELROOT32_ENABLE_DIALOG
