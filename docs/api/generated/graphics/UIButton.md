# UIButton

<Badge type="info" text="Class" />

**Source:** `UIButton.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

A clickable button UI element.

Supports both physical (keyboard/gamepad) and touch input.
Can trigger a callback function when pressed.

## Inheritance

[UIElement](./UIElement.md) → `UIButton`

## Methods

### `void setStyle(Color textCol, Color bgCol, bool drawBg)`

**Description:**

Configures the button's visual style.

**Parameters:**

- `textCol`: Color of the text.
- `bgCol`: Color of the background.
- `drawBg`: Whether to draw the background rectangle.

### `void setSelected(bool selected)`

**Description:**

Sets the selection state (e.g., focused via D-pad).

**Parameters:**

- `selected`: True if selected.

### `bool getSelected() const`

**Description:**

Checks if the button is currently selected.

**Returns:** true if selected.

### `void press()`

**Description:**

Manually triggers the button's action.

### `bool isPointInside(int px, int py) const`

**Description:**

Internal helper to check if a point is inside the button's bounds.

**Parameters:**

- `px`: Point X coordinate.
- `py`: Point Y coordinate.

**Returns:** true if point is inside.
