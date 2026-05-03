/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/DirtyGrid.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <new>
#include <utility>

namespace pixelroot32::graphics {

namespace {

inline int divFloorNonneg(int v, int d) {
    assert(v >= 0 && d > 0);
    return v / d;
}

}  // namespace

DirtyGrid::~DirtyGrid() {
    freeBuffers();
}

DirtyGrid::DirtyGrid(DirtyGrid&& other) noexcept
    : cols(other.cols),
      rows(other.rows),
      prev(other.prev),
      curr(other.curr),
      fullDirty(other.fullDirty),
      byteCount(other.byteCount) {
    other.cols       = 0;
    other.rows       = 0;
    other.prev       = nullptr;
    other.curr       = nullptr;
    other.fullDirty  = false;
    other.byteCount  = 0;
}

DirtyGrid& DirtyGrid::operator=(DirtyGrid&& other) noexcept {
    if (this != &other) {
        freeBuffers();
        cols       = other.cols;
        rows       = other.rows;
        prev       = other.prev;
        curr       = other.curr;
        fullDirty  = other.fullDirty;
        byteCount  = other.byteCount;
        other.cols       = 0;
        other.rows       = 0;
        other.prev       = nullptr;
        other.curr       = nullptr;
        other.fullDirty  = false;
        other.byteCount  = 0;
    }
    return *this;
}

size_t DirtyGrid::bytesForGrid(uint8_t c, uint8_t r) {
    const uint32_t cells = static_cast<uint32_t>(c) * static_cast<uint32_t>(r);
    return static_cast<size_t>((cells + 7u) / 8u);
}

void DirtyGrid::freeBuffers() {
    delete[] prev;
    delete[] curr;
    prev      = nullptr;
    curr      = nullptr;
    cols      = 0;
    rows      = 0;
    byteCount = 0;
}

void DirtyGrid::init(int screenW, int screenH) {
    assert(screenW > 0 && screenH > 0);
    freeBuffers();
    cols = static_cast<uint8_t>((screenW + static_cast<int>(CELL_W) - 1) / static_cast<int>(CELL_W));
    rows = static_cast<uint8_t>((screenH + static_cast<int>(CELL_H) - 1) / static_cast<int>(CELL_H));
    byteCount       = bytesForGrid(cols, rows);
    prev            = new (std::nothrow) uint8_t[byteCount];
    curr            = new (std::nothrow) uint8_t[byteCount];
    if (!prev || !curr) {
        freeBuffers();
        return;
    }
    std::memset(prev, 0, byteCount);
    std::memset(curr, 0, byteCount);
    fullDirty = false;
}

void DirtyGrid::markCell(uint8_t cx, uint8_t cy) {
    if (cx >= cols || cy >= rows || !curr) {
        return;
    }
    setBit(curr, cx, cy);
}

void DirtyGrid::markRect(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0 || !curr || cols == 0 || rows == 0) {
        return;
    }
    const int maxPxX = static_cast<int>(cols) * static_cast<int>(CELL_W) - 1;
    const int maxPxY = static_cast<int>(rows) * static_cast<int>(CELL_H) - 1;
    int       x1     = std::max(0, x);
    int       y1     = std::max(0, y);
    int       x2     = std::min(maxPxX, x + w - 1);
    int       y2     = std::min(maxPxY, y + h - 1);
    if (x1 > x2 || y1 > y2) {
        return;
    }
    const uint8_t cx0 = static_cast<uint8_t>(divFloorNonneg(x1, static_cast<int>(CELL_W)));
    const uint8_t cy0 = static_cast<uint8_t>(divFloorNonneg(y1, static_cast<int>(CELL_H)));
    const uint8_t cx1 = static_cast<uint8_t>(divFloorNonneg(x2, static_cast<int>(CELL_W)));
    const uint8_t cy1 = static_cast<uint8_t>(divFloorNonneg(y2, static_cast<int>(CELL_H)));
    for (uint8_t cy = cy0; cy <= cy1; ++cy) {
        for (uint8_t cx = cx0; cx <= cx1; ++cx) {
            setBit(curr, cx, cy);
        }
    }
}

bool DirtyGrid::isPrevDirty(uint8_t cx, uint8_t cy) const {
    if (cx >= cols || cy >= rows || !prev) {
        return false;
    }
    return getBit(prev, cx, cy);
}

void DirtyGrid::swapAndClear() {
    if (!prev || !curr) {
        return;
    }
    std::swap(prev, curr);
    std::memset(curr, 0, byteCount);
}

void DirtyGrid::markAll() {
    fullDirty = true;
}

void DirtyGrid::setBit(uint8_t* buf, uint8_t cx, uint8_t cy) {
    const uint32_t idx  = static_cast<uint32_t>(cy) * static_cast<uint32_t>(cols) + static_cast<uint32_t>(cx);
    const size_t   b    = idx >> 3;
    const uint8_t  mask = static_cast<uint8_t>(1u << (idx & 7u));
    buf[b] |= mask;
}

bool DirtyGrid::getBit(const uint8_t* buf, uint8_t cx, uint8_t cy) const {
    const uint32_t idx  = static_cast<uint32_t>(cy) * static_cast<uint32_t>(cols) + static_cast<uint32_t>(cx);
    const size_t   b    = idx >> 3;
    const uint8_t  mask = static_cast<uint8_t>(1u << (idx & 7u));
    return (buf[b] & mask) != 0;
}

uint32_t DirtyGrid::popcountBuffer(const uint8_t* buf, size_t nbytes) {
    uint32_t n = 0;
    for (size_t i = 0; i < nbytes; ++i) {
        uint8_t b = buf[i];
        while (b != 0) {
            ++n;
            b = static_cast<uint8_t>(b & static_cast<uint8_t>(b - 1));
        }
    }
    return n;
}

uint32_t DirtyGrid::countPrevMarkedCells() const {
    if (!prev) {
        return 0;
    }
    return popcountBuffer(prev, byteCount);
}

uint32_t DirtyGrid::countCurrMarkedCells() const {
    if (!curr) {
        return 0;
    }
    return popcountBuffer(curr, byteCount);
}

uint32_t DirtyGrid::totalCellCount() const {
    return static_cast<uint32_t>(cols) * static_cast<uint32_t>(rows);
}

bool DirtyGrid::isCurrMarked(uint8_t cx, uint8_t cy) const {
    if (!curr || cx >= cols || cy >= rows) {
        return false;
    }
    return getBit(curr, cx, cy);
}

void DirtyGrid::clearFramebuffer8FromPrev(uint8_t* fb,
                                          int framebufferWidth,
                                          int framebufferHeight,
                                          uint8_t fillByte) const {
    if (!fb || !prev || cols == 0 || rows == 0) {
        return;
    }
    for (uint8_t cy = 0; cy < rows; ++cy) {
        const int py = static_cast<int>(cy) * static_cast<int>(CELL_H);
        if (py >= framebufferHeight) {
            break;
        }
        const int rowH = std::min(static_cast<int>(CELL_H), framebufferHeight - py);

        uint8_t cx = 0;
        while (cx < cols) {
            while (cx < cols && !isPrevDirty(cx, cy)) {
                ++cx;
            }
            if (cx >= cols) {
                break;
            }
            const uint8_t runStart = cx;
            while (cx < cols && isPrevDirty(cx, cy)) {
                ++cx;
            }
            const uint8_t runEnd = static_cast<uint8_t>(cx - 1u);

            const int px = static_cast<int>(runStart) * static_cast<int>(CELL_W);
            if (px >= framebufferWidth) {
                continue;
            }

            const int spanCells = static_cast<int>(runEnd) - static_cast<int>(runStart) + 1;
            int         wpixels = spanCells * static_cast<int>(CELL_W);
            if (px + wpixels > framebufferWidth) {
                wpixels = framebufferWidth - px;
            }
            if (wpixels <= 0) {
                continue;
            }

            uint8_t* rowPtr = fb + py * framebufferWidth + px;
            for (int r = 0; r < rowH; ++r) {
                std::memset(rowPtr + r * framebufferWidth, fillByte, static_cast<size_t>(wpixels));
            }
        }
    }
}

}  // namespace pixelroot32::graphics
