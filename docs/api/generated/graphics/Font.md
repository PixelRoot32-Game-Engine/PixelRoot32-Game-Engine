# Font

<Badge type="info" text="Struct" />

**Source:** `Font.h`

## Description

Descriptor for a bitmap font using 1bpp sprites.

A Font contains an array of Sprite structures, one for each character
in the font's character set. Each glyph is rendered as a 1bpp sprite,
allowing consistent rendering across platforms.

The font uses fixed-width glyphs for simplicity and performance.
All glyphs share the same width and height, with spacing between
characters controlled by the `spacing` field.

Font data should be stored in flash memory (const/constexpr)
      to minimize RAM usage on embedded systems.
Include "Renderer.h" when using Font in implementation files
      to get the full Sprite definition.

::: tip
Font data should be stored in flash memory (const/constexpr)
      to minimize RAM usage on embedded systems.
:::

::: tip
Include "Renderer.h" when using Font in implementation files
      to get the full Sprite definition.
:::

## Properties

| Name | Type | Description |
|------|------|-------------|
| `Sprite` | `const` | Array of sprites, one per character (indexed by character code - firstChar) |
| `firstChar` | `uint8_t` | First character code in the font (e.g., 32 for space ' ') |
| `lastChar` | `uint8_t` | Last character code in the font (e.g., 126 for '~') |
| `glyphWidth` | `uint8_t` | Fixed width of each glyph in pixels |
| `glyphHeight` | `uint8_t` | Fixed height of each glyph in pixels |
| `spacing` | `uint8_t` | Horizontal spacing between characters in pixels |
| `lineHeight` | `uint8_t` | Total line height including vertical spacing (glyphHeight + vertical spacing) |
| `extFirstChar` | `uint8_t` | First supplement codepoint (e.g., 0xA0) |
| `extLastChar` | `uint8_t` | Last supplement codepoint (e.g., 0xFF) |
| `extYOffset` | `int8_t` | Vertical shift applied to supplement glyphs |
