# UIHitTest

<Badge type="info" text="Class" />

**Source:** `UIHitTest.h`

## Description

AABB hit testing for touch UI widgets

Provides hit testing for touch widgets. Iterates through all widgets
in reverse order (top-most first) to find the first hit.
Supports both UITouchWidget* and UITouchElement* (Entity) arrays.

## Methods

### `static bool hitTest(const UITouchWidget& widget, int16_t px, int16_t py)`

**Description:**

Check if a point hits a single widget (AABB)

**Parameters:**

- `widget`: The widget to test
- `px`: X coordinate
- `py`: Y coordinate

**Returns:** true if point is inside widget bounds

### `static bool hitTest(const UITouchElement& element, int16_t px, int16_t py)`

**Description:**

Check if a point hits a UITouchElement (Entity)

**Parameters:**

- `element`: The element to test
- `px`: X coordinate
- `py`: Y coordinate

**Returns:** true if point is inside element bounds

### `static UITouchWidget* findHit(UITouchWidget* widgets[], uint8_t count, int16_t px, int16_t py)`

**Description:**

Find the top-most widget that contains the point

**Parameters:**

- `widgets`: Array of widgets to search
- `count`: Number of widgets in array
- `px`: X coordinate
- `py`: Y coordinate

**Returns:** Pointer to hit widget, or nullptr if no hit

Searches in reverse order (last widget = top-most) for O(1) best case

### `static UITouchElement* findHit(UITouchElement* elements[], uint8_t count, int16_t px, int16_t py)`

**Description:**

Find the top-most element that contains the point (UITouchElement version)

**Parameters:**

- `elements`: Array of elements to search
- `count`: Number of elements in array
- `px`: X coordinate
- `py`: Y coordinate

**Returns:** Pointer to hit element, or nullptr if no hit

### `static const UITouchWidget* findHit(const UITouchWidget* widgets[], uint8_t count, int16_t px, int16_t py)`

**Description:**

Find the top-most widget that contains the point (const version)

**Parameters:**

- `widgets`: Array of widgets to search
- `count`: Number of widgets in array
- `px`: X coordinate
- `py`: Y coordinate

**Returns:** Pointer to hit widget, or nullptr if no hit

### `static const UITouchElement* findHit(const UITouchElement* elements[], uint8_t count, int16_t px, int16_t py)`

**Description:**

Find the top-most element that contains the point (const UITouchElement version)

**Parameters:**

- `elements`: Array of elements to search
- `count`: Number of elements in array
- `px`: X coordinate
- `py`: Y coordinate

**Returns:** Pointer to hit element, or nullptr if no hit
