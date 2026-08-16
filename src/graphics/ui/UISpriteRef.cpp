#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UISpriteRef.h"

namespace pixelroot32::graphics::ui {

bool uiSpriteRefIsSet(const UISpriteRef& ref) {
    switch (ref.format) {
        case UISpriteFormat::Mono: return ref.storage.mono != nullptr;
        case UISpriteFormat::Bpp2: return ref.storage.bpp2 != nullptr;
        case UISpriteFormat::Bpp4: return ref.storage.bpp4 != nullptr;
        case UISpriteFormat::None:
        default:                   return false;
    }
}

int uiSpriteRefWidth(const UISpriteRef& ref) {
    if (!uiSpriteRefIsSet(ref)) return 0;

    switch (ref.format) {
        case UISpriteFormat::Mono: return static_cast<int>(ref.storage.mono->width);
        case UISpriteFormat::Bpp2: return static_cast<int>(ref.storage.bpp2->width);
        case UISpriteFormat::Bpp4: return static_cast<int>(ref.storage.bpp4->width);
        default:                   return 0;
    }
}

int uiSpriteRefHeight(const UISpriteRef& ref) {
    if (!uiSpriteRefIsSet(ref)) return 0;

    switch (ref.format) {
        case UISpriteFormat::Mono: return static_cast<int>(ref.storage.mono->height);
        case UISpriteFormat::Bpp2: return static_cast<int>(ref.storage.bpp2->height);
        case UISpriteFormat::Bpp4: return static_cast<int>(ref.storage.bpp4->height);
        default:                   return 0;
    }
}

void drawUISpriteRef(Renderer& renderer, const UISpriteRef& ref, int x, int y, bool flipX) {
    if (!uiSpriteRefIsSet(ref)) return;

    switch (ref.format) {
        case UISpriteFormat::Mono:
            renderer.drawSprite(*ref.storage.mono, x, y, ref.tint, flipX);
            break;
        case UISpriteFormat::Bpp2:
            renderer.drawSprite(*ref.storage.bpp2, x, y, ref.paletteSlot, flipX);
            break;
        case UISpriteFormat::Bpp4:
            renderer.drawSprite(*ref.storage.bpp4, x, y, ref.paletteSlot, flipX);
            break;
        default:
            break;
    }
}

}  // namespace pixelroot32::graphics::ui

#endif  // PIXELROOT32_ENABLE_UI_SYSTEM
