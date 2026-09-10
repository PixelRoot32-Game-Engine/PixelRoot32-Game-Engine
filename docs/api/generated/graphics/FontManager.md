# FontManager

<Badge type="info" text="Class" />

**Source:** `FontManager.h`

## Description

Static utility class for managing bitmap fonts.

FontManager provides functions to:
- Set and retrieve the default font
- Calculate text width for layout purposes
- Convert character codes to glyph indices

The default font is used when no font is explicitly specified
in rendering calls.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `index` | `uint16_t` | Index into glyphs[] or extGlyphs[]; kNoGlyph if undrawable |
| `bytes` | `uint8_t` | Bytes consumed from `text` at `pos` (1-4) |
| `extended` | `bool` | true when `index` addresses extGlyphs (apply extYOffset) |

## Methods

### `static void setDefaultFont(const Font* font)`

**Description:**

Sets the default font used for text rendering.

**Parameters:**

- `font`: Pointer to a Font structure. Must remain valid for the lifetime of its use.
            Pass nullptr to clear the default font (not recommended).

### `static const Font* getDefaultFont()`

**Description:**

Gets the current default font.

**Returns:** Pointer to the default font, or nullptr if no font is set.

### `static int16_t textWidth(const Font* font, const char* text, uint8_t size = 1)`

**Description:**

Calculates the width in pixels of a text string when rendered.

**Parameters:**

- `font`: Pointer to the font to use. If nullptr, uses the default font.
- `text`: The text string to measure.
- `size`: Text size multiplier (1 = normal, 2 = double size, etc.).

**Returns:** Width in pixels, or 0 if font is invalid or text is empty.

### `static int16_t textWidth(const Font* font, std::string_view text, uint8_t size = 1)`

**Description:**

Calculates the width in pixels of a text string when rendered.

**Parameters:**

- `font`: Pointer to the font to use. If nullptr, uses the default font.
- `text`: The text string to measure.
- `size`: Text size multiplier (1 = normal, 2 = double size, etc.).

**Returns:** Width in pixels, or 0 if font is invalid or text is empty.

### `static uint16_t getGlyphIndex(char c, const Font* font = nullptr)`

**Description:**

Gets the glyph index for a character code.

**Parameters:**

- `c`: The character code.
- `font`: Pointer to the font to use. If nullptr, uses the default font.

**Returns:** Glyph index (0-based) if character is in font range, or `kNoGlyph` if not found.

### `static bool isCharSupported(char c, const Font* font = nullptr)`

**Description:**

Checks if a character is supported by a font.

**Parameters:**

- `c`: The character code.
- `font`: Pointer to the font to check. If nullptr, uses the default font.

**Returns:** true if the character is in the font's range, false otherwise.

::: tip
Evaluates a single byte only. Does not decode multi-byte UTF-8
      sequences -- a lead byte passed alone is evaluated as that byte
      value, not as the start of a sequence. Use isCodepointSupported()
      or nextGlyph() for multi-byte-aware queries.
:::

### `static bool isCodepointSupported(uint16_t codepoint, const Font* font = nullptr)`

**Description:**

Checks if a decoded codepoint is supported by a font.

**Parameters:**

- `codepoint`: The decoded codepoint (base ASCII or Latin-1 supplement).
- `font`: Pointer to the font to check. If nullptr, uses the default font.

**Returns:** true if the codepoint is in the font's base or supplement range.

### `static GlyphStep nextGlyph(std::string_view text, size_t pos, const Font* font)`

**Description:**

Decodes one glyph position from `text` starting at `pos`.

**Parameters:**

- `text`: The full string being decoded.
- `pos`: Byte offset to start decoding at.
- `font`: Font whose base/supplement ranges resolve the codepoint.

**Returns:** The decoded step; `bytes` is always >= 1.
