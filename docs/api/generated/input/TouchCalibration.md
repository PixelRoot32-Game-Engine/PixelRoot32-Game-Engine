# TouchCalibration

<Badge type="info" text="Class" />

**Source:** `TouchAdapter.h`

## Description

Calibration parameters for coordinate transformation

Supports:
- Scale factors for X/Y axes
- Offset adjustments
- Display rotation transformation
- Factory presets for common displays

## Properties

| Name | Type | Description |
|------|------|-------------|
| `scaleX` | `float` | X axis scale factor |
| `scaleY` | `float` | Y axis scale factor |
| `offsetX` | `int16_t` | X axis offset |
| `offsetY` | `int16_t` | Y axis offset |
| `displayWidth` | `int16_t` | Display width for clamping |
| `displayHeight` | `int16_t` | Display height for clamping |

## Methods

### `static TouchCalibration fromPreset(DisplayPreset preset)`

**Description:**

Create calibration from preset

**Parameters:**

- `preset`: Display preset

**Returns:** TouchCalibration instance

### `static TouchCalibration forResolution(int16_t width, int16_t height)`

**Description:**

Create calibration for custom resolution

**Parameters:**

- `width`: Display width
- `height`: Display height

**Returns:** TouchCalibration instance

### `TouchPoint transform(int16_t rawX, int16_t rawY, bool pressed, uint8_t id, uint32_t ts) const`

**Description:**

Transform raw coordinates to screen space

**Parameters:**

- `rawX`: Raw X coordinate
- `rawY`: Raw Y coordinate
- `pressed`: Touch pressed state
- `id`: Touch ID
- `ts`: Timestamp

**Returns:** Transformed TouchPoint

### `void setRotation(TouchRotation rot)`

**Description:**

Set rotation mode

### `void applyRotation(int16_t x, int16_t y, int16_t& outX, int16_t& outY) const`

**Description:**

Apply rotation transformation to coordinates

**Parameters:**

- `x`: Input X coordinate
- `y`: Input Y coordinate
- `outX`: Output X coordinate
- `outY`: Output Y coordinate

### `TouchRotation inverted() const`

**Description:**

Invert rotation (for opposite rotation)
