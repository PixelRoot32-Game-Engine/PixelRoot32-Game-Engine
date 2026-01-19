/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#include "graphics/Color.h"

namespace pixelroot32::graphics {

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value.
 * @param color The Color enum value.
 * @return The 16-bit color value.
 */
uint16_t resolveColor(Color color) {
    uint8_t idx = static_cast<uint8_t>(color);
    if (idx >= static_cast<uint8_t>(Color::COUNT)) {
        return 0xFFFF; // fallback white
    }
    return ENGINE_PALETTE[idx];
}

}
