# UISprite

<Badge type="info" text="Class" />

**Source:** `UISprite.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

A UI leaf that draws a single sprite.

The UI system could previously draw text and rectangles only, so anything
with an icon — an item slot in a menu, a dialog portrait, a button glyph, a
HUD resource — had to be drawn by hand in a `Scene::draw()` override,
outside the entity tree. That costs three things this element gets for
free: `setVisible()`, placement by a UILayout, and `setFixedPosition()`,
which bypasses the camera offset so a HUD stays put while the world
scrolls.

Accepts all three sprite formats (`Sprite`, `Sprite2bpp`, `Sprite4bpp`) via
UISpriteRef, and adopts the sprite's dimensions as its own so layouts can
size it. It holds no clock: animating means setting a different sprite,
which is the game's decision, not the element's.


```cpp
UISprite icon(Vector2(8, 8));
icon.setSprite(kKeyIcon4bpp);
icon.setFixedPosition(true);   // stays put while the camera scrolls
scene.addEntity(&icon);
```

## Inheritance

[UIElement](./UIElement.md) → `UISprite`

## Methods

### `explicit UISprite(pixelroot32::math::Vector2 position)`

**Description:**

Constructs an empty sprite element at `position`.

### `void setSprite(const Sprite& sprite, Color tint = Color::White)`

**Description:**

Sets a 1bpp sprite, drawn in `tint`.

**Parameters:**

- `sprite`: Sprite to reference. Must outlive this element.
- `tint`: Color the set pixels are drawn in.

### `void setSprite(const Sprite2bpp& sprite, uint8_t paletteSlot = 0)`

**Description:**

Sets a 2bpp sprite, drawn through `paletteSlot`.

**Parameters:**

- `sprite`: Sprite to reference. Must outlive this element.
- `paletteSlot`: Sprite palette slot to resolve colors through.

### `void setSprite(const Sprite4bpp& sprite, uint8_t paletteSlot = 0)`

**Description:**

Sets a 4bpp sprite, drawn through `paletteSlot`.

**Parameters:**

- `sprite`: Sprite to reference. Must outlive this element.
- `paletteSlot`: Sprite palette slot to resolve colors through.

### `void clearSprite()`

**Description:**

Removes the sprite, returning the element to 0x0 and drawing
       nothing.

### `UISpriteFormat getFormat() const`

### `bool hasSprite() const`

### `Color getTint() const`

### `uint8_t getPaletteSlot() const`

### `void setFlipX(bool flip)`

**Description:**

Mirrors the sprite horizontally when drawn.

### `bool getFlipX() const`

### `void update(unsigned long deltaTime)`

**Description:**

No-op. A sprite leaf has no internal animation clock.

**Parameters:**

- `deltaTime`: Ignored.

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws the sprite, honoring isVisible and fixedPosition.

**Parameters:**

- `renderer`: Reference to the renderer.

### `void recalcSize()`

**Description:**

Re-reads width/height from the current sprite.
