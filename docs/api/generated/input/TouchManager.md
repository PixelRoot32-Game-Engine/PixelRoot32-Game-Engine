# TouchManager

<Badge type="info" text="Class" />

**Source:** `TouchManager.h`

## Description

Touch event aggregation layer

This class is COMPLETELY INDEPENDENT of the touch adapter.
It receives normalized TouchPoints and provides:
- Circular buffer of recent touch points
- Active touch count query
- Coordinate mapping integration point for InputManager

The pipeline is:
[XPT2046/GT911 Adapter] → [TouchManager] → [UI System]

## Properties

| Name | Type | Description |
|------|------|-------------|
| `constexpr` | `static` | Fixed-size circular buffer |

## Methods

### `bool init()`

**Description:**

Initialize touch system

**Returns:** true if initialization successful

### `void update(unsigned long dt)`

**Description:**

Update touch state - call this every frame

**Parameters:**

- `dt`: Delta time in milliseconds

### `uint8_t getTouchPoints(TouchPoint* points) const`

**Description:**

Get all active touch points

**Parameters:**

- `points`: Output buffer

**Returns:** Number of active touch points

### `uint8_t getActiveCount() const`

**Description:**

Get number of active touch points

**Returns:** Count of currently pressed touches

### `bool isTouchActive() const`

**Description:**

Check if any touch is active

**Returns:** true if any touch is pressed

### `const TouchPoint& getTouchPoint(uint8_t index) const`

**Description:**

Get touch point at specific index

**Parameters:**

- `index`: Index (0 to getActiveCount() - 1)

**Returns:** TouchPoint reference

### `bool isTouchedInArea(int16_t x, int16_t y, int16_t radius) const`

**Description:**

Check if specific area is touched

**Parameters:**

- `x`: X coordinate
- `y`: Y coordinate
- `radius`: Touch hit radius

**Returns:** true if touch detected in area

### `void setCalibration(const TouchCalibration& calib)`

**Description:**

Set calibration parameters

**Parameters:**

- `calib`: Calibration data

### `bool isConnected() const`

**Description:**

Check if touch controller is connected

**Returns:** true if responding

### `void addTouchPoint(const TouchPoint& point)`

**Description:**

Add touch point to circular buffer

**Parameters:**

- `point`: Touch point to add

### `void clearBuffer()`

**Description:**

Clear all touch points

### `static int16_t distance(int16_t x1, int16_t y1, int16_t x2, int16_t y2)`

**Description:**

Calculate distance between two points

**Returns:** Distance in pixels
