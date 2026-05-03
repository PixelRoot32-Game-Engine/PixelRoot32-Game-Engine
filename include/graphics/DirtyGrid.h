/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace pixelroot32::graphics {

/**
 * @class DirtyGrid
 * @brief Two-buffer dirty cell grid (8×8 px cells) for selective framebuffer clears.
 *
 * Bit-packed storage: one bit per cell. `curr` accumulates marks for the current frame;
 * `swapAndClear()` moves that state into `prev` for the next frame's cleanup pass.
 */
class DirtyGrid {
public:
    static constexpr uint8_t CELL_W = 8;
    static constexpr uint8_t CELL_H = 8;

    DirtyGrid() = default;
    ~DirtyGrid();

    DirtyGrid(DirtyGrid&& other) noexcept;
    DirtyGrid& operator=(DirtyGrid&& other) noexcept;

    DirtyGrid(const DirtyGrid&) = delete;
    DirtyGrid& operator=(const DirtyGrid&) = delete;

    /**
     * @brief Initializes the dirty grid dimensions based on screen size.
     * @param screenW Width of the screen in pixels.
     * @param screenH Height of the screen in pixels.
     */
    void init(int screenW, int screenH);

    /**
     * @brief Marks a specific cell as dirty in the current frame.
     * @param cx Cell X coordinate.
     * @param cy Cell Y coordinate.
     */
    void markCell(uint8_t cx, uint8_t cy);

    /**
     * @brief Marks cells intersected by a rectangle as dirty.
     * @param x Top-left X coordinate in pixels.
     * @param y Top-left Y coordinate in pixels.
     * @param w Width of the rectangle in pixels.
     * @param h Height of the rectangle in pixels.
     */
    void markRect(int x, int y, int w, int h);

    /**
     * @brief Checks if a cell was marked dirty in the previous frame.
     * @param cx Cell X coordinate.
     * @param cy Cell Y coordinate.
     * @return true if the cell was dirty in the previous frame, false otherwise.
     */
    bool isPrevDirty(uint8_t cx, uint8_t cy) const;

    /**
     * @brief Swaps the current and previous buffers, clearing the new current buffer.
     */
    void swapAndClear();

    /**
     * @brief Marks all cells as dirty in the current frame.
     */
    void markAll();

    /**
     * @brief Checks if the entire grid is marked as fully dirty.
     * @return true if the grid is fully dirty, false otherwise.
     */
    bool isFullDirty() const { return fullDirty; }

    /**
     * @brief Sets the full dirty state of the grid.
     * @param v The full dirty state to set.
     */
    void setFullDirty(bool v) { fullDirty = v; }

    /**
     * @brief Gets the number of columns in the grid.
     * @return The number of columns.
     */
    uint8_t getCols() const { return cols; }

    /**
     * @brief Gets the number of rows in the grid.
     * @return The number of rows.
     */
    uint8_t getRows() const { return rows; }

    /**
     * @brief Gets the number of cells marked in the previous frame.
     * @return The number of cells marked in the previous frame.
     */
    uint32_t countPrevMarkedCells() const;

    /**
     * @brief Gets the number of cells marked in the current frame.
     * @return The number of cells marked in the current frame.
     */
    uint32_t countCurrMarkedCells() const;

    /**
     * @brief Gets the total number of cells in the grid.
     * @return The total number of cells in the grid.
     */
    uint32_t totalCellCount() const;

    /**
     * @brief True when the curr buffer has this cell marked for the current frame.
     * @param cx Cell X coordinate.
     * @param cy Cell Y coordinate.
     * @return True when the curr buffer has this cell marked for the current frame, false otherwise.
     */
    bool isCurrMarked(uint8_t cx, uint8_t cy) const;

    /**
     * Zeros 8×8 regions in an 8bpp linear framebuffer for each cell set in `prev`.
     * Merges contiguous dirty cells per scanline into single horizontal memsets.
     * @param framebufferWidth Row stride in bytes (typically logical width).
     */
    void clearFramebuffer8FromPrev(uint8_t* fb, int framebufferWidth, int framebufferHeight, uint8_t fillByte) const;

private:
    uint8_t  cols = 0;
    uint8_t  rows = 0;
    uint8_t* prev = nullptr;
    uint8_t* curr = nullptr;
    bool     fullDirty = false;
    size_t   byteCount = 0;

    static size_t bytesForGrid(uint8_t c, uint8_t r);
    void            freeBuffers();
    void            setBit(uint8_t* buf, uint8_t cx, uint8_t cy);
    bool            getBit(const uint8_t* buf, uint8_t cx, uint8_t cy) const;
    static uint32_t popcountBuffer(const uint8_t* buf, size_t nbytes);
};

}  // namespace pixelroot32::graphics
