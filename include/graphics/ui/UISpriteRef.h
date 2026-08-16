/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/Renderer.h"
#include "graphics/Color.h"

#include <cstdint>

/**
 * @file UISpriteRef.h
 * @brief Format-tagged reference to a sprite, shared by every UI element that
 *        draws one.
 *
 * The engine ships three unrelated sprite descriptors with three different
 * draw signatures: `Sprite` (1bpp, takes a Color), `Sprite2bpp` and
 * `Sprite4bpp` (both take a palette slot). A UI element that wants to draw
 * "a sprite" has to handle all three.
 *
 * `UISpriteRef` is the tagged union that lets it do so once. UISprite holds
 * one; UISpriteRow holds one per icon state. Both call drawUISpriteRef(),
 * so the format switch exists in exactly one place — the same type-erasure
 * choice the gameplay framework made over templating, and for the same
 * reason: one copy in flash instead of one per instantiation.
 *
 * A ref never owns the sprite. Sprites are `constexpr`/flash-resident asset
 * data that outlives any UI element pointing at it.
 */

namespace pixelroot32::graphics::ui {

/**
 * @enum UISpriteFormat
 * @brief Which member of a UISpriteRef's storage is live.
 */
enum class UISpriteFormat : uint8_t {
    None = 0,  ///< No sprite set; the ref draws nothing and measures 0x0.
    Mono,      ///< 1bpp `Sprite`, drawn with `tint`.
    Bpp2,      ///< `Sprite2bpp`, drawn with `paletteSlot`.
    Bpp4       ///< `Sprite4bpp`, drawn with `paletteSlot`.
};

/**
 * @struct UISpriteRef
 * @brief Non-owning, format-tagged pointer to one sprite plus its draw
 *        parameters.
 */
struct UISpriteRef {
    /// Storage for exactly one of the three descriptors; `format` says which.
    union Storage {
        const Sprite* mono;
        const Sprite2bpp* bpp2;
        const Sprite4bpp* bpp4;
    };

    Storage storage{nullptr};                       ///< The referenced sprite.
    UISpriteFormat format = UISpriteFormat::None;   ///< Live member of `storage`.
    Color tint = Color::White;                      ///< Mono only; ignored otherwise.
    uint8_t paletteSlot = 0;                        ///< 2bpp/4bpp only; ignored otherwise.
};

/// Builds a ref to a 1bpp sprite drawn in `tint`.
inline UISpriteRef makeUISpriteRef(const Sprite& sprite, Color tint = Color::White) {
    UISpriteRef ref;
    ref.storage.mono = &sprite;
    ref.format = UISpriteFormat::Mono;
    ref.tint = tint;
    return ref;
}

/// Builds a ref to a 2bpp sprite drawn through `paletteSlot`.
inline UISpriteRef makeUISpriteRef(const Sprite2bpp& sprite, uint8_t paletteSlot = 0) {
    UISpriteRef ref;
    ref.storage.bpp2 = &sprite;
    ref.format = UISpriteFormat::Bpp2;
    ref.paletteSlot = paletteSlot;
    return ref;
}

/// Builds a ref to a 4bpp sprite drawn through `paletteSlot`.
inline UISpriteRef makeUISpriteRef(const Sprite4bpp& sprite, uint8_t paletteSlot = 0) {
    UISpriteRef ref;
    ref.storage.bpp4 = &sprite;
    ref.format = UISpriteFormat::Bpp4;
    ref.paletteSlot = paletteSlot;
    return ref;
}

/**
 * @brief True if the ref points at a real sprite.
 *
 * Also returns false for a ref whose format is set but whose pointer is null,
 * so callers get one check instead of two.
 */
bool uiSpriteRefIsSet(const UISpriteRef& ref);

/// Width of the referenced sprite in pixels, or 0 if none is set.
int uiSpriteRefWidth(const UISpriteRef& ref);

/// Height of the referenced sprite in pixels, or 0 if none is set.
int uiSpriteRefHeight(const UISpriteRef& ref);

/**
 * @brief Draws the referenced sprite at (x, y), dispatching on its format.
 *
 * A no-op when the ref is unset. This is the single place the three sprite
 * formats are switched over.
 */
void drawUISpriteRef(Renderer& renderer, const UISpriteRef& ref, int x, int y, bool flipX);

}  // namespace pixelroot32::graphics::ui

#endif  // PIXELROOT32_ENABLE_UI_SYSTEM
