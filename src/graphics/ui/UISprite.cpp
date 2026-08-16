#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UISprite.h"

namespace pixelroot32::graphics::ui {

UISprite::UISprite(pixelroot32::math::Vector2 position)
    : UIElement(position, 0, 0, UIElementType::GENERIC) {
}

UISprite::UISprite(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y)
    : UIElement(x, y, 0, 0, UIElementType::GENERIC) {
}

void UISprite::setSprite(const Sprite& spriteRef, Color tint) {
    sprite = makeUISpriteRef(spriteRef, tint);
    recalcSize();
}

void UISprite::setSprite(const Sprite2bpp& spriteRef, uint8_t paletteSlot) {
    sprite = makeUISpriteRef(spriteRef, paletteSlot);
    recalcSize();
}

void UISprite::setSprite(const Sprite4bpp& spriteRef, uint8_t paletteSlot) {
    sprite = makeUISpriteRef(spriteRef, paletteSlot);
    recalcSize();
}

void UISprite::clearSprite() {
    sprite = UISpriteRef{};
    recalcSize();
}

void UISprite::recalcSize() {
    width = uiSpriteRefWidth(sprite);
    height = uiSpriteRefHeight(sprite);
}

void UISprite::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void UISprite::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!isVisible) return;

    // Return before touching the renderer's bypass state: an empty slot must
    // be indistinguishable from one that was never drawn.
    if (!uiSpriteRefIsSet(sprite)) return;

    bool oldBypass = renderer.isOffsetBypassEnabled();
    if (fixedPosition) {
        renderer.setOffsetBypass(true);
    }

    drawUISpriteRef(renderer, sprite,
                    static_cast<int>(position.x),
                    static_cast<int>(position.y),
                    flipX);

    if (fixedPosition) {
        renderer.setOffsetBypass(oldBypass);
    }
}

}  // namespace pixelroot32::graphics::ui

#endif  // PIXELROOT32_ENABLE_UI_SYSTEM
