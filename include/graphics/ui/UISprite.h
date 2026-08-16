/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "UIElement.h"
#include "UISpriteRef.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UISprite
 * @brief A UI leaf that draws a single sprite.
 *
 * Inherits from UIElement.
 *
 * The UI system could previously draw text and rectangles only, so anything
 * with an icon — an item slot in a menu, a dialog portrait, a button glyph, a
 * HUD resource — had to be drawn by hand in a `Scene::draw()` override,
 * outside the entity tree. That costs three things this element gets for
 * free: `setVisible()`, placement by a UILayout, and `setFixedPosition()`,
 * which bypasses the camera offset so a HUD stays put while the world
 * scrolls.
 *
 * Accepts all three sprite formats (`Sprite`, `Sprite2bpp`, `Sprite4bpp`) via
 * UISpriteRef, and adopts the sprite's dimensions as its own so layouts can
 * size it. It holds no clock: animating means setting a different sprite,
 * which is the game's decision, not the element's.
 *
 * @code
 * UISprite icon(Vector2(8, 8));
 * icon.setSprite(kKeyIcon4bpp);
 * icon.setFixedPosition(true);   // stays put while the camera scrolls
 * scene.addEntity(&icon);
 * @endcode
 */
class UISprite : public UIElement {
public:
    /**
     * @brief Constructs an empty sprite element at `position`.
     *
     * Starts with no sprite and a 0x0 size; drawing is a no-op until
     * setSprite() is called.
     */
    explicit UISprite(pixelroot32::math::Vector2 position);

    /**
     * @brief Constructs an empty sprite element at (x, y).
     */
    UISprite(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y);

    virtual ~UISprite() = default;

    /**
     * @brief Sets a 1bpp sprite, drawn in `tint`.
     * @param sprite Sprite to reference. Must outlive this element.
     * @param tint Color the set pixels are drawn in.
     */
    void setSprite(const Sprite& sprite, Color tint = Color::White);

    /**
     * @brief Sets a 2bpp sprite, drawn through `paletteSlot`.
     * @param sprite Sprite to reference. Must outlive this element.
     * @param paletteSlot Sprite palette slot to resolve colors through.
     */
    void setSprite(const Sprite2bpp& sprite, uint8_t paletteSlot = 0);

    /**
     * @brief Sets a 4bpp sprite, drawn through `paletteSlot`.
     * @param sprite Sprite to reference. Must outlive this element.
     * @param paletteSlot Sprite palette slot to resolve colors through.
     */
    void setSprite(const Sprite4bpp& sprite, uint8_t paletteSlot = 0);

    /**
     * @brief Removes the sprite, returning the element to 0x0 and drawing
     *        nothing.
     *
     * Useful for an item slot that becomes empty without leaving the layout.
     */
    void clearSprite();

    /// The format currently set, or UISpriteFormat::None.
    UISpriteFormat getFormat() const { return sprite.format; }

    /// True if a sprite is set and its pointer is non-null.
    bool hasSprite() const { return uiSpriteRefIsSet(sprite); }

    /// Tint used for a 1bpp sprite; meaningless for the other formats.
    Color getTint() const { return sprite.tint; }

    /// Palette slot used for 2bpp/4bpp; meaningless for 1bpp.
    uint8_t getPaletteSlot() const { return sprite.paletteSlot; }

    /**
     * @brief Mirrors the sprite horizontally when drawn.
     *
     * Survives a later setSprite(): an explicit flip is a property of the
     * element (a left-facing slot), not of the sprite currently in it.
     */
    void setFlipX(bool flip) { flipX = flip; }

    /// Whether the sprite is drawn mirrored horizontally.
    bool getFlipX() const { return flipX; }

    /**
     * @brief No-op. A sprite leaf has no internal animation clock.
     * @param deltaTime Ignored.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws the sprite, honoring isVisible and fixedPosition.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    UISpriteRef sprite;   ///< The referenced sprite plus its draw parameters.
    bool flipX = false;   ///< Horizontal mirror flag.

    /**
     * @brief Re-reads width/height from the current sprite.
     *
     * Mirrors UILabel::recalcSize(): the element's size is derived from its
     * content, so layouts asking for a preferred size get a real answer.
     */
    void recalcSize();
};

}  // namespace pixelroot32::graphics::ui

#endif  // PIXELROOT32_ENABLE_UI_SYSTEM
