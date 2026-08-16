# UISpriteRow

<Badge type="info" text="Class" />

**Source:** `UISpriteRow.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

A UI leaf that draws a row of repeated icons whose fill is driven by
       one value — hearts, lives, keys, ammo.

**One element draws every icon.** The obvious alternative — a UILayout
holding N UISprite children — costs one scene entity per icon, and a row of
16 hearts would take two thirds of the 24-entity budget recommended for the
ESP32-C3. Worse, every one of those entities re-enters `Scene::sortEntities()`
(an insertion sort that runs each frame once depth sorting is on) to produce
an order that never changes. A single element sidesteps both.

The engine does not know what a heart is. It knows a `value`, a `capacity`,
and how many units fill one icon:

- `unitsPerIcon == 1` gives binary icons: a lives row, a key count.
- `unitsPerIcon == 2` gives half-steps: value 5 over 3 icons reads
  full, full, half.
- Higher values give finer partial icons (quarter hearts at 4).

State sprites are indexed by fill level: index 0 is empty, index
`unitsPerIcon` is full, and the values between are the partial steps. They
need not share a sprite format.

`setCapacity()` may grow at runtime — a heart container picked up mid-game
widens the element, and any layout holding it sees the new preferred size.
`setIconsPerRow()` wraps onto further rows, which is how a heart bar longer
than the screen stays on screen.


```cpp
UISpriteRow hearts(Vector2(8, 8));
hearts.setStateSprite(0, kHeartEmpty);
hearts.setStateSprite(1, kHeartHalf);
hearts.setStateSprite(2, kHeartFull);
hearts.setUnitsPerIcon(2);      // half-heart granularity
hearts.setIconsPerRow(8);       // wrap like the NES original
hearts.setCapacity(3);          // three containers to start
hearts.setValue(6);             // all full
hearts.setFixedPosition(true);  // immune to camera scroll
scene.addEntity(&hearts);
```

## Inheritance

[UIElement](./UIElement.md) → `UISpriteRow`

## Methods

### `explicit UISpriteRow(pixelroot32::math::Vector2 position)`

**Description:**

Constructs an empty row at `position`.

### `void setStateSprite(int stateIndex, const Sprite& sprite, Color tint = Color::White)`

**Description:**

Assigns the 1bpp sprite drawn at fill level `stateIndex`.

**Parameters:**

- `stateIndex`: 0 = empty .. unitsPerIcon = full. Out-of-range
       indices are ignored rather than clamped, so a caller's off-by-one
       cannot silently overwrite a neighbouring state.
- `sprite`: Sprite to reference. Must outlive this element.
- `tint`: Color the set pixels are drawn in.

### `void setStateSprite(int stateIndex, const Sprite2bpp& sprite, uint8_t paletteSlot = 0)`

**Description:**

Assigns the 2bpp sprite drawn at fill level `stateIndex`.

**Parameters:**

- `stateIndex`: 0 = empty .. unitsPerIcon = full. Out-of-range ignored.
- `sprite`: Sprite to reference. Must outlive this element.
- `paletteSlot`: Sprite palette slot to resolve colors through.

### `void setStateSprite(int stateIndex, const Sprite4bpp& sprite, uint8_t paletteSlot = 0)`

**Description:**

Assigns the 4bpp sprite drawn at fill level `stateIndex`.

**Parameters:**

- `stateIndex`: 0 = empty .. unitsPerIcon = full. Out-of-range ignored.
- `sprite`: Sprite to reference. Must outlive this element.
- `paletteSlot`: Sprite palette slot to resolve colors through.

### `void setUnitsPerIcon(uint8_t units)`

**Description:**

Sets how many units fill a single icon.

**Parameters:**

- `units`: Clamped to [1, kMaxStates - 1].

### `uint8_t getUnitsPerIcon() const`

### `void setCapacity(uint8_t iconCount)`

**Description:**

Sets how many icons are drawn.

### `uint8_t getCapacity() const`

### `void setValue(int filledUnits)`

**Description:**

Sets the filled amount, in units.

### `int getValue() const`

### `void setSpacing(int pixels)`

### `int getSpacing() const`

### `void setRowSpacing(int pixels)`

### `int getRowSpacing() const`

### `void setIconsPerRow(uint8_t iconsPerRow)`

**Description:**

Sets how many icons fit on a row before wrapping.

**Parameters:**

- `iconsPerRow`: 0 disables wrapping (a single unbounded row).

### `uint8_t getIconsPerRow() const`

### `int stateIndexAt(int iconIndex) const`

**Description:**

Fill state of the icon at `iconIndex`.

**Parameters:**

- `iconIndex`: Icon position, 0-based.

**Returns:** 0 (empty) .. unitsPerIcon (full). Returns 0 for an index outside
        [0, capacity).

Exposed because a game often needs the same answer the row draws with —
to flash the icon that just changed, or park a cursor on it.

### `void iconOffsetAt(int iconIndex, int& outOffsetX, int& outOffsetY) const`

**Description:**

Offset of the icon at `iconIndex` relative to this element's
       position, accounting for spacing and wrapping.

**Parameters:**

- `iconIndex`: Icon position, 0-based.
- `outOffsetX`: Receives the horizontal offset in pixels.
- `outOffsetY`: Receives the vertical offset in pixels.

Both outputs are set to 0 for an index outside [0, capacity).

### `void update(unsigned long deltaTime)`

**Description:**

No-op. The row has no internal clock; it renders whatever
       setValue() last said.

**Parameters:**

- `deltaTime`: Ignored.

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws every icon, honoring isVisible and fixedPosition.

**Parameters:**

- `renderer`: Reference to the renderer.

### `int iconWidth() const`

**Description:**

Widest sprite across the configured states.

### `void recalcSize()`

**Description:**

Recomputes width/height from icon size, capacity and wrapping.
