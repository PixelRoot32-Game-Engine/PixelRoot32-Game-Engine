# DirtyGrid

<Badge type="info" text="Class" />

**Source:** `DirtyGrid.h`

## Description

Two-buffer dirty cell grid (8×8 px cells) for selective framebuffer clears.

Bit-packed storage: one bit per cell. `curr` accumulates marks for the current frame;
`swapAndClear()` moves that state into `prev` for the next frame's cleanup pass.

## Methods

### `void init(int screenW, int screenH)`

**Description:**

Initializes the dirty grid dimensions based on screen size.

**Parameters:**

- `screenW`: Width of the screen in pixels.
- `screenH`: Height of the screen in pixels.

### `void markCell(uint8_t cx, uint8_t cy)`

**Description:**

Marks a specific cell as dirty in the current frame.

**Parameters:**

- `cx`: Cell X coordinate.
- `cy`: Cell Y coordinate.

### `void markRect(int x, int y, int w, int h)`

**Description:**

Marks cells intersected by a rectangle as dirty.

**Parameters:**

- `x`: Top-left X coordinate in pixels.
- `y`: Top-left Y coordinate in pixels.
- `w`: Width of the rectangle in pixels.
- `h`: Height of the rectangle in pixels.

### `bool isPrevDirty(uint8_t cx, uint8_t cy) const`

**Description:**

Checks if a cell was marked dirty in the previous frame.

**Parameters:**

- `cx`: Cell X coordinate.
- `cy`: Cell Y coordinate.

**Returns:** true if the cell was dirty in the previous frame, false otherwise.

### `void swapAndClear()`

**Description:**

Swaps the current and previous buffers, clearing the new current buffer.

### `void markAll()`

**Description:**

Marks all cells as dirty in the current frame.

### `bool isFullDirty() const`

**Description:**

Checks if the entire grid is marked as fully dirty.

**Returns:** true if the grid is fully dirty, false otherwise.

### `void setFullDirty(bool v)`

**Description:**

Sets the full dirty state of the grid.

**Parameters:**

- `v`: The full dirty state to set.

### `uint8_t getCols() const`

**Description:**

Gets the number of columns in the grid.

**Returns:** The number of columns.

### `uint8_t getRows() const`

**Description:**

Gets the number of rows in the grid.

**Returns:** The number of rows.

### `uint32_t countPrevMarkedCells() const`

**Description:**

Gets the number of cells marked in the previous frame.

**Returns:** The number of cells marked in the previous frame.

### `uint32_t countCurrMarkedCells() const`

**Description:**

Gets the number of cells marked in the current frame.

**Returns:** The number of cells marked in the current frame.

### `uint32_t totalCellCount() const`

**Description:**

Gets the total number of cells in the grid.

**Returns:** The total number of cells in the grid.

### `bool isCurrMarked(uint8_t cx, uint8_t cy) const`

**Description:**

True when the curr buffer has this cell marked for the current frame.

**Parameters:**

- `cx`: Cell X coordinate.
- `cy`: Cell Y coordinate.

**Returns:** True when the curr buffer has this cell marked for the current frame, false otherwise.

### `void clearFramebuffer8FromPrev(uint8_t* fb, int framebufferWidth, int framebufferHeight, uint8_t fillByte) const`

**Parameters:**

- `framebufferWidth`: Row stride in bytes (typically logical width).
