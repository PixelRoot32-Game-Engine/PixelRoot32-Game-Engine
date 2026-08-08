# GridSpec

<Badge type="info" text="Struct" />

**Source:** `GridSpace.h`

## Description

Plain six-`int` aggregate describing a grid's origin, per-axis cell
       size, and extent (columns/rows). No member functions, no
       per-instance runtime state beyond these fields — a `constexpr
       GridSpec` costs zero SRAM.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `originX` | `int` | World-space X of cell (0, 0)'s top-left corner, in pixels. |
| `originY` | `int` | World-space Y of cell (0, 0)'s top-left corner, in pixels. |
| `cellWidth` | `int` | Cell width in pixels. MUST be >= 1 (see gridSpecIsValid()). |
| `cellHeight` | `int` | Cell height in pixels. MUST be >= 1 (see gridSpecIsValid()). |
| `cols` | `int` | Number of columns, for containsCell() bounds-checking. |
| `rows` | `int` | Number of rows, for containsCell() bounds-checking. |

## Methods

### `constexpr bool gridSpecIsValid(const GridSpec& spec)`

**Description:**

Validates a GridSpec's invariants at compile time or runtime.

### `constexpr int gridFloorDiv(int value, int divisor)`

**Description:**

Floor division: `floor(value / divisor)`, for `divisor >= 1`.

### `constexpr bool containsCell(int cellX, int cellY, const GridSpec& spec)`

**Description:**

Returns true iff (cellX, cellY) is within [0, cols) x [0, rows).
