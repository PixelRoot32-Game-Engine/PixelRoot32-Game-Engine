# ScreenBounds

<Badge type="info" text="Struct" />

**Source:** `ProjectedMapBounds.h`

## Description

Half-open screen-space bounding box accumulated across one or more
       `expandProjectedMapBounds` calls.

`right` and `bottom` are one PAST the last covered pixel -- the same
convention a tile blit uses (`[drawX, drawX + width)`), so
`right - left` and `bottom - top` are exact pixel widths/heights with no
off-by-one at the edge.

`valid` distinguishes "never seeded" from the representable real box
`{0, 0, 0, 0}` (e.g. a single-cell map at the origin with a zero-reach
tileset). `false` means the next `expandProjectedMapBounds` call seeds the
box directly from its own extent; `true` means it widens the existing box
to the union. A default-constructed `ScreenBounds{}` is always the correct
value to seed a fresh accumulation with.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `left` | `int` | Leftmost covered screen X, in pixels. |
| `top` | `int` | Topmost covered screen Y, in pixels. |
| `right` | `int` | One past the rightmost covered screen X, in pixels. |
| `bottom` | `int` | One past the bottommost covered screen Y, in pixels. |
| `valid` | `bool` | `false` until the first successful expand call. |
