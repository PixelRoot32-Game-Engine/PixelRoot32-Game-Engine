/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include "platforms/EngineConfig.h"
#include "graphics/Color.h"
#include "graphics/PaletteDefs.h"

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace pixelroot32::graphics {

struct PaletteEntry {
    PaletteType type;
    const uint16_t* colors;
};

static constexpr PaletteEntry kPalettes[] = {
    { PaletteType::NES,   PALETTE_NES   },
    { PaletteType::GB,    PALETTE_GB    },
    { PaletteType::GBC,   PALETTE_GBC   },
    { PaletteType::PICO8, PALETTE_PICO8 },
    { PaletteType::PR32,  PALETTE_PR32  },
};


// Current active palette pointer. Defaults to PR32.
// Used in legacy mode (single palette) for backward compatibility.
static const uint16_t* currentPalette = PALETTE_PR32;

// Dual palette mode: separate palettes for backgrounds and sprites.
static const uint16_t* backgroundPalette = PALETTE_PR32;
static const uint16_t* spritePalette = PALETTE_PR32;
static bool dualPaletteMode = false;

// Multi-palette background: bank of 8 slots; slot 0 = default = backgroundPalette semantics.
static constexpr uint8_t kNumBackgroundPaletteSlots =
    static_cast<uint8_t>(pixelroot32::platforms::config::kMaxBackgroundPaletteSlots);
static const uint16_t* backgroundPaletteSlots[kNumBackgroundPaletteSlots] = {};

// Multi-palette sprites: bank of 8 slots; slot 0 = default = spritePalette semantics.
static constexpr uint8_t kNumSpritePaletteSlots =
    static_cast<uint8_t>(pixelroot32::platforms::config::kMaxSpritePaletteSlots);
static const uint16_t* spritePaletteSlots[kNumSpritePaletteSlots] = {};

static void ensureBackgroundPaletteSlotsInited() {
    if (backgroundPaletteSlots[0] != nullptr) return;
    for (uint8_t i = 0; i < kNumBackgroundPaletteSlots; i++)
        backgroundPaletteSlots[i] = PALETTE_PR32;
}

static void ensureSpritePaletteSlotsInited() {
    if (spritePaletteSlots[0] != nullptr) return;
    for (uint8_t i = 0; i < kNumSpritePaletteSlots; i++)
        spritePaletteSlots[i] = PALETTE_PR32;
}

void initBackgroundPaletteSlots() {
    for (uint8_t i = 0; i < kNumBackgroundPaletteSlots; i++)
        backgroundPaletteSlots[i] = PALETTE_PR32;
}

void initSpritePaletteSlots() {
    for (uint8_t i = 0; i < kNumSpritePaletteSlots; i++)
        spritePaletteSlots[i] = PALETTE_PR32;
}

/** 
 * @brief Set palette (legacy mode).
 * Sets both background and sprite palettes to the same value.
 * Does not enable dual palette mode (maintains backward compatibility).
 * @param palette type of palette.
 * 
*/
void setPalette(PaletteType palette) {
    const uint16_t* selectedPalette = PALETTE_PR32; // fallback
    
    for (const auto& entry : kPalettes) {
        if (entry.type == palette) {
            selectedPalette = entry.colors;
            break;
        }
    }
    
    // Set all palettes to the same value (legacy behavior)
    currentPalette = selectedPalette;
    backgroundPalette = selectedPalette;
    spritePalette = selectedPalette;
    ensureBackgroundPaletteSlotsInited();
    backgroundPaletteSlots[0] = selectedPalette;
    ensureSpritePaletteSlotsInited();
    spritePaletteSlots[0] = selectedPalette;
    // Keep dualPaletteMode = false for backward compatibility
}

/** 
 * @brief Set custom palette (legacy mode).
 * Sets both background and sprite palettes to the same value.
 * Does not enable dual palette mode (maintains backward compatibility).
 * @param palette array colors.
 * 
 * Note:
 * The engine does NOT copy the palette.
 * The palette pointer must remain valid for the entire usage period.
*/
void setCustomPalette(const uint16_t* palette) {
    if (palette != nullptr) {
        // Set all palettes to the same value (legacy behavior)
        currentPalette = palette;
        backgroundPalette = palette;
        spritePalette = palette;
        ensureBackgroundPaletteSlotsInited();
        backgroundPaletteSlots[0] = palette;
        ensureSpritePaletteSlotsInited();
        spritePaletteSlots[0] = palette;
        // Keep dualPaletteMode = false for backward compatibility
    }
}

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value (legacy mode).
 * Uses the current active palette (single palette mode).
 * @param color The Color enum value.
 * @return The 16-bit color value.
 * @note Function is kept in .cpp to avoid exposing internal state (currentPalette).
 * The compiler may still inline it via LTO (Link Time Optimization).
 */
uint16_t IRAM_ATTR resolveColor(Color color) {
    uint8_t idx = static_cast<uint8_t>(color);

    // Color::Transparent must be handled by the renderer, not here
    if (idx >= PALETTE_SIZE) {
        return 0; // value is irrelevant, should never be drawn
    }

    return currentPalette[idx];
}

/**
 * @brief Enables or disables dual palette mode.
 * @param enable True to enable dual palette mode, false for legacy mode.
 */
void enableDualPaletteMode(bool enable) {
    dualPaletteMode = enable;
}

/**
 * @brief Sets the background palette.
 * @param palette The palette type to use for backgrounds.
 */
void setBackgroundPalette(PaletteType palette) {
    ensureBackgroundPaletteSlotsInited();
    for (const auto& entry : kPalettes) {
        if (entry.type == palette) {
            backgroundPalette = entry.colors;
            backgroundPaletteSlots[0] = entry.colors;
            return;
        }
    }
    backgroundPalette = PALETTE_PR32; // fallback
    backgroundPaletteSlots[0] = PALETTE_PR32;
}

/**
 * @brief Sets the sprite palette.
 * @param palette The palette type to use for sprites.
 */
void setSpritePalette(PaletteType palette) {
    for (const auto& entry : kPalettes) {
        if (entry.type == palette) {
            spritePalette = entry.colors;
            ensureSpritePaletteSlotsInited();
            spritePaletteSlots[0] = entry.colors;
            return;
        }
    }
    spritePalette = PALETTE_PR32; // fallback
    ensureSpritePaletteSlotsInited();
    spritePaletteSlots[0] = PALETTE_PR32;
}

/**
 * @brief Sets a custom background palette.
 * @param palette Pointer to an array of 16 uint16_t RGB565 color values.
 */
void setBackgroundCustomPalette(const uint16_t* palette) {
    if (palette != nullptr) {
        ensureBackgroundPaletteSlotsInited();
        backgroundPalette = palette;
        backgroundPaletteSlots[0] = palette;
    }
}

/**
 * @brief Sets a custom sprite palette.
 * @param palette Pointer to an array of 16 uint16_t RGB565 color values.
 */
void setSpriteCustomPalette(const uint16_t* palette) {
    if (palette != nullptr) {
        spritePalette = palette;
        ensureSpritePaletteSlotsInited();
        spritePaletteSlots[0] = palette;
    }
}

/**
 * @brief Sets both background and sprite palettes at once (convenience function).
 * Automatically enables dual palette mode.
 */
void setDualPalette(PaletteType bgPalette, PaletteType spritePalette) {
    enableDualPaletteMode(true);
    setBackgroundPalette(bgPalette);
    setSpritePalette(spritePalette);
}

/**
 * @brief Sets both background and sprite custom palettes at once (convenience function).
 * Automatically enables dual palette mode.
 */
void setDualCustomPalette(const uint16_t* bgPalette, const uint16_t* spritePal) {
    if (bgPalette != nullptr && spritePal != nullptr) {
        enableDualPaletteMode(true);
        ensureBackgroundPaletteSlotsInited();
        backgroundPalette = bgPalette;
        backgroundPaletteSlots[0] = bgPalette;
        spritePalette = spritePal;
        ensureSpritePaletteSlotsInited();
        spritePaletteSlots[0] = spritePal;
    }
}

void setBackgroundPaletteSlot(uint8_t slotIndex, PaletteType palette) {
    if (slotIndex >= kNumBackgroundPaletteSlots) return;
    ensureBackgroundPaletteSlotsInited();
    for (const auto& entry : kPalettes) {
        if (entry.type == palette) {
            backgroundPaletteSlots[slotIndex] = entry.colors;
            if (slotIndex == 0) backgroundPalette = entry.colors;
            return;
        }
    }
    backgroundPaletteSlots[slotIndex] = PALETTE_PR32;
    if (slotIndex == 0) backgroundPalette = PALETTE_PR32;
}

void setBackgroundCustomPaletteSlot(uint8_t slotIndex, const uint16_t* palette) {
    if (slotIndex >= kNumBackgroundPaletteSlots || palette == nullptr) return;
    ensureBackgroundPaletteSlotsInited();
    backgroundPaletteSlots[slotIndex] = palette;
    if (slotIndex == 0) backgroundPalette = palette;
}

const uint16_t* getBackgroundPaletteSlot(uint8_t slotIndex) {
    ensureBackgroundPaletteSlotsInited();
    if (slotIndex >= kNumBackgroundPaletteSlots)
        return backgroundPaletteSlots[0];
    const uint16_t* p = backgroundPaletteSlots[slotIndex];
    return (p != nullptr) ? p : backgroundPaletteSlots[0];
}

void setSpritePaletteSlot(uint8_t slotIndex, PaletteType palette) {
    if (slotIndex >= kNumSpritePaletteSlots) return;
    ensureSpritePaletteSlotsInited();
    for (const auto& entry : kPalettes) {
        if (entry.type == palette) {
            spritePaletteSlots[slotIndex] = entry.colors;
            if (slotIndex == 0) spritePalette = entry.colors;
            return;
        }
    }
    spritePaletteSlots[slotIndex] = PALETTE_PR32;
    if (slotIndex == 0) spritePalette = PALETTE_PR32;
}

void setSpriteCustomPaletteSlot(uint8_t slotIndex, const uint16_t* palette) {
    if (slotIndex >= kNumSpritePaletteSlots || palette == nullptr) return;
    ensureSpritePaletteSlotsInited();
    spritePaletteSlots[slotIndex] = palette;
    if (slotIndex == 0) spritePalette = palette;
}

const uint16_t* getSpritePaletteSlot(uint8_t slotIndex) {
    ensureSpritePaletteSlotsInited();
    if (slotIndex >= kNumSpritePaletteSlots)
        return spritePaletteSlots[0];
    const uint16_t* p = spritePaletteSlots[slotIndex];
    return (p != nullptr) ? p : spritePaletteSlots[0];
}

uint16_t IRAM_ATTR resolveColorWithPalette(Color color, const uint16_t* palette) {
    uint8_t idx = static_cast<uint8_t>(color);
    if (idx >= PALETTE_SIZE || palette == nullptr)
        return 0;
    return palette[idx];
}

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value with context.
 * Uses the appropriate palette based on the context (dual palette mode) or
 * the current active palette (legacy mode).
 * @param color The Color enum value.
 * @param context The palette context (Background or Sprite).
 * @return The 16-bit color value.
 * @note Function is kept in .cpp to avoid exposing internal state.
 * Branch prediction: dualPaletteMode is typically constant during a frame,
 * making the branch highly predictable for the CPU.
 */
uint16_t IRAM_ATTR resolveColor(Color color, PaletteContext context) {
    uint8_t idx = static_cast<uint8_t>(color);

    // Color::Transparent must be handled by the renderer, not here
    if (idx >= PALETTE_SIZE) {
        return 0; // value is irrelevant, should never be drawn
    }

    // In dual palette mode, use the appropriate palette based on context
    // Branch prediction: dualPaletteMode is typically constant during a frame
    if (dualPaletteMode) {
        const uint16_t* palette = (context == PaletteContext::Background) 
            ? backgroundPalette 
            : spritePalette;
        return palette[idx];
    }
    
    // Legacy mode: use current active palette
    return currentPalette[idx];
}

}
