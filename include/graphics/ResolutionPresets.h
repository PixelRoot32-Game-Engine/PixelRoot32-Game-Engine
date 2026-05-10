/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License.
 */
#pragma once

#include "DisplayConfig.h"

namespace pixelroot32::graphics {

/**
 * @file ResolutionPresets.h
 * @brief Predefined logical resolution presets with associated memory savings.
 *
 * Provides a convenient enum + factory for common PixelRoot32 configurations.
 * Each preset pairs a logical resolution with automatic nearest-neighbor scaling
 * to the physical display, trading visual fidelity for framebuffer memory.
 *
 * Memory is calculated as: (logicalWidth × logicalHeight) bytes for 8bpp.
 * Savings are relative to 240×240 (57,600 bytes).
 */

/**
 * @enum ResolutionPreset
 * @brief Logical resolution choices for memory-constrained targets.
 *
 * Use create() to turn a preset into a DisplayConfig with the correct
 * logical and physical dimensions already set.
 */
enum ResolutionPreset {
    RES_240x240,  ///< Full resolution: 240×240 logical on 240×240 physical (57,600 bytes).
    RES_160x160,  ///< 160×160 logical scaled to 240×240 (~55% memory savings, ~25,600 bytes).
    RES_128x128,  ///< 128×128 logical scaled to 240×240 (~72% memory savings — recommended, ~16,384 bytes).
    RES_96x96,    ///< 96×96 logical scaled to 240×240 (~84% memory savings, very pixelated, ~9,216 bytes).
    RES_CUSTOM    ///< User-defined resolution.
};

/**
 * @class ResolutionPresets
 * @brief Factory for creating DisplayConfig from resolution presets.
 *
 * Simplifies display setup on memory-constrained targets by providing
 * a single create() call that sets logical dimensions, physical dimensions,
 * and rotation together.
 */
class ResolutionPresets {
public:
    /**
     * @brief Creates a DisplayConfig from a preset and hardware target.
     * @param preset Desired resolution preset.
     * @param type Physical display type (default ST7789).
     * @param physicalW Physical width in pixels (default 240).
     * @param physicalH Physical height in pixels (default 240).
     * @return Configured DisplayConfig with correct logical/physical dimensions.
     */
    static DisplayConfig create(ResolutionPreset preset, DisplayType type = ST7789, 
                               uint16_t physicalW = 240, uint16_t physicalH = 240) {
        switch (preset) {
            case RES_240x240:
                return DisplayConfig(type, 0, physicalW, physicalH, physicalW, physicalH);
            case RES_160x160:
                return DisplayConfig(type, 0, physicalW, physicalH, 160, 160);
            case RES_128x128:
                return DisplayConfig(type, 0, physicalW, physicalH, 128, 128);
            case RES_96x96:
                return DisplayConfig(type, 0, physicalW, physicalH, 96, 96);
            default:
                return DisplayConfig(type, 0, physicalW, physicalH, physicalW, physicalH);
        }
    }
};

} // namespace pixelroot32::graphics
