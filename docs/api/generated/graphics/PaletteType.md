# PaletteType

<Badge type="info" text="Enum" />

**Source:** `Color.h`

## Description

Selects which built-in 16-color palette the renderer resolves against.

PR32 is the default and is the palette the Color enumerators are indexed for.
The others are retro presets; the same Color value resolves to a different
RGB565 output under each one.
