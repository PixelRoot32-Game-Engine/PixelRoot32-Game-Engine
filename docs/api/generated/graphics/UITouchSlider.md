# UITouchSlider

<Badge type="info" text="Class" />

**Source:** `UITouchSlider.h`

**Inherits from:** [UITouchElement](./UITouchElement.md)

## Description

Touch-optimized slider widget.

Provides touch input support and the Entity update/draw interface.
Construct with position/size; register with UIManager::addElement for touch routing.
Value range: 0-100
States: Idle, Dragging
Events: OnValueChanged, OnDragStart, OnDragEnd

## Inheritance

[UITouchElement](./UITouchElement.md) → `UITouchSlider`

## Methods

### `explicit UITouchSlider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t initialValue)`

**Description:**

Construct a new UITouchSlider

**Parameters:**

- `x`: X position
- `y`: Y position
- `w`: Width
- `h`: Height
- `initialValue`: Initial value (0-100)

### `void setColors(Color track, Color thumb)`

**Description:**

Set track and thumb colors

**Parameters:**

- `track`: Color for track
- `thumb`: Color for thumb

### `void setOnValueChanged(SliderCallback callback)`

**Description:**

Set the OnValueChanged callback

**Parameters:**

- `callback`: Function to call when value changes (receives new value)

### `void setOnDragStart(SliderCallback callback)`

**Description:**

Set the OnDragStart callback

**Parameters:**

- `callback`: Function to call when drag starts

### `void setOnDragEnd(SliderCallback callback)`

**Description:**

Set the OnDragEnd callback

**Parameters:**

- `callback`: Function to call when drag ends

### `SliderCallback getOnValueChanged() const`

**Description:**

Get the OnValueChanged callback

**Returns:** The current OnValueChanged callback

### `SliderCallback getOnDragStart() const`

**Description:**

Get the OnDragStart callback

**Returns:** The current OnDragStart callback

### `SliderCallback getOnDragEnd() const`

**Description:**

Get the OnDragEnd callback

**Returns:** The current OnDragEnd callback

### `Color getTrackColor() const`

**Description:**

Get track color

**Returns:** Current track color

### `Color getThumbColor() const`

**Description:**

Get thumb color

**Returns:** Current thumb color

### `Color getDisabledColor() const`

**Description:**

Get disabled color

**Returns:** Current disabled color

### `uint8_t getValue() const`

**Description:**

Get the current value

**Returns:** Current value (0-100)

### `void setValue(uint8_t newValue)`

**Description:**

Set the value

**Parameters:**

- `newValue`: New value (0-100)

### `uint8_t getPreviousValue() const`

**Description:**

Get the previous value

**Returns:** Previous value (0-100)

### `bool hasValueChanged() const`

**Description:**

Check if value changed since last frame

**Returns:** true if value changed

### `void reset()`

**Description:**

Reset slider state

### `void updateValueFromPosition(int16_t xPos)`

**Description:**

Update value based on X position

**Parameters:**

- `xPos`: X position

### `void setActive()`

**Description:**

Set active flag

### `void clearActive()`

**Description:**

Clear active flag
