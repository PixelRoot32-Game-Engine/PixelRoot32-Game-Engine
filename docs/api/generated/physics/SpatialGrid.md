# SpatialGrid

<Badge type="info" text="Class" />

**Source:** `SpatialGrid.h`

## Description

Optimized spatial partitioning with separate static/dynamic layers.

Static layer: built once per level (or when entities change), not cleared each frame.
Dynamic layer: cleared and refilled every frame (RIGID, KINEMATIC).
Reduces per-frame cost when many static tiles are present.

## Methods

### `void clearDynamic()`

**Description:**

Clears all dynamic entities from the grid.

### `void clear()`

**Description:**

Clears all entities (static and dynamic) from the grid.

### `void markStaticDirty()`

**Description:**

Marks the static layer as dirty, requiring a rebuild.
