# UITouchElement

<Badge type="info" text="Class" />

**Source:** `UITouchElement.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

UIElement with embedded UITouchWidget data for touch interaction.

Embeds widget data directly (x, y, width, height, flags, state)
to avoid memory corruption from overlapping placement new.
Integrates with the UILayout system.

Memory: Owns widget data inline - no external allocation needed.

## Inheritance

[UIElement](./UIElement.md) → `UITouchElement`

## Methods

### `virtual bool processEvent(const pixelroot32::input::TouchEvent& event)`

**Description:**

Process a touch event (polymorphic dispatch from UIManager).

**Returns:** true if this element handled the event for UI consumption semantics

### `virtual uint8_t getWidgetState() const`

**Description:**

Get widget state

**Returns:** Current UIWidgetState

### `virtual bool isPressed() const`

**Description:**

Check if widget is currently pressed

**Returns:** true if state is Pressed

### `virtual bool isEnabled() const`

**Description:**

Check if widget is enabled

**Returns:** true if widget is enabled

### `virtual bool isVisible() const`

**Description:**

Check if widget is visible

**Returns:** true if widget is visible

### `void setWidgetVisible(bool visible)`

**Description:**

Set widget visibility

**Parameters:**

- `visible`: True to make visible

### `void setWidgetEnabled(bool enabled)`

**Description:**

Set widget enabled state

**Parameters:**

- `enabled`: True to enable

### `void setPosition(pixelroot32::math::Scalar newX, pixelroot32::math::Scalar newY)`

**Description:**

Override setPosition to sync widgetData_ with Entity position

**Parameters:**

- `newX`: New X coordinate
- `newY`: New Y coordinate

### `int16_t getX() const`

**Description:**

Get widget x position

**Returns:** X position

### `int16_t getY() const`

**Description:**

Get widget y position

**Returns:** Y position

### `uint16_t getWidgetWidth() const`

**Description:**

Get widget width

**Returns:** Width

### `uint16_t getWidgetHeight() const`

**Description:**

Get widget height

**Returns:** Height

### `UITouchWidget& getWidgetData()`

**Description:**

Get reference to embedded widget data

**Returns:** Reference to UITouchWidget data

### `const UITouchWidget& getWidgetData() const`

**Description:**

Get const reference to embedded widget data

**Returns:** Const reference to UITouchWidget data
