# TouchAdapterBase

<Badge type="info" text="Class" />

**Source:** `TouchAdapter.h`

## Description

Base class requirements for touch adapters (conceptual)

This defines the interface that ALL touch adapters must implement.
Uses template static dispatch instead of virtual functions for ESP32.

Adapter Concrete adapter type (XPT2046Adapter or GT911Adapter)

## Methods

### `static bool init()`

**Description:**

Initialize the touch controller

**Returns:** true if initialization successful

### `static bool read(TouchPoint* points, uint8_t& count)`

**Description:**

Read touch points from controller

**Parameters:**

- `points`: Output buffer for touch points
- `count`: Number of touch points read

**Returns:** true if read successful

### `static void setCalibration(const TouchCalibration& calib)`

**Description:**

Set calibration parameters

**Parameters:**

- `calib`: Calibration data

### `static bool isConnected()`

**Description:**

Check if controller is connected

**Returns:** true if controller responds
