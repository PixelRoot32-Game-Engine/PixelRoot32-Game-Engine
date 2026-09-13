# TextLayout

<Badge type="info" text="Class" />

**Source:** `TextLayout.h`

## Description

Wrap/measure over FontManager. Stateless, allocation-free, no Renderer.

UNIT CONTRACT -- measure and index in GLYPHS, slice in BYTES.
Every std::string_view is a BYTE range. Every *Px value is pixels. Every
*glyphCount is decoded cells, one per FontManager::nextGlyph step, never
one per byte. Multi-byte UTF-8 therefore has byteLength > glyphCount.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `slice` | `std::string_view` | BYTES. Sub-view of the input; never owns. |
| `widthPx` | `int16_t` | Pixel width of `slice` at the wrap's `size`. |

## Methods

### `static uint8_t wrap(std::string_view text, const Font*      font, uint8_t          size, int16_t          maxWidthPx, uint16_t         skipLines, WrappedLine*     outLines, uint8_t          maxOutLines)`

**Description:**

Wraps `text` into `outLines`, skipping the first `skipLines` wrapped lines.

**Parameters:**

- `text`: The full BYTE range to wrap; never mutated.
- `font`: Font to measure with; nullptr resolves FontManager's default.
- `size`: Glyph scale factor applied to `font`'s cell metrics.
- `maxWidthPx`: Maximum pixel width a wrapped line may occupy.
- `skipLines`: Number of wrapped lines to skip before writing to `outLines`.
- `outLines`: Caller-owned buffer to write wrapped lines into.
- `maxOutLines`: Capacity of `outLines`; never exceeded.

**Returns:** linesWritten: rows written to `outLines` (<= maxOutLines).
        0 when the font is unusable, text is empty or whitespace-only,
        maxWidthPx cannot hold one glyph, skipLines is past the end,
        outLines is null, or maxOutLines is 0.

### `static uint16_t countWrappedLines(std::string_view text, const Font*      font, uint8_t          size, int16_t          maxWidthPx)`

**Description:**

Total wrapped lines `text` needs at `maxWidthPx`, without writing any output.

**Parameters:**

- `text`: The full BYTE range to measure.
- `font`: Font to measure with; nullptr resolves FontManager's default.
- `size`: Glyph scale factor applied to `font`'s cell metrics.
- `maxWidthPx`: Maximum pixel width a wrapped line may occupy.

**Returns:** Total number of lines `text` would wrap to.

### `static int16_t measureWidthPx(std::string_view slice, const Font*      font, uint8_t          size)`

**Description:**

Pixel width of a BYTE slice.

**Parameters:**

- `slice`: The BYTE range to measure.
- `font`: Font to measure with; nullptr resolves FontManager's default.
- `size`: Glyph scale factor applied to `font`'s cell metrics.

**Returns:** Pixel width of `slice` at `size`.

### `static uint16_t countGlyphs(std::string_view slice, const Font*      font)`

**Description:**

Decoded cells in a BYTE slice, one per FontManager::nextGlyph step.

**Parameters:**

- `slice`: The BYTE range to count.
- `font`: Font whose decode rules resolve each glyph; nullptr resolves FontManager's default.

**Returns:** Number of decoded glyphs in `slice`.

### `static size_t byteOffsetForGlyphs(std::string_view slice, const Font*      font, size_t           fromByteOffset, uint16_t         glyphCount)`

**Description:**

Byte offset of the glyph boundary `glyphCount` glyphs after `fromByteOffset`.

**Parameters:**

- `slice`: The BYTE range to walk.
- `font`: Font whose decode rules resolve each glyph; nullptr resolves FontManager's default.
- `fromByteOffset`: Byte offset to start counting glyphs from.
- `glyphCount`: Number of glyphs to advance past `fromByteOffset`.

**Returns:** Byte offset of the resulting glyph boundary, clamped to slice.size().
