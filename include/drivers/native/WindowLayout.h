/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#ifndef PIXELROOT32_WINDOW_LAYOUT_H
#define PIXELROOT32_WINDOW_LAYOUT_H

#include <algorithm>
#include <cmath>

// Window geometry for the native drawer, kept out of SDL2_Drawer.cpp so it can
// be reasoned about (and tested) without a display.
//
// The drawer has three coordinate spaces, and the old hardcoded `windowScale = 2`
// conflated all of them:
//
//   framebuffer  the surface the game draws to (logicalWidth x logicalHeight,
//                placed at xOffset/yOffset inside the physical display area)
//   physical     the display the surface is pretending to be (physicalWidth x
//                physicalHeight)
//   window       what SDL actually allocated: points on a HiDPI screen, pixels
//                on a plain one, and whatever the user resizes it to
//
// `WindowLayout` maps physical onto window (scale + letterbox origin); the
// framebuffer rect and the inverse input mapping are derived from that one
// decision, so scaling, resizing, fullscreen and DPI cannot disagree with each
// other the way the two copies of `2` did.
namespace pixelroot32::drivers::native {

/**
 * @struct WindowRect
 * @brief Pixel rectangle in window coordinates (SDL_Rect without the SDL include).
 */
struct WindowRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

/**
 * @struct WindowLayout
 * @brief How the physical display area is placed inside the window.
 *
 * `scale` is window units per physical pixel and the origins are the letterbox
 * offsets that centre the physical area. A layout with scale 0 is invalid.
 */
struct WindowLayout {
    float scale = 1.0f;
    float originX = 0.0f;
    float originY = 0.0f;
};

/**
 * @brief Default window scale used when the display cannot be queried.
 *
 * Matches the pre-H5 behaviour, so a headless or dummy video driver (CI) still
 * gets a deterministic window instead of a one-to-one thumbnail.
 */
inline constexpr int kFallbackWindowScale = 2;

/**
 * @brief Largest window scale allowed when sizing a new window.
 *
 * A 240x240 game at 4x is 960x960 window units, which already fills a laptop
 * screen; beyond that the default would open larger than most displays.
 */
inline constexpr int kMaxDefaultWindowScale = 4;

/**
 * @brief Fraction of the usable display a new window may aim for.
 */
inline constexpr float kDefaultWindowFill = 0.6f;

/**
 * @brief Computes the placement of the physical area inside a window.
 *
 * Above 1x the scale is an integer, because a pixel-art framebuffer scaled by
 * 2.4 gets rows of different thickness. Below 1x (window shrunk smaller than
 * the panel) the exact ratio is used: crispness is already lost, and an integer
 * scale would floor to 1 and overflow the window.
 *
 * @param windowWidth Window width in its own units (pixels or points).
 * @param windowHeight Window height in its own units.
 * @param physicalWidth Physical display width in framebuffer pixels.
 * @param physicalHeight Physical display height in framebuffer pixels.
 * @return Centred layout, or `{1, 0, 0}` when any dimension is not positive.
 */
inline WindowLayout fitWindowLayout(int windowWidth, int windowHeight,
                                    int physicalWidth, int physicalHeight) {
    if (windowWidth <= 0 || windowHeight <= 0 || physicalWidth <= 0 || physicalHeight <= 0) {
        return WindowLayout{};
    }

    const float exact = std::min(static_cast<float>(windowWidth) / static_cast<float>(physicalWidth),
                                 static_cast<float>(windowHeight) / static_cast<float>(physicalHeight));

    WindowLayout layout;
    if (exact >= 1.0f) {
        const float integral = std::floor(exact);
        layout.scale = integral >= 1.0f ? integral : 1.0f;
    } else {
        layout.scale = exact;
    }

    layout.originX = (static_cast<float>(windowWidth) - static_cast<float>(physicalWidth) * layout.scale) * 0.5f;
    layout.originY = (static_cast<float>(windowHeight) - static_cast<float>(physicalHeight) * layout.scale) * 0.5f;
    return layout;
}

/**
 * @brief Default scale for a newly created window.
 *
 * Aims for `kDefaultWindowFill` of the usable display, so the default window
 * grows with the screen instead of being a fixed 2x that is tiny on a 4K panel
 * and larger than the screen on a small one. The result is clamped to
 * [1, kMaxDefaultWindowScale]; a display that cannot be queried falls back to
 * `kFallbackWindowScale`.
 *
 * @param physicalWidth Physical display width in framebuffer pixels.
 * @param physicalHeight Physical display height in framebuffer pixels.
 * @param usableWidth Usable display width in window units, 0 if unknown.
 * @param usableHeight Usable display height in window units, 0 if unknown.
 * @return Integer window scale, always >= 1.
 */
inline int defaultWindowScale(int physicalWidth, int physicalHeight,
                              int usableWidth, int usableHeight) {
    if (usableWidth <= 0 || usableHeight <= 0 || physicalWidth <= 0 || physicalHeight <= 0) {
        return kFallbackWindowScale;
    }

    const float fitWidth = static_cast<float>(usableWidth) / static_cast<float>(physicalWidth);
    const float fitHeight = static_cast<float>(usableHeight) / static_cast<float>(physicalHeight);
    const float target = kDefaultWindowFill * std::min(fitWidth, fitHeight);

    int scale = static_cast<int>(std::lround(target));
    scale = std::max(scale, 1);
    scale = std::min(scale, kMaxDefaultWindowScale);
    return scale;
}

/**
 * @brief Destination rectangle of the framebuffer inside the window.
 *
 * The framebuffer sits at (xOffset, yOffset) within the physical area, which is
 * letterboxed in the window, so both offsets and the letterbox move it.
 */
inline WindowRect framebufferRect(const WindowLayout& layout,
                                  int xOffset, int yOffset,
                                  int logicalWidth, int logicalHeight) {
    WindowRect rect;
    rect.x = static_cast<int>(std::lround(layout.originX + static_cast<float>(xOffset) * layout.scale));
    rect.y = static_cast<int>(std::lround(layout.originY + static_cast<float>(yOffset) * layout.scale));
    rect.width = static_cast<int>(std::lround(static_cast<float>(logicalWidth) * layout.scale));
    rect.height = static_cast<int>(std::lround(static_cast<float>(logicalHeight) * layout.scale));
    return rect;
}

/**
 * @brief Maps a window point back to framebuffer coordinates.
 *
 * The inverse of `framebufferRect`. Points outside the framebuffer (the
 * letterbox bars, or past the edge during a drag) are clamped rather than
 * dropped: a release outside the surface still has to reach the game, and a
 * press could otherwise leave a touch stuck down.
 */
inline void mapToFramebuffer(const WindowLayout& layout,
                             int windowX, int windowY,
                             int xOffset, int yOffset,
                             int logicalWidth, int logicalHeight,
                             int& outX, int& outY) {
    if (layout.scale <= 0.0f || logicalWidth <= 0 || logicalHeight <= 0) {
        outX = 0;
        outY = 0;
        return;
    }

    const float x = (static_cast<float>(windowX) - layout.originX) / layout.scale - static_cast<float>(xOffset);
    const float y = (static_cast<float>(windowY) - layout.originY) / layout.scale - static_cast<float>(yOffset);

    outX = static_cast<int>(std::floor(x + 0.5f));
    outY = static_cast<int>(std::floor(y + 0.5f));

    outX = std::clamp(outX, 0, logicalWidth - 1);
    outY = std::clamp(outY, 0, logicalHeight - 1);
}

} // namespace pixelroot32::drivers::native

#endif // PIXELROOT32_WINDOW_LAYOUT_H
