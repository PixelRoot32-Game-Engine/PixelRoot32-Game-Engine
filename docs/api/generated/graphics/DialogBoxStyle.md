# DialogBoxStyle

<Badge type="info" text="Struct" />

**Source:** `DialogBox.h`

## Description

Every visual and layout knob DialogBox needs to draw a panel.

Pointer first, tags last (the same field-packing convention
DialogRunner and DialogTypes follow, copied from StateMachine): `font`
leads, the 20 scalar/enum fields follow.

Colours: `panel`, `border`, `ink`, `inkDim` and `inkSelected` are Color
names, not RGB565 values. DialogBox::draw() hands them to the renderer,
which resolves each one through a palette at draw time, exactly as for
any primitive or text. In single-palette mode that is the palette set by
setPalette() or setCustomPalette(). In dual-palette mode it is the sprite
palette, unless the renderer's render context is
PaletteContext::Background during the draw; Scene::draw() sets that
context only while drawing entities on render layer 0 and clears it
afterwards. A palette that stores 0x0000 in one of these slots draws that
element black: with the defaults, a zeroed Yellow slot makes the selected
choice invisible on the Black panel. Set these fields to slots the game's
palette defines, or keep the default slots (Black, White, Gray, Yellow)
populated in that palette. This is not hypothetical -- legend_of_clone
shipped exactly that palette. `choiceCaret` is the answer: DialogBox marks
the selected row with that glyph as well as with `inkSelected`, and draws
the caret in `ink`, NOT in `inkSelected`, on purpose. Selection therefore
has two independent signals, and the caret's own slot is the one the body
text already proves is visible; a caret drawn in `inkSelected` would
disappear in the very case it exists to cover. Setting `choiceCaret` to 0
disables the caret and gives back the pre-caret geometry exactly, at the
cost of returning selection to a single point of failure.

Sizing: `padding` is applied once inside the border on every side, and
again above and below the text of every choice row. The height one line
needs, with L = font lineHeight x textSize, is:


```cpp
bodyRowPx   = L + lineSpacing
choiceRowPx = L + 2 * padding
height      = 2 * (borderWidth + padding)
            + (speaker != nullptr ? bodyRowPx : 0)
            + bodyRows * bodyRowPx
            + (willPage ? bodyRowPx : 0)
            + choiceRows * choiceRowPx
```


`bodyRows` is the line's wrapped text row count, capped at
config::DialogMaxWrappedLines, wrapping at w - 2 * (borderWidth +
padding). `willPage` holds for a non-Choice line whose text wraps past
that cap, and reserves one row for the next-page cue. `choiceRows` is a
Choice line's declared choiceCount capped at config::DialogMaxChoices,
and 0 for any other kind. Padding therefore adds
2 * padding * (1 + choiceRows) px, and it also narrows the wrap width,
which can add body rows. DialogBox::measureHeightPx() returns this height
for the tallest line of a script; compare it with the area the box must
fit.

A non-zero `choiceCaret` also reserves a gutter to the left of every
option, as wide as the two-character slice {caret, ' '} at this font and
textSize (Layout::caretGutterPx). It costs no height, but it narrows the
room an option's text has: choice text is NOT wrapped, so a label that no
longer fits simply runs past the panel's inner edge. Keep the longest
option shorter than contentWidth - caretGutterPx, or set `choiceCaret`
to 0. The caret itself draws at Layout::caretX -- the text-column origin,
past a left-side portrait -- never at the full-row choiceX it would
overlap the face from. The row rect choiceRect() reports is unaffected
and still spans the gutter AND the portrait area, so the whole row
stays tappable.

Multi-column rows: DialogChoice::detail, when non-null, draws a second
column RIGHT-aligned against the text column's trailing edge
(Layout::detailRightX -- the panel's inner edge, unless a right-side
portrait narrows the column), on the same row baseline, in `inkDetail`
(or `inkDetailSelected` when that row is selected). The label keeps its
left-aligned origin exactly, so a null detail reproduces the
single-column geometry bit for bit. Neither column wraps and drawText()
does not clip: the game keeps `label + gap + detail` within the text
column width minus caretGutterPx. On overlap the detail wins visually
(drawn second, same row); there is no truncation or ellipsis, by
design -- one drawText per column, zero heap.

Speaker portraits: DialogLine::portrait / portrait2bpp / portrait4bpp,
when any one is non-null AND portraitsEnabled is true AND the sprite
fits the `portraitSize` box, draws that flash sprite 1:1 at the TOP of
the content area -- top-LEFT by default, top-right with
kLineFlagPortraitRight -- so two speakers can face each other across
alternating lines. A 1bpp portrait draws in `portraitInk`;
a 2bpp/4bpp portrait draws through the line's `portraitPaletteSlot`,
allowing higher-detail faces than 1bpp allows. A sprite larger than
the box in either dimension is ignored (portrait-less geometry, no
draw): without a scaler or clip rect, drawing it would overflow the
panel, so the engine refuses instead. Author every face at exactly
the box size; a smaller one still draws 1:1 and only shifts the text
by its own width. The whole text
column (speaker label, body, choices) shifts to the other side and the
body wrap width narrows by portrait width + one padding (the gap); no
portrait -- all three pointers null, portraitsEnabled false, or
over-box -- reproduces the portrait-less geometry bit for bit.
Portraits never scale and drawText()/drawSprite() never clip, so the
game authors portraits to fit: measureHeightPx() takes the taller of
the portrait and the text block per line. choiceRect() still reports
the FULL content-width row, portrait area included -- the whole row
stays tappable, the same precedent as the caret gutter.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `font` | `const Font*` | nullptr uses FontManager's default. |
| `panel` | `Color` | Palette-resolved at draw time; see the colour note above. |
| `border` | `Color` | Palette-resolved at draw time. |
| `ink` | `Color` | Speaker, body and unselected choices. Palette-resolved. |
| `inkDim` | `Color` | Next-page cue. Palette-resolved. |
| `inkSelected` | `Color` | Selected choice. Palette-resolved; a zeroed slot hides it. |
| `inkDetail` | `Color` | Second-column (DialogChoice::detail) text. Palette-resolved. |
| `inkDetailSelected` | `Color` | Selected row's second column. Palette-resolved. |
| `portraitInk` | `Color` | Speaker portrait tint (1bpp Sprite). Palette-resolved. |
| `padding` | `uint8_t` | Inside the border, and above and below each choice row. |
| `lineSpacing` | `uint8_t` | Extra px between wrapped body lines. |
| `choiceCaret` | `char` | Marks the selected choice. 0 disables the caret and its gutter. |
| `fixedPosition` | `bool` | true: setOffsetBypass(true) while drawing, ignoring the camera. |
| `portraitsEnabled` | `bool` | Master switch for speaker portraits. False draws every line as if it carried no portrait, with portrait-less geometry bit for bit -- dialogs work without portraits without touching scripts. |
| `portraitSize` | `DialogPortraitSize` | Fixed portrait box every face must fit in. A sprite larger than the box in either dimension is ignored (same geometry as no portrait); author faces at exactly this size. |
