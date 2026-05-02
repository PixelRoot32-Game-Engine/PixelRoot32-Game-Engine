# UITouchCheckbox

<Badge type="info" text="Class" />

**Source:** `UITouchCheckbox.h`

**Inherits from:** [UITouchElement](./UITouchElement.md)

## Description

Touch-optimized checkbox widget.

Provides touch input support and the Entity update/draw interface.
Construct with position/size; register with UIManager::addElement for touch routing.
States: Idle, Pressed (transient), checked/unchecked
Events: OnChanged (when checked state changes)

## Inheritance

[UITouchElement](./UITouchElement.md) → `UITouchCheckbox`

## Methods

### `void setFontSize(int size)`

**Description:**

Set font size for text rendering

**Parameters:**

- `size`: Font size multiplier

### `int getFontSize() const`

**Description:**

Get current font size

**Returns:** Font size multiplier

### `void setChecked(bool checked)`

**Description:**

Set the checked state

**Parameters:**

- `checked`: True to check, false to uncheck

### `bool isChecked() const`

**Description:**

Get the current checked state

**Returns:** True if checked

### `void toggle()`

**Description:**

Toggle the checked state

### `void setColors(Color normal, Color checked, Color disabled)`

**Description:**

Set checkbox colors

**Parameters:**

- `normal`: Color for normal/unchecked state
- `checked`: Color for checked state
- `disabled`: Color for disabled state

### `Color getNormalColor() const`

**Description:**

Get normal color

**Returns:** Normal state color

### `Color getCheckedColor() const`

**Description:**

Get checked color

**Returns:** Checked state color

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

### `void setOnChanged(UIElementBoolCallback callback)`

**Description:**

Set the OnChanged callback

**Parameters:**

- `callback`: Function to call when checked state changes

### `UIElementBoolCallback getOnChanged() const`

**Description:**

Get the OnChanged callback

**Returns:** The current OnChanged callback

### `void reset()`

**Description:**

Reset checkbox state

### `void setActive()`

**Description:**

Set active flag (visual pressed state)

### `void clearActive()`

**Description:**

Clear active flag

### `Color getCurrentColor() const`

**Description:**

Get color based on current state
