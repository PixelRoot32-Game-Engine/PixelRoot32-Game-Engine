/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/StaticLayerSnapshot.h"

#include <cstring>

#include "platforms/EngineConfig.h"

namespace pixelroot32::graphics {

void StaticLayerSnapshot::clear() {
#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    snapshotBytes.reset();
    snapshotByteCount = 0;
    snapshotValid = false;
#endif
}

bool StaticLayerSnapshot::allocateForLogicalSize(int width, int height) {
#if !PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    (void)width;
    (void)height;
    return false;
#else
    if (width <= 0 || height <= 0) {
        clear();
        return false;
    }

    const std::size_t n = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (snapshotBytes && snapshotByteCount == n) {
        // Same size: keep the buffer, drop the contents. Re-allocating here
        // would hand back memory the allocator may not return on a fragmented
        // heap, which is the one failure a game cannot recover from mid-scene.
        invalidate();
        return true;
    }

    snapshotBytes.reset();
    snapshotByteCount = 0;

    // malloc (not operator new per STYLE_GUIDE); nullptr on OOM without abort.
    uint8_t* raw = static_cast<uint8_t*>(std::malloc(n));
    if (!raw) {
        return false;
    }
    snapshotBytes.reset(raw);
    snapshotByteCount = n;
    snapshotValid = false;
    return true;
#endif
}

bool StaticLayerSnapshot::allocateForRenderer(const Renderer& renderer) {
    return allocateForLogicalSize(renderer.getLogicalWidth(), renderer.getLogicalHeight());
}

void StaticLayerSnapshot::invalidate() {
#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    snapshotValid = false;
#endif
}

bool StaticLayerSnapshot::isValid() const {
#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    return snapshotValid;
#else
    return false;
#endif
}

#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
bool StaticLayerSnapshot::usable(Renderer& renderer) const {
    if (!snapshotBytes) {
        return false;
    }
    const uint8_t* fb = renderer.getDrawSurface().getSpriteBuffer();
    if (fb == nullptr) {
        return false;
    }
    // A resolution change since allocateForLogicalSize() would make every
    // offset wrong; report unusable rather than copy across mismatched strides.
    const std::size_t bufBytes = static_cast<std::size_t>(renderer.getLogicalWidth()) *
                                 static_cast<std::size_t>(renderer.getLogicalHeight());
    return snapshotByteCount == bufBytes;
}
#endif

bool StaticLayerSnapshot::capture(Renderer& renderer) {
#if !PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    (void)renderer;
    return false;
#else
    if (!usable(renderer)) {
        return false;
    }
    const uint8_t* fb = renderer.getDrawSurface().getSpriteBuffer();
    std::memcpy(snapshotBytes.get(), fb, snapshotByteCount);
    snapshotValid = true;
    return true;
#endif
}

bool StaticLayerSnapshot::restore(Renderer& renderer) {
#if !PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    (void)renderer;
    return false;
#else
    if (!snapshotValid || !usable(renderer)) {
        return false;
    }

    // Selective path: repaint only what last frame's movers disturbed. For a
    // room with one actor that is a few hundred bytes against 57,600.
    if (renderer.restoreDirtyCellsFromSnapshot(snapshotBytes.get())) {
        return true;
    }

    // No dirty-region record of what moved, so everything is suspect.
    uint8_t* fb = renderer.getDrawSurface().getSpriteBuffer();
    std::memcpy(fb, snapshotBytes.get(), snapshotByteCount);
    return true;
#endif
}

void StaticLayerSnapshot::adviseFramebufferBeforeBeginFrame(Renderer& renderer) const {
#if !PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    (void)renderer;
#else
    if constexpr (!pixelroot32::platforms::config::EnableDirtyRegions) {
        (void)renderer;
    } else {
        // Only on frames restore() will cover the framebuffer itself. On a cold
        // frame the caller redraws from scratch and still needs the clear.
        const bool willRestore = snapshotValid && snapshotBytes != nullptr;
        renderer.accumulateFramebufferClearSuppressionAdvice(willRestore);
    }
#endif
}

}  // namespace pixelroot32::graphics
