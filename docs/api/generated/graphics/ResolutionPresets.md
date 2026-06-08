# ResolutionPresets

<Badge type="info" text="Class" />

**Source:** `ResolutionPresets.h`

## Description

Factory for creating DisplayConfig from resolution presets.

Simplifies display setup on memory-constrained targets by providing
a single create() call that sets logical dimensions, physical dimensions,
and rotation together.

## Methods

### `static DisplayConfig create(ResolutionPreset preset, DisplayType type = ST7789, uint16_t physicalW = 240, uint16_t physicalH = 240)`

**Description:**

Creates a DisplayConfig from a preset and hardware target.

**Parameters:**

- `preset`: Desired resolution preset.
- `type`: Physical display type (default ST7789).
- `physicalW`: Physical width in pixels (default 240).
- `physicalH`: Physical height in pixels (default 240).

**Returns:** Configured DisplayConfig with correct logical/physical dimensions.

### `case RES_240x240: return DisplayConfig(type, 0, physicalW, physicalH, physicalW, physicalH)`

### `case RES_160x160: return DisplayConfig(type, 0, physicalW, physicalH, 160, 160)`

### `case RES_128x128: return DisplayConfig(type, 0, physicalW, physicalH, 128, 128)`

### `case RES_96x96: return DisplayConfig(type, 0, physicalW, physicalH, 96, 96)`

### `default: return DisplayConfig(type, 0, physicalW, physicalH, physicalW, physicalH)`
