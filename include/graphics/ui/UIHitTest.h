/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UIHitTest.h - AABB hit testing for touch widgets
 * O(N) hit test finding top-most widget first
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <cstdint>
#include "graphics/ui/UITouchWidget.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UIHitTest
 * @brief AABB hit testing for touch UI widgets
 * 
 * Provides hit testing for touch widgets. Iterates through all widgets
 * in reverse order (top-most first) to find the first hit.
 */
class UIHitTest {
public:
    /**
     * @brief Check if a point hits a single widget (AABB)
     * @param widget The widget to test
     * @param px X coordinate
     * @param py Y coordinate
     * @return true if point is inside widget bounds
     */
    static bool hitTest(const UITouchWidget& widget, int16_t px, int16_t py) {
        // Widget must be enabled, visible, and contain the point
        return widget.isEnabled() && 
               widget.isVisible() && 
               widget.contains(px, py);
    }
    
    /**
     * @brief Find the top-most widget that contains the point
     * @param widgets Array of widgets to search
     * @param count Number of widgets in array
     * @param px X coordinate
     * @param py Y coordinate
     * @return Pointer to hit widget, or nullptr if no hit
     * 
     * Searches in reverse order (last widget = top-most) for O(1) best case
     */
    static UITouchWidget* findHit(UITouchWidget* widgets[], uint8_t count, int16_t px, int16_t py) {
        // Iterate backwards to find top-most widget first
        for (int8_t i = static_cast<int8_t>(count) - 1; i >= 0; --i) {
            if (hitTest(*widgets[i], px, py)) {
                return widgets[i];
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Find the top-most widget that contains the point (const version)
     * @param widgets Array of widgets to search
     * @param count Number of widgets in array
     * @param px X coordinate
     * @param py Y coordinate
     * @return Pointer to hit widget, or nullptr if no hit
     */
    static const UITouchWidget* findHit(const UITouchWidget* widgets[], uint8_t count, int16_t px, int16_t py) {
        // Iterate backwards to find top-most widget first
        for (int8_t i = static_cast<int8_t>(count) - 1; i >= 0; --i) {
            if (hitTest(*widgets[i], px, py)) {
                return widgets[i];
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Check if a rectangle intersects with a widget (AABB)
     * @param widget The widget to test
     * @param rx Rectangle X (top-left)
     * @param ry Rectangle Y (top-left)
     * @param rw Rectangle width
     * @param rh Rectangle height
     * @return true if rectangles intersect
     */
    static bool intersects(const UITouchWidget& widget, int16_t rx, int16_t ry, 
                           uint16_t rw, uint16_t rh) {
        int16_t widgetRight = widget.x + static_cast<int16_t>(widget.width);
        int16_t widgetBottom = widget.y + static_cast<int16_t>(widget.height);
        int16_t rectRight = rx + static_cast<int16_t>(rw);
        int16_t rectBottom = ry + static_cast<int16_t>(rh);
        
        return !(widget.x >= rectRight || widgetRight < rx ||
                 widget.y >= rectBottom || widgetBottom < ry);
    }
};

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
