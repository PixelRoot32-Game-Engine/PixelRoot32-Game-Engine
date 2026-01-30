/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License.
 */
#pragma once

#include "DisplayConfig.h"

namespace pixelroot32::graphics {

/**
 * @brief Predefined resolution presets for different memory/performance profiles.
 */
enum ResolutionPreset {
    RES_240x240,  ///< Full resolution (no scaling), 240x240 logical on 240x240 physical.
    RES_160x160,  ///< 160x160 logical scaled to 240x240. ~55% memory savings.
    RES_128x128,  ///< 128x128 logical scaled to 240x240. ~72% memory savings (Recommended).
    RES_96x96,    ///< 96x96 logical scaled to 240x240. ~84% memory savings (Very pixelated).
    RES_CUSTOM    ///< User-defined resolution.
};

/**
 * @brief Helper class to create DisplayConfigs from presets.
 */
class ResolutionPresets {
public:
    /**
     * @brief Creates a DisplayConfig based on a preset and hardware target.
     * @param preset The desired resolution preset.
     * @param type The physical display type (default ST7789).
     * @param physicalW Physical width (default 240).
     * @param physicalH Physical height (default 240).
     * @return A configured DisplayConfig object.
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
