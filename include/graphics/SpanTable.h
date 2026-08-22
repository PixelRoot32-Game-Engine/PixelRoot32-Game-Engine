/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file SpanTable.h
 * @brief Per-row opaque span metadata for 4bpp/2bpp sprites.
 *
 * Diamond-shaped tiles (isometric floors, hexagonal pieces, etc.) sit in a
 * rectangular bounding box padded with transparent pixels. The skip predicate
 * in drawSpriteInternal uses the metadata this header populates to skip
 * leading/trailing transparent nibbles per row, recovering ~50% of inner-loop
 * iterations for the most common case.
 *
 * ### Lifetime contract
 *
 * computeSpanTable writes into caller-provided uint8_t[height] arrays. The
 * sprite struct then holds non-owning pointers to those arrays. The buffers
 * MUST outlive any draw call that uses the resulting sprite.
 *
 * ### Convex-row limitation
 *
 * Span limits only skip leading/trailing transparent runs. Interior
 * transparent pixels within [rowMinX, rowMaxX] are still iterated by
 * drawSpriteInternal's existing val==0 branch. The metadata is therefore
 * an over-approximation that is safe to skip into.
 *
 * ### Fully-transparent rows
 *
 * A row with no opaque nibbles produces {minX=0, maxX=0} (an empty range).
 * drawSpriteInternal treats that as a no-op for the row.
 */
#pragma once

#include "graphics/Renderer.h"

namespace pixelroot32::graphics {

/**
 * @brief Scans a 4bpp sprite's nibbles once at init time and writes the
 *        per-row opaque span into caller-provided arrays.
 *
 * For each row r in [0, height):
 *   - If any nibble is non-zero, outMinX[r] = smallest opaque column,
 *     outMaxX[r] = one past the largest opaque column.
 *   - If the row is fully transparent, outMinX[r] = outMaxX[r] = 0.
 *
 * Odd widths are handled: the last nibble lives in the high half of the
 * last byte. No dynamic allocation. NOT IRAM_ATTR (init-time only).
 *
 * @param s Sprite whose data is scanned. rowMinX/rowMaxX are NOT modified;
 *          the caller is responsible for assigning them after this call.
 * @param outMinX Caller-owned uint8_t[height]. Written.
 * @param outMaxX Caller-owned uint8_t[height]. Written.
 */
void computeSpanTable(Sprite4bpp& s, uint8_t* outMinX, uint8_t* outMaxX);

/**
 * @brief 2bpp overload of computeSpanTable. Extracts 2-bit pairs per pixel.
 *
 * @param s Sprite whose data is scanned.
 * @param outMinX Caller-owned uint8_t[height]. Written.
 * @param outMaxX Caller-owned uint8_t[height]. Written.
 */
void computeSpanTable(Sprite2bpp& s, uint8_t* outMinX, uint8_t* outMaxX);

}  // namespace pixelroot32::graphics
