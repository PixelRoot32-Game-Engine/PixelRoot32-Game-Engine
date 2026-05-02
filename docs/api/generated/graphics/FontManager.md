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

### `static uint8_t getGlyphIndex(char c, const Font* font = nullptr)`

**Description:**

Gets the glyph index for a character code.

**Parameters:**

- `c`: The character code.
- `font`: Pointer to the font to use. If nullptr, uses the default font.

**Returns:** Glyph index (0-based) if character is in font range, or 255 if not found.

### `static bool isCharSupported(char c, const Font* font = nullptr)`

**Description:**

Checks if a character is supported by a font.

**Parameters:**

- `c`: The character code.
- `font`: Pointer to the font to check. If nullptr, uses the default font.

**Returns:** true if the character is in the font's range, false otherwise.
