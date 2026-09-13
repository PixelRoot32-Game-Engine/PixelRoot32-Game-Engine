# DialogBox

<Badge type="info" text="Class" />

**Source:** `DialogBox.h`

## Description

Optional default dialog panel, driven by a DialogRunner.

Deliberately NOT a UIElement: both demos build with UI_SYSTEM=0, and
UILabel::text / UILayout::elements are the engine's only two heap sites.
With UI_SYSTEM=1 there are two panel paths -- DialogBox is the dialog
path, UIPanel is the structured-HUD path. Zero heap allocation in every
method.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `speakerX` | `int16_t` | Valid when hasSpeaker. |
| `bodyX` | `int16_t` | Top-left of body line 0. |
| `choiceX` | `int16_t` | Top-left of choice row 0. |
| `choiceW` | `int16_t` | Row width (panel inner width). |
| `bodyLineCount` | `uint8_t` | Rows valid in bodyLines. |
| `choiceCount` | `uint8_t` | Already clamped to DialogMaxChoices. |
| `pageCount` | `uint8_t` | Total pages of the current line's text. |
| `page` | `uint8_t` | Zero-based current page index. |

## Methods

### `void setStyle(const DialogBoxStyle& style)`

**Description:**

Sets the visual/layout style used by every subsequent call.

**Parameters:**

- `style`: The style to apply.

### `const DialogBoxStyle& style() const`

**Description:**

The style currently in effect.

**Returns:** A const reference to the active DialogBoxStyle.

### `static void computeLayout(const DialogBoxStyle&         style, const gameplay::DialogRunner& runner, Layout&                       outLayout)`

**Description:**

THE single geometry producer. Pure, static, no Renderer, no mutation.

**Parameters:**

- `style`: The style to lay out against.
- `runner`: The runner to read the current line, page and choice state from.
- `outLayout`: Written with every coordinate draw()/choiceRect() need.

Wraps the current line's text for `runner.page()` via
TextLayout::wrap(..., skipLines = page * DialogMaxWrappedLines, ...).
When the runner has no current line (Inactive or Finished), every
count in `outLayout` is zeroed and no wrap runs -- there is nothing
to lay out.

### `void draw(RendererT& renderer, gameplay::DialogRunner& runner)`

**Description:**

Draws the panel, border, speaker, body page and option list.

**Parameters:**

- `renderer`: The renderer to issue draw calls against.
- `runner`: The runner to present. Read for layout; setPageCount()
       is the only mutation this method makes.

Templated on the renderer type because Renderer declares NO virtual
methods: a Renderer& parameter would statically dispatch past
MockRenderer and make this class unassertable. Zero vtable, zero
heap. Takes the runner NON-const solely to call
runner.setPageCount() with the count only this class can compute
(pageCountFor()); it never changes state and never emits an event.
A no-op when the runner has no current line.

### `bool needsRedraw(const gameplay::DialogRunner& runner) const`

**Description:**

Whether the runner changed since the last draw().

**Parameters:**

- `runner`: The runner to check.

**Returns:** true when `runner.revision()` differs from the value observed
        by the last draw() call.

An optimisation hint only -- draw() is always safe to call
unconditionally. Compares revision() by inequality; never orders it.

### `bool choiceRect(const gameplay::DialogRunner& runner, uint8_t                       index, int16_t&                      outXPx, int16_t&                      outYPx, int16_t&                      outWPx, int16_t&                      outHPx) const`

**Description:**

Rect of option `index`, for touch hit-testing.

**Parameters:**

- `runner`: The runner to read choice state from.
- `index`: Zero-based option index.
- `outXPx`: Written with the row's top-left X.
- `outYPx`: Written with the row's top-left Y.
- `outWPx`: Written with the row's width.
- `outHPx`: Written with the row's height.

**Returns:** false when `index >= choiceCount()` or the runner is not
        showing choices; outputs are untouched. true otherwise.

Derived from the SAME computeLayout() call draw() uses, so hit-test
and draw geometry cannot drift apart.

### `static uint8_t pageCountFor(const gameplay::DialogLine& line, const DialogBoxStyle&       style)`

**Description:**

Pages the given line needs at this style.

**Parameters:**

- `line`: The line to measure.
- `style`: The style to wrap against.

**Returns:** The number of pages `line`'s body text needs at `style`'s
        width, at least 1; always exactly 1 for a `LineKind::Choice`
        line, since DialogRunner ignores Advance while ShowingChoices.
        Explicit path for games that draw the panel themselves.

### `static int16_t measureHeightPx(const gameplay::DialogScript& script, const DialogBoxStyle&         style)`

**Description:**

Minimum panel height in px for the tallest line in `script`.

**Parameters:**

- `script`: The script to measure every line of.
- `style`: The style to measure against.

**Returns:** The minimum panel height, in pixels, that fits the tallest
        single page any line in `script` can produce.
