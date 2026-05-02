# XPT2046Adapter

<Badge type="info" text="Class" />

**Source:** `XPT2046Adapter.h`

## Description

XPT2046 SPI touch controller driver

Hardware: XPT2046 (common on TFT touchscreens)
Protocol: SPI — usually shares the TFT bus; ESP32-2432S028R uses a separate GPIO bit-bang bus
          (build with -D XPT2046_USE_GPIO_SPI; pins overridable via XPT2046_GPIO_* macros).
          Use -D XPT2046_GPIO_SWAP_AXES=1 when finger vertical/horizontal tracks the wrong screen axis.
          Use -D XPT2046_GPIO_MIRROR_X=1 when left/right are inverted (horizontal flip in screen space).
          GPIO path order: map → vendor swap → MIRROR_X → CAL_OFFSET_* → clamp (offsets = final nudge).
Sampling: Up to 125Hz
Filtering: Heavy (median + debounce + pressure threshold)

SPI bus MUST be coordinated with display DMA
Requires display-specific calibration

::: tip
SPI bus MUST be coordinated with display DMA
:::

::: tip
Requires display-specific calibration
:::

## Methods

### `static bool initImpl()`

**Description:**

Initialize XPT2046 hardware

**Returns:** true if successful

### `static bool readImpl(TouchPoint* points, uint8_t& count)`

**Description:**

Read touch data from XPT2046

**Parameters:**

- `points`: Output buffer
- `count`: Number of points read

**Returns:** true if successful

### `static void setCalibrationImpl(const TouchCalibration& calib)`

**Description:**

Set calibration parameters

**Parameters:**

- `calib`: Calibration data

### `static int16_t readRawX()`

**Description:**

Read raw X coordinate from XPT2046

**Returns:** Raw ADC value (0-4095)

### `static int16_t readRawY()`

**Description:**

Read raw Y coordinate from XPT2046

**Returns:** Raw ADC value (0-4095)

### `static uint16_t readPressure()`

**Description:**

Read pressure (Z) from XPT2046

**Returns:** Pressure value

### `static int16_t medianFilter(int16_t value, bool isX)`

**Description:**

Median filter implementation

**Parameters:**

- `value`: New sample
- `isX`: True for X coordinate

**Returns:** Filtered value

### `static bool isConnectedImpl()`

**Description:**

Check if controller is connected

**Returns:** true if responding

### `static bool waitForDMADone()`

**Description:**

Wait for DMA operation to complete (SPI bus coordination)

**Returns:** true if bus is available, false if DMA still active

### `static uint32_t millis()`

**Description:**

Platform-specific millis() function
