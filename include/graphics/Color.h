/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once
#include <cstdint>

namespace pixelroot32::graphics {

enum class PaletteType {
    NES,
    GB,
    GBC,
    PICO8,
    PR32
};

/**
 * @brief Context for palette selection in dual palette mode.
 * Determines which palette (background or sprite) is used for color resolution.
 */
enum class PaletteContext {
    Background,  // For backgrounds, tilemaps, and background primitives
    Sprite       // For sprites, characters, and gameplay elements
};

static constexpr uint8_t PALETTE_SIZE = 16;

// Default palette is PR32.
// The Color enum is mapped to the PR32 palette indices (0-15).
// Some legacy colors are aliased to the nearest available color in the 16-color palette.
enum class Color : uint8_t {
    // Standard Colors (PR32 indices)
    Black = 0,
    White = 1,
    Navy = 2,
    Blue = 3,
    Cyan = 4,
    DarkGreen = 5,
    Green = 6,
    LightGreen = 7,
    Yellow = 8,
    Orange = 9,
    LightRed = 10,
    Red = 11,
    DarkRed = 12,
    Purple = 13,
    Magenta = 14,
    Gray = 15,

    // Aliases for compatibility
    DarkBlue = Navy,
    LightBlue = Blue,   // Closest match
    Teal = Cyan,        // Closest match
    Olive = DarkGreen,  // Closest match
    Gold = Yellow,      // Closest match
    Brown = DarkRed,    // Closest match (Maroon)
    Pink = Magenta,
    LightPurple = Magenta,
    Maroon = DarkRed,
    MidGray = Gray,
    LightGray = Gray,
    DarkGray = Gray,
    Silver = Gray,

    // Special

    // Color::Transparent is not a real color.
    // It must be handled by the renderer/blitter and must never be resolved.
    // Using it in drawing primitives results in a no-op.
    Transparent = 255,
    DebugRed = Red,
    DebugGreen = Green,
    DebugBlue = Blue
};


// -----------------------------------------------------------------------------
// Palette system
// -----------------------------------------------------------------------------
//
// The engine supports two modes:
// - Legacy mode (default): Single global palette for all rendering
// - Dual palette mode: Separate palettes for backgrounds and sprites
//
// In legacy mode, setPalette() and setCustomPalette() set a single palette
// that is used for all rendering operations.
//
// In dual palette mode, backgrounds use backgroundPalette and sprites use spritePalette.
// Custom palettes are not copied; the provided palette pointer must remain valid.
//
// Usage Examples:
//
// Legacy mode (single palette):
//   setPalette(PaletteType::GB);  // Sets same palette for backgrounds and sprites
//
// Dual palette mode:
//   enableDualPaletteMode(true);
//   setBackgroundPalette(PaletteType::NES);
//   setSpritePalette(PaletteType::GB);
//   // Or use helper:
//   setDualPalette(PaletteType::NES, PaletteType::GB);
//
// Custom palettes:
//   static const uint16_t MY_BG_PALETTE[16] = { ... };
//   static const uint16_t MY_SPRITE_PALETTE[16] = { ... };
//   enableDualPaletteMode(true);
//   setBackgroundCustomPalette(MY_BG_PALETTE);
//   setSpriteCustomPalette(MY_SPRITE_PALETTE);
//   // Or use helper:
//   setDualCustomPalette(MY_BG_PALETTE, MY_SPRITE_PALETTE);
//

/**
 * @brief Selects the active color palette (legacy mode).
 * Sets both background and sprite palettes to the same value.
 * Does not enable dual palette mode.
 * @param palette The palette to use.
 */
void setPalette(PaletteType palette);

/**
 * @brief Sets a custom color palette (legacy mode).
 * Sets both background and sprite palettes to the same value.
 * Does not enable dual palette mode.
 * @param palette Pointer to an array of 16 uint16_t RGB565 color values.
 *                The array must remain valid (e.g., static or global) as the engine does not copy it.
 */
void setCustomPalette(const uint16_t* palette);

/**
 * @brief Enables or disables dual palette mode.
 * When enabled, backgrounds and sprites use separate palettes.
 * When disabled, a single palette is used for all rendering (legacy mode).
 * @param enable True to enable dual palette mode, false for legacy mode.
 */
void enableDualPaletteMode(bool enable);

/**
 * @brief Sets the background palette (for backgrounds, tilemaps, etc.).
 * @param palette The palette type to use for backgrounds.
 */
void setBackgroundPalette(PaletteType palette);

/**
 * @brief Sets the sprite palette (for sprites, characters, etc.).
 * @param palette The palette type to use for sprites.
 */
void setSpritePalette(PaletteType palette);

/**
 * @brief Sets a custom background palette.
 * @param palette Pointer to an array of 16 uint16_t RGB565 color values.
 *                The array must remain valid (e.g., static or global) as the engine does not copy it.
 */
void setBackgroundCustomPalette(const uint16_t* palette);

/**
 * @brief Sets a custom sprite palette.
 * @param palette Pointer to an array of 16 uint16_t RGB565 color values.
 *                The array must remain valid (e.g., static or global) as the engine does not copy it.
 */
void setSpriteCustomPalette(const uint16_t* palette);

/**
 * @brief Sets both background and sprite palettes at once (convenience function).
 * Automatically enables dual palette mode.
 * @param bgPalette The palette type to use for backgrounds.
 * @param spritePalette The palette type to use for sprites.
 * 
 * @example
 *   setDualPalette(PaletteType::NES, PaletteType::GB);
 */
void setDualPalette(PaletteType bgPalette, PaletteType spritePalette);

/**
 * @brief Sets both background and sprite custom palettes at once (convenience function).
 * Automatically enables dual palette mode.
 * @param bgPalette Pointer to an array of 16 uint16_t RGB565 color values for backgrounds.
 *                  The array must remain valid (e.g., static or global) as the engine does not copy it.
 * @param spritePal Pointer to an array of 16 uint16_t RGB565 color values for sprites.
 *                  The array must remain valid (e.g., static or global) as the engine does not copy it.
 * 
 * @example
 *   static const uint16_t BG_PAL[16] = { ... };
 *   static const uint16_t SPRITE_PAL[16] = { ... };
 *   setDualCustomPalette(BG_PAL, SPRITE_PAL);
 */
void setDualCustomPalette(const uint16_t* bgPalette, const uint16_t* spritePal);

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value (legacy mode).
 * Uses the current active palette (single palette mode).
 * @note Color::Transparent is not a real color and must not be resolved.
 * @param color The Color enum value.
 * @return The 16-bit color value.
 */
uint16_t resolveColor(Color color);

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value with context.
 * Uses the appropriate palette based on the context (dual palette mode) or
 * the current active palette (legacy mode).
 * @note Color::Transparent is not a real color and must not be resolved.
 * @param color The Color enum value.
 * @param context The palette context (Background or Sprite).
 * @return The 16-bit color value.
 */
uint16_t resolveColor(Color color, PaletteContext context);

}
