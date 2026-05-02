# UITouchButton

<Badge type="info" text="Class" />

**Source:** `UITouchButton.h`

**Inherits from:** [UITouchElement](./UITouchElement.md)

## Description

Touch-optimized button widget.

Provides touch input support and the Entity update/draw interface.
Construct with position/size; register with UIManager::addElement for touch routing.
States: Idle, Pressed, Hover
Events: OnDown, OnUp, OnClick

## Inheritance

[UITouchElement](./UITouchElement.md) → `UITouchButton`

## Methods

### `void setColors(Color normal, Color pressed, Color disabled)`

**Description:**

Set button colors

**Parameters:**

- `normal`: Color for normal state
- `pressed`: Color for pressed state
- `disabled`: Color for disabled state

### `Color getNormalColor() const`

**Description:**

Get normal color

**Returns:** Normal state color

### `Color getPressedColor() const`

**Description:**

Get pressed color

**Returns:** Pressed state color

### `Color getDisabledColor() const`

**Description:**

Get disabled color

**Returns:** Disabled state color

### `Color getBorderColor() const`

**Description:**

Get border color

**Returns:** Border color

### `Color getDisabledBorderColor() const`

**Description:**

Get disabled border color

**Returns:** Disabled border color

### `void setFontSize(int size)`

**Description:**

Set font size for text rendering

**Parameters:**

- `size`: Font size (pixels/8, typically 1-4)

### `int getFontSize() const`

**Description:**

Get current font size

**Returns:** Font size

### `void setTextAlignment(TextAlignment align)`

**Description:**

Set text alignment

**Parameters:**

- `align`: Text alignment (LEFT, CENTER, RIGHT)

### `TextAlignment getTextAlignment() const`

**Description:**

Get current text alignment

**Returns:** Text alignment

### `void setOnDown(UIElementVoidCallback callback)`

**Description:**

Set the OnDown callback

**Parameters:**

- `callback`: Function to call when touch goes down

### `void setOnUp(UIElementVoidCallback callback)`

**Description:**

Set the OnUp callback

**Parameters:**

- `callback`: Function to call when touch goes up

### `void setOnClick(UIElementVoidCallback callback)`

**Description:**

Set the OnClick callback

**Parameters:**

- `callback`: Function to call when button is clicked

### `UIElementVoidCallback getOnDown() const`

**Description:**

Get the OnDown callback

**Returns:** The current OnDown callback

### `UIElementVoidCallback getOnUp() const`

**Description:**

Get the OnUp callback

**Returns:** The current OnUp callback

### `UIElementVoidCallback getOnClick() const`

**Description:**

Get the OnClick callback

**Returns:** The current OnClick callback

### `void reset()`

**Description:**

Reset button state

### `void autoSize(uint8_t padding = 4)`

**Description:**

Auto-size button width to fit the current label

**Parameters:**

- `padding`: Extra pixels to add around text (default: 4)

### `void setActive()`

**Description:**

Set active flag

### `void clearActive()`

**Description:**

Clear active flag

### `Color getCurrentColor() const`

**Description:**

Get color based on current state
