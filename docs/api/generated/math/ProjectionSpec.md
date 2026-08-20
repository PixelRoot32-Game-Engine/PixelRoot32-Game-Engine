# ProjectionSpec

<Badge type="info" text="Struct" />

**Source:** `Projection.h`

## Description

Plain six-`int` aggregate: the screen anchor of cell (0, 0) plus the
       two screen-space axis vectors of the cell grid.

The two axis vectors form the columns of a 2x2 integer matrix. No member
functions, no derived fields, no per-instance runtime state — a `constexpr
ProjectionSpec` costs zero SRAM. The determinant is deliberately NOT a
field: see projectionDet().

Defaults form the identity basis, so a default-constructed spec maps every
cell to itself.

| Layout | Tile | Value | det |
|---|---|---|---|
| Orthogonal | 16x16 | `{0, 0, 16, 0, 0, 16}` | 256 |
| Isometric 2:1 | 32x16 | `{0, 0, 16, 8, -16, 8}` | 256 |
| Isometric 1:1 | 32x32 | `{0, 0, 16, 16, -16, 16}` | 512 |
| Oblique | 16x16 | `{0, 0, 16, 0, 8, 16}` | 256 |

## Properties

| Name | Type | Description |
|------|------|-------------|
| `originX` | `int` | Screen X of cell (0, 0)'s anchor, in pixels. |
| `originY` | `int` | Screen Y of cell (0, 0)'s anchor, in pixels. |
| `axisXx` | `int` | Screen X delta per +1 cellX. |
| `axisXy` | `int` | Screen Y delta per +1 cellX. |
| `axisYx` | `int` | Screen X delta per +1 cellY. |
| `axisYy` | `int` | Screen Y delta per +1 cellY. |
