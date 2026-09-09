# CellRange

<Badge type="info" text="Struct" />

**Source:** `Projection.h`

## Description

Half-open cell-space window `[startCol, endCol) x [startRow, endRow)`
       covering a screen rectangle, under a given ProjectionSpec.

Field names are deliberately identical to
`TilemapDirtyTrackingHelper`'s (`include/graphics/Renderer.h`), so the
eventual renderer wiring is a rename-free assignment rather than a mapping.
A default-constructed `CellRange` is empty (`start == end == 0` on both
axes), matching "nothing to draw".
