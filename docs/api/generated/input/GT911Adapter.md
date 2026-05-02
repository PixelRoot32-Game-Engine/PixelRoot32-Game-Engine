# GT911Adapter

<Badge type="info" text="Class" />

**Source:** `GT911Adapter.h`

## Description

GT911 I2C touch controller driver

Hardware: GT911 (Goodix) - common on higher-quality touch panels
Protocol: I2C (independent bus from display SPI)
Sampling: 100Hz+
Filtering: Minimal (passthrough - controller does processing)

I2C bus is independent - no DMA coordination needed
Supports 5-point multi-touch

::: tip
I2C bus is independent - no DMA coordination needed
:::

::: tip
Supports 5-point multi-touch
:::

## Methods

### `static bool initImpl()`

**Description:**

Initialize GT911 hardware

**Returns:** true if successful

### `static bool readImpl(TouchPoint* points, uint8_t& count)`

**Description:**

Read touch data from GT911

**Parameters:**

- `points`: Output buffer
- `count`: Number of points read

**Returns:** true if successful

### `static void setCalibrationImpl(const TouchCalibration& calib)`

**Description:**

Set calibration parameters

**Parameters:**

- `calib`: Calibration data

### `static bool isConnectedImpl()`

**Description:**

Check if controller is connected

**Returns:** true if responding

### `static uint8_t readRegister(uint8_t addr)`

**Description:**

Read a single register from GT911

**Parameters:**

- `addr`: Register address

**Returns:** Register value

### `static int16_t readCoordinate(uint8_t addr)`

**Description:**

Read a 16-bit coordinate from GT911

**Parameters:**

- `addr`: Register address (LSB first)

**Returns:** Coordinate value

### `static void writeRegister(uint8_t addr, uint8_t value)`

**Description:**

Write a register to GT911

**Parameters:**

- `addr`: Register address
- `value`: Value to write

### `static uint32_t millis()`

**Description:**

Platform-specific millis() function
