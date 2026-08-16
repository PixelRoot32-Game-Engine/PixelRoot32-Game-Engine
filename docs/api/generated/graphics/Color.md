# Color

<Badge type="info" text="Enum" />

**Source:** `Color.h`

## Description

Named color indices into the active 16-entry palette.

Enumerator values are the PR32 palette indices 0-15, so a Color is a palette
slot, not an RGB value: the same enumerator resolves differently under each
PaletteType.

Legacy names are aliases, not extra colors. `LightBlue`, `Teal`,
      `Olive`, `Gold` and `Brown` collapse onto the nearest of the 16 slots,
      and every gray name (`MidGray`, `LightGray`, `DarkGray`, `Silver`)
      resolves to the single `Gray` slot. Two aliased names compare equal.

`Transparent` (255) is a sentinel, not a color. The renderer and
         blitter must intercept it; it must never be resolved against a
         palette, and passing it to a drawing primitive is a no-op.

::: warning
`Transparent` (255) is a sentinel, not a color. The renderer and
         blitter must intercept it; it must never be resolved against a
         palette, and passing it to a drawing primitive is a no-op.
:::

::: tip
Legacy names are aliases, not extra colors. `LightBlue`, `Teal`,
      `Olive`, `Gold` and `Brown` collapse onto the nearest of the 16 slots,
      and every gray name (`MidGray`, `LightGray`, `DarkGray`, `Silver`)
      resolves to the single `Gray` slot. Two aliased names compare equal.
:::
