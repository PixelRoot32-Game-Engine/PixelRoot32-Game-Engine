# UICheckBox

<Badge type="info" text="Class" />

**Source:** `UICheckbox.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

A clickable checkbox UI element.

Supports both physical (keyboard/gamepad) and touch input.
Can trigger a callback function when its state changes.

## Inheritance

[UIElement](./UIElement.md) → `UICheckBox`

## Methods

### `void setStyle(Color textCol, Color bgCol, bool drawBg = false)`

**Description:**

Configures the checkbox's visual style.

**Parameters:**

- `textCol`: Color of the text.
- `bgCol`: Color of the background.
- `drawBg`: Whether to draw the background rectangle.

### `void setChecked(bool checked)`

**Description:**

Sets the checked state.

**Parameters:**

- `checked`: True if checked.

### `bool isChecked() const`

**Description:**

Checks if the checkbox is currently checked.

**Returns:** true if checked.

### `void setSelected(bool selected)`

**Description:**

Sets the selection state (e.g., focused via D-pad).

**Parameters:**

- `selected`: True if selected.

### `bool getSelected() const`

**Description:**

Checks if the checkbox is currently selected.

**Returns:** true if selected.

### `void toggle()`

**Description:**

Toggles the checkbox state.

### `bool isPointInside(int px, int py) const`

**Description:**

Internal helper to check if a point is inside the checkbox's bounds.

**Parameters:**

- `px`: Point X coordinate.
- `py`: Point Y coordinate.

**Returns:** true if point is inside.
