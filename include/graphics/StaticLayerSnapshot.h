/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
#include <cstdlib>
#include <memory>
#endif

#include "graphics/Renderer.h"

namespace pixelroot32::graphics {

/**
 * @class StaticLayerSnapshot
 * @brief Framebuffer cache for static layers a game draws ITSELF.
 *
 * `StaticTilemapLayerCache` solves the same problem for games whose background
 * is a `TileMap4bpp`: it owns the tilemaps, redraws them when the cache goes
 * cold, and snapshots the result. That ownership is exactly what an isometric
 * or oblique room cannot satisfy -- `drawTileMap()` assumes axis-aligned cells
 * and cannot express a diamond, so such a room is drawn sprite-per-cell by game
 * code and there is no `TileMap4bpp` to hand over.
 *
 * This class inverts the relationship: it never draws anything. The game says
 * "the framebuffer now holds my static layers" (capture()) and, on later
 * frames, "put them back" (restore()). What those layers are, and how they were
 * drawn, is none of its business -- which is the whole point, and why this is
 * projection-agnostic where the tilemap cache is not.
 *
 * ### What it costs, stated plainly
 *
 * One full logical framebuffer of heap: 57,600 B at 240x240. That is the entire
 * trade. In exchange a static layer of N sprites costs zero draw calls per
 * frame instead of N, and with dirty regions enabled the restore touches only
 * the cells last frame's movers dirtied rather than the whole buffer.
 *
 * ### Restore has two speeds, and the fast one needs dirty regions
 *
 * With `PIXELROOT32_ENABLE_DIRTY_REGIONS`, restore() repaints only the
 * previously-dirtied cells -- for a room with one moving actor that is a few
 * hundred bytes. Without it, there is no record of what moved, so restore()
 * falls back to copying the whole buffer. Both are correct; the selective path
 * is roughly two orders of magnitude cheaper. See restore().
 *
 * ### Usage
 *
 * @code
 * // Scene::init() -- allocate off the game loop (see ARCH_MEMORY_SYSTEM.md).
 * if (!snapshot_.allocateForRenderer(engine.getRenderer())) {
 *     // Out of memory: every call below degrades to "draw normally".
 * }
 *
 * // Scene::adviseFramebufferBeforeBeginFrame() -- lets beginFrame skip work
 * // restore() is about to redo anyway.
 * snapshot_.adviseFramebufferBeforeBeginFrame(renderer);
 *
 * // In the static layer's draw(), before anything dynamic:
 * if (!snapshot_.restore(renderer)) {
 *     drawMyStaticLayers(renderer);   // cache cold, or unavailable
 *     snapshot_.capture(renderer);
 * }
 * @endcode
 *
 * Call invalidate() whenever the static layers would draw differently -- a
 * palette swap, a camera move, a door opening. The class cannot detect that on
 * its own and deliberately does not try: it has no idea what it is holding.
 */
class StaticLayerSnapshot {
public:
    StaticLayerSnapshot() = default;

    /** Releases the buffer and marks the snapshot invalid. */
    void clear();

    /**
     * @brief Pre-allocates a buffer of width * height bytes.
     *
     * Call from scene init(), never from the game loop. Re-allocating for the
     * size already held keeps the buffer and only invalidates the contents.
     *
     * @return false if the dimensions are invalid or the allocation failed. A
     *         false return is not fatal: every other method then reports
     *         "unavailable" and the caller draws normally.
     */
    [[nodiscard]] bool allocateForLogicalSize(int width, int height);

    /** @brief Same as allocateForLogicalSize(renderer.getLogicalWidth/Height()). */
    [[nodiscard]] bool allocateForRenderer(const Renderer& renderer);

    /** Forces the next restore() to report "cold" so the caller redraws. */
    void invalidate();

    /** True when a capture() has happened and no invalidate() has since. */
    [[nodiscard]] bool isValid() const;

    /**
     * @brief Copies the current framebuffer into the snapshot.
     *
     * Call immediately after drawing the static layers and BEFORE drawing
     * anything dynamic -- whatever is on the framebuffer at this moment is what
     * every later restore() will put back, moving actors included if you get
     * the order wrong.
     *
     * @return false when there is no buffer or the driver exposes no 8bpp
     *         framebuffer; the snapshot stays invalid and restore() keeps
     *         reporting cold.
     */
    bool capture(Renderer& renderer);

    /**
     * @brief Puts the static layers back onto the framebuffer.
     *
     * Repaints only the cells dirtied by the previous frame when dirty regions
     * are enabled; copies the whole buffer otherwise. Call after beginFrame()
     * and before drawing anything dynamic.
     *
     * @return false when the snapshot is cold or unavailable, which is the
     *         caller's signal to draw its static layers and capture() them.
     */
    bool restore(Renderer& renderer);

    /**
     * @brief Tells the renderer it may skip its own clear this frame.
     *
     * Call from `Scene::adviseFramebufferBeforeBeginFrame`, which the engine
     * runs before `Renderer::beginFrame`. Only advises suppression on the
     * frames restore() will handle the framebuffer itself; on a cold frame the
     * clear must still happen, so this stays silent.
     *
     * Harmless to skip -- the result is a redundant clear, not a wrong frame.
     */
    void adviseFramebufferBeforeBeginFrame(Renderer& renderer) const;

private:
#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    /** std::malloc / std::free — STYLE_GUIDE forbids operator new; avoid game-loop alloc. */
    struct SnapshotBufferDeleter {
        void operator()(uint8_t* p) const noexcept {
            std::free(p);
        }
    };

    /// True when the renderer can actually be snapshotted at this size.
    [[nodiscard]] bool usable(Renderer& renderer) const;

    std::unique_ptr<uint8_t, SnapshotBufferDeleter> snapshotBytes;
    std::size_t snapshotByteCount = 0;
    bool snapshotValid = false;
#endif
};

}  // namespace pixelroot32::graphics
