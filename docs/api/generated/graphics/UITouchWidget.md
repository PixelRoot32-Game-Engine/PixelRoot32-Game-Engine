# UITouchWidget

<Badge type="info" text="Struct" />

**Source:** `UITouchWidget.h`

## Description

Base touch widget structure

## Properties

| Name | Type | Description |
|------|------|-------------|
| `type` | `UIWidgetType` | Widget type |
| `state` | `UIWidgetState` | Current state |
| `flags` | `UIWidgetFlags` | Widget flags |
| `id` | `uint8_t` | Unique widget ID |
| `x` | `int16_t` | X position (top-left) |
| `y` | `int16_t` | Y position (top-left) |
| `width` | `uint16_t` | Widget width |
| `height` | `uint16_t` | Widget height |

## Methods

### `, id(0)`

### `, x(0)`

### `, y(0)`

### `, width(0)`

### `, height(0)`

### `, id(widgetId)`

### `, x(xPos)`

### `, y(yPos)`

### `, width(w)`

### `, height(h)`

### `bool isEnabled() const`

**Description:**

Check if widget is enabled

**Returns:** true if Enabled flag is set

### `bool isVisible() const`

**Description:**

Check if widget is visible

**Returns:** true if Visible flag is set

### `bool isActive() const`

**Description:**

Check if widget is active (being interacted with)

**Returns:** true if Active flag is set

### `bool isConsumed() const`

**Description:**

Check if event was consumed

**Returns:** true if Consumed flag is set

### `void setEnabled(bool enabled)`

**Description:**

Enable or disable the widget

**Parameters:**

- `enabled`: True to enable

### `void setVisible(bool visible)`

**Description:**

Show or hide the widget

**Parameters:**

- `visible`: True to show

### `void consume()`

**Description:**

Mark event as consumed

### `void clearConsume()`

**Description:**

Clear consumed flag for next frame

### `bool contains(int16_t px, int16_t py) const`

**Description:**

Check if point is inside widget bounds (AABB)
