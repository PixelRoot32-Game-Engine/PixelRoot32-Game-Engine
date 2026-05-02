# TouchFactory

<Badge type="info" text="Class" />

**Source:** `TouchFactory.h`

## Description

Factory for creating touch system configurations

Provides convenient factory methods to create TouchManager
with common display presets and configurations.

Usage:
  auto manager = TouchFactory::createForILI9341();
  manager->init();

## Methods

### `static TouchManager createForILI9341()`

**Description:**

Create TouchManager for ILI9341 display (320x240)

**Returns:** Initialized TouchManager

### `TouchManager manager(320, 240)`

### `static TouchManager createForST7789_240x320()`

**Description:**

Create TouchManager for ST7789 display (240x320 portrait)

**Returns:** Initialized TouchManager

### `TouchManager manager(240, 320)`

### `static TouchManager createForST7789_240x240()`

**Description:**

Create TouchManager for ST7789 display (240x240 round)

**Returns:** Initialized TouchManager

### `TouchManager manager(240, 240)`

### `static TouchManager createForST7735_128x160()`

**Description:**

Create TouchManager for ST7735 display (128x160)

**Returns:** Initialized TouchManager

### `TouchManager manager(128, 160)`

### `static TouchManager createForST7735_128x128()`

**Description:**

Create TouchManager for ST7735 display (128x128)

**Returns:** Initialized TouchManager

### `TouchManager manager(128, 128)`

### `static TouchManager createForILI9488()`

**Description:**

Create TouchManager for ILI9488 display (320x480)

**Returns:** Initialized TouchManager

### `TouchManager manager(320, 480)`

### `static TouchManager createForGC9A01()`

**Description:**

Create TouchManager for GC9A01 display (240x240 round)

**Returns:** Initialized TouchManager

### `static TouchManager createForResolution(uint16_t width, uint16_t height)`

**Description:**

Create TouchManager for custom resolution

**Parameters:**

- `width`: Display width
- `height`: Display height

**Returns:** Initialized TouchManager

### `TouchManager manager(width, height)`

### `static TouchManager createFromPreset(DisplayPreset preset)`

**Description:**

Create TouchManager from DisplayPreset enum

**Parameters:**

- `preset`: Display preset

**Returns:** Initialized TouchManager

### `TouchManager manager(calib.displayWidth, calib.displayHeight)`

### `static DisplayPreset detectPreset(uint16_t width, uint16_t height)`

**Description:**

Get display preset from width/height

**Parameters:**

- `width`: Display width
- `height`: Display height

**Returns:** Matching DisplayPreset or Custom if no match
