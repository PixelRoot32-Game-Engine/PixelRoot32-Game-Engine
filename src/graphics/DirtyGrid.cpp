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
      byteCount(other.byteCount),
      currMarkedCount_(other.currMarkedCount_),
      prevMarkedCount_(other.prevMarkedCount_) {
    other.cols             = 0;
    other.rows             = 0;
    other.prev             = nullptr;
    other.curr             = nullptr;
    other.fullDirty        = false;
    other.byteCount        = 0;
    other.currMarkedCount_ = 0;
    other.prevMarkedCount_ = 0;
}

DirtyGrid& DirtyGrid::operator=(DirtyGrid&& other) noexcept {
    if (this != &other) {
        freeBuffers();
        cols             = other.cols;
        rows             = other.rows;
        prev             = other.prev;
        curr             = other.curr;
        fullDirty        = other.fullDirty;
        byteCount        = other.byteCount;
        currMarkedCount_ = other.currMarkedCount_;
        prevMarkedCount_ = other.prevMarkedCount_;
        other.cols             = 0;
        other.rows             = 0;
        other.prev             = nullptr;
        other.curr             = nullptr;
        other.fullDirty        = false;
        other.byteCount        = 0;
        other.currMarkedCount_ = 0;
        other.prevMarkedCount_ = 0;
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
    prev             = nullptr;
    curr             = nullptr;
    cols             = 0;
    rows             = 0;
    byteCount        = 0;
    currMarkedCount_ = 0;
    prevMarkedCount_ = 0;
}

bool DirtyGrid::init(int screenW, int screenH) {
    assert(screenW > 0 && screenH > 0);
    freeBuffers();
    const int c = (screenW + static_cast<int>(CELL_W) - 1) / static_cast<int>(CELL_W);
    const int r = (screenH + static_cast<int>(CELL_H) - 1) / static_cast<int>(CELL_H);
    if (c > 255 || r > 255) {
        return false;
    }
    cols = static_cast<uint8_t>(c);
    rows = static_cast<uint8_t>(r);
    byteCount       = bytesForGrid(cols, rows);
    prev            = new (std::nothrow) uint8_t[byteCount];
    curr            = new (std::nothrow) uint8_t[byteCount];
    if (!prev || !curr) {
        freeBuffers();
        return false;
    }
    std::memset(prev, 0, byteCount);
    std::memset(curr, 0, byteCount);
    fullDirty        = false;
    currMarkedCount_ = 0;
    prevMarkedCount_ = 0;
    return true;
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
    prevMarkedCount_ = currMarkedCount_;
    std::memset(curr, 0, byteCount);
    currMarkedCount_ = 0;
}

void DirtyGrid::markAll() {
    fullDirty = true;
    if (curr && byteCount > 0) {
        std::memset(curr, 0xFF, byteCount);
        currMarkedCount_ = totalCellCount();
    }
}

void DirtyGrid::setBit(uint8_t* buf, uint8_t cx, uint8_t cy) {
    const uint32_t idx  = static_cast<uint32_t>(cy) * static_cast<uint32_t>(cols) + static_cast<uint32_t>(cx);
    const size_t   b    = idx >> 3;
    const uint8_t  mask = static_cast<uint8_t>(1u << (idx & 7u));
    if (buf == curr && !(buf[b] & mask)) {
        ++currMarkedCount_;
    }
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
    const uint8_t* end = buf + nbytes;
    while (buf < end) {
        n += __builtin_popcount(*buf);
        ++buf;
    }
    return n;
}

uint32_t DirtyGrid::countPrevMarkedCells() const {
    return prevMarkedCount_;
}

uint32_t DirtyGrid::countCurrMarkedCells() const {
    return currMarkedCount_;
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

    const size_t bytesPerRow = (cols + 7u) >> 3u;

    for (uint8_t cy = 0; cy < rows; ++cy) {
        const int py = static_cast<int>(cy) * static_cast<int>(CELL_H);
        if (py >= framebufferHeight) {
            break;
        }
        const int rowH = std::min(static_cast<int>(CELL_H), framebufferHeight - py);

        size_t byteIdx = static_cast<size_t>(cy) * bytesPerRow;
        uint8_t cx = 0;

        while (byteIdx <= static_cast<size_t>(cy + 1) * bytesPerRow - 1 && cx < cols) {
            if (prev[byteIdx] == 0) {
                cx += 8;
                ++byteIdx;
                continue;
            }

            const uint8_t bits = prev[byteIdx];
            const int localCx = static_cast<int>(cx);

            if (bits == 0xFF) {
                int runStart = localCx;
                int runEnd = localCx;

                size_t nextByteIdx = byteIdx + 1;
                while (nextByteIdx < static_cast<size_t>(cy + 1) * bytesPerRow && prev[nextByteIdx] == 0xFF) {
                    runEnd += 8;
                    ++nextByteIdx;
                }
                runEnd += 7;
                cx = static_cast<uint8_t>(std::min(runEnd + 1, static_cast<int>(cols)));
                byteIdx = nextByteIdx;

                const int px = runStart * static_cast<int>(CELL_W);
                if (px >= framebufferWidth) {
                    continue;
                }
                int spanCells = runEnd - runStart + 1;
                int wpixels = spanCells * static_cast<int>(CELL_W);
                if (px + wpixels > framebufferWidth) {
                    wpixels = framebufferWidth - px;
                }
                if (wpixels > 0) {
                    uint8_t* rowPtr = fb + py * framebufferWidth + px;
                    for (int r = 0; r < rowH; ++r) {
                        std::memset(rowPtr + r * framebufferWidth, fillByte, static_cast<size_t>(wpixels));
                    }
                }
            } else {
                int pos = localCx;
                while (pos < static_cast<int>(cols)) {
                    while (pos < static_cast<int>(cols) && (bits & (1u << (pos & 7))) == 0) {
                        ++pos;
                    }
                    if (pos >= static_cast<int>(cols)) {
                        break;
                    }
                    int runStart = pos;
                    int runStartPx = runStart * static_cast<int>(CELL_W);
                    while (pos < static_cast<int>(cols) && (bits & (1u << (pos & 7))) != 0) {
                        ++pos;
                    }
                    int spanCells = pos - runStart;

                    if (runStartPx < framebufferWidth) {
                        int wpixels = spanCells * static_cast<int>(CELL_W);
                        if (runStartPx + wpixels > framebufferWidth) {
                            wpixels = framebufferWidth - runStartPx;
                        }
                        if (wpixels > 0) {
                            uint8_t* rowPtr = fb + py * framebufferWidth + runStartPx;
                            for (int r = 0; r < rowH; ++r) {
                                std::memset(rowPtr + r * framebufferWidth, fillByte, static_cast<size_t>(wpixels));
                            }
                        }
                    }
                }
                ++byteIdx;
                cx = static_cast<uint8_t>(std::min(static_cast<int>(cx) + 8, static_cast<int>(cols)));
            }
            if (byteIdx >= static_cast<size_t>(cy + 1) * bytesPerRow) {
                break;
            }
        }
    }
}

}  // namespace pixelroot32::graphics
