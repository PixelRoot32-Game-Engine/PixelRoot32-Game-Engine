# DialogBoxStyle

<Badge type="info" text="Struct" />

**Source:** `DialogBox.h`

## Description

Every visual and layout knob DialogBox needs to draw a panel.

Pointer first, tags last (the same field-packing convention
DialogRunner and DialogTypes follow, copied from StateMachine): `font`
leads, the 15 scalar/enum fields follow.

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
to 0. The row rect choiceRect() reports is unaffected and still spans the
gutter, so the whole row stays tappable.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `Font` | `const` | nullptr uses FontManager's default. |
| `panel` | `Color` | Palette-resolved at draw time; see the colour note above. |
| `border` | `Color` | Palette-resolved at draw time. |
| `ink` | `Color` | Speaker, body and unselected choices. Palette-resolved. |
| `inkDim` | `Color` | Next-page cue. Palette-resolved. |
| `inkSelected` | `Color` | Selected choice. Palette-resolved; a zeroed slot hides it. |
| `padding` | `uint8_t` | Inside the border, and above and below each choice row. |
| `lineSpacing` | `uint8_t` | Extra px between wrapped body lines. |
| `choiceCaret` | `char` | Marks the selected choice. 0 disables the caret and its gutter. |
| `fixedPosition` | `bool` | true: setOffsetBypass(true) while drawing, ignoring the camera. |
