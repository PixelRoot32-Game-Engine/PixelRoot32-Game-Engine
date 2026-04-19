# Migration Guide: v1.3.0 - Display Bottleneck Optimization

## Overview

Version 1.3.0 introduces display bottleneck optimization through partial screen updates and configurable color depth. These features reduce SPI transfer time on slow displays and enable memory optimization for resource-constrained games.

**Release Date**: 2026-04-19

---

## What's New in v1.3.0

### Features Added

| Feature | Description | Breaking Change |
|---------|-------------|-----------------|
| Partial Updates | Dirty rect tracking - only modified regions sent to display | No |
| Color Depth Config | Configurable color depth (24/16/8 bits) | No |
| Auto-Mark Dirty | Automatic dirty region marking in drawTileDirect | No |
| Debug Overlay | Visual debug for dirty regions | No |
| Region Combining | Merge adjacent dirty blocks to reduce SPI transfers | No |
| Benchmark APIs | Query last frame statistics for tuning | No |

### Configuration Flags

All flags can be overridden in `platform.ini` under `[env:esp32dev]` with `-DFLAG_NAME=value`:

| Flag | Default | Valid Range | Description |
|------|---------|-------------|-------------|
| `ENABLE_PARTIAL_UPDATES` | 1 | 0-1 | Enable partial screen updates (0=full frame) |
| `DISPLAY_COLOR_DEPTH` | 16 | 24, 16, 8 | Color depth in bits |
| `MAX_DIRTY_RATIO_PERCENT` | 70 | 0-100 | Threshold: if dirty>% then use full frame |
| `ENABLE_DIRTY_RECT_COMBINE` | 1 | 0-1 | Enable adjacent dirty block combining |
| `PIXELROOT32_DEBUG_DIRTY_REGIONS` | 0 | 0-1 | Enable debug overlay (red borders) | |

---

## Rollback Procedure

If you experience issues with the optimizations, you can roll back to previous behavior.

### Quick Rollback (Partial Updates Off)

To restore original behavior (without partial updates):

```ini
[env:esp32dev]
build_flags =
    -DENABLE_PARTIAL_UPDATES=0
    -DDISPLAY_COLOR_DEPTH=24
```

This disables only partial updates while keeping the 24-bit color depth.

### Full Rollback (Complete v1.2.x Behavior)

For complete v1.2.x behavior with all optimizations disabled:

```ini
[env:esp32dev]
build_flags =
    -DENABLE_PARTIAL_UPDATES=0
    -DDISPLAY_COLOR_DEPTH=24
    -DMAX_DIRTY_RATIO_PERCENT=100
    -DENABLE_DIRTY_RECT_COMBINE=0
    -DPIXELROOT32_DEBUG_DIRTY_REGIONS=0
```

### Verification Steps

After rollback:

1. Build and upload to device
2. Verify display shows full frame updates (no partial optimization)
3. Test game performance meets expectations
4. Confirm no visual artifacts

### Troubleshooting

| Problem | Solution |
|---------|----------|
| Display shows wrong colors | Set `DISPLAY_COLOR_DEPTH=24` for RGB888 output |
| Flickering or tearing | Set `ENABLE_PARTIAL_UPDATES=0` to disable partial updates |
| Only partial screen updates | Disable partial updates: `ENABLE_PARTIAL_UPDATES=0` |
| Debug borders visible | Set `PIXELROOT32_DEBUG_DIRTY_REGIONS=0` |

---

## Migration: Existing Games

### Backward Compatibility

Version 1.3.0 maintains full backward compatibility:

- **No changes required**: Existing games work without modifications
- **Partial updates via impl**: Only drivers with implementation activate it
- **Legacy API intact**: All existing methods remain available

### Default Behavior

By default, partial updates is enabled (`ENABLE_PARTIAL_UPDATES=1`), but default implementations are no-op. Existing games work without changes because:

1. `DrawSurface::markDirty()` is a no-op by default
2. `DrawSurface::hasDirtyRegions()` returns false by default
3. Engine calls `beginFrame()`/`endFrame()` but they do nothing without driver

### Verify Compatibility

1. Build existing game with new v1.3.0 version
2. Deploy to target hardware
3. Verify visual output is identical to v1.2.x
4. Confirm FPS is not degraded

### Progressive Adoption

To enable optimizations progressively:

```ini
; Enable partial updates (already default, shown for clarity)
[env:esp32dev]
build_flags =
    -DENABLE_PARTIAL_UPDATES=1

; Use 8-bit indexed color for memory savings
[env:esp32dev]
build_flags =
    -DDISPLAY_COLOR_DEPTH=8
```

| Step | Change | Benefit |
|------|--------|---------|
| 1 | Keep defaults | Transparent upgrade |
| 2 | Enable partial updates | ~50%+ display transfer reduction |
| 3 | Set color depth 8 | ~50-75% memory reduction |
| 4 | Tune min region size | Filter noise for smaller transfers |

### No Deprecated API

No methods are deprecated in v1.3.0. All existing APIs remain available.

### Compatibility Verification

```cpp
void setup() {
    // Existing game - no changes required
    Engine engine(displayConfig);
    engine.run<MyGameScene>();
    // Partial updates available but not required
}
```

---

## Configuration Reference

### Partial Update Settings

| Setting | Optimized | Rollback | Description |
|---------|-----------|----------|-------------|
| `ENABLE_PARTIAL_UPDATES` | 1 | 0 | Enable partial updates |
| `MAX_DIRTY_RATIO_PERCENT` | 70 | 100 | Use full frame if dirty>% |
| `ENABLE_DIRTY_RECT_COMBINE` | 1 | 0 | Merge adjacent dirty blocks |

### Color Depth Settings

| Setting | Optimized | Rollback | Description |
|---------|-----------|----------|-------------|
| `DISPLAY_COLOR_DEPTH` | 16 | 24 | Color depth in bits |

### Color Depth Values

| Value | Format | Memory (320x240) | Use Case |
|-------|--------|------------------|----------|
| 24 | RGB888 | 230,400 bytes | Highest quality |
| 16 | RGB565 (default) | 153,600 bytes | Balance quality/performance |
| 8 | Indexed 256-color | 76,800 bytes | Lower memory footprint |

### Platform Support

| Platform | 24-bit | 16-bit | 8-bit |
|----------|--------|--------|-------|
| ESP32/TFT_eSPI | No | Yes (default) | Yes (with sprite) |
| SDL2/Native | Yes | Yes | No |

---

## API Changes

### New Methods Added

All new methods have default no-op implementations in `DrawSurface` base class, ensuring backward compatibility. Override in driver implementations (`TFT_eSPI_Drawer`) for full functionality.

| Method | Class | Description |
|--------|-------|------------|
| **DrawSurface / BaseDrawSurface:** |
| `markDirty(x, y, w, h)` | DrawSurface | Mark region as dirty to track |
| `clearDirtyFlags()` | DrawSurface | Clear dirty tracking for next frame |
| `hasDirtyRegions()` | DrawSurface | Check if dirty regions exist |
| `setPartialUpdateEnabled(bool)` | DrawSurface | Enable/disable partial updates |
| `isPartialUpdateEnabled()` | DrawSurface | Check if partial updates enabled |
| `setColorDepth(int depth)` | DrawSurface | Set color depth (24/16/8 bits) |
| `beginFrame()` | DrawSurface | Begin frame, prepare tracking |
| `endFrame()` | DrawSurface | End frame, finalize dirty regions |
| `setAutoMarkDirty(bool)` | DrawSurface | Enable auto-marking after draw ops |
| `isAutoMarkDirty()` | DrawSurface | Check auto-marking status |
| `setDebugDirtyRegions(bool)` | DrawSurface | Enable debug overlay |
| `isDebugDirtyRegions()` | DrawSurface | Check debug overlay status |
| **PartialUpdateController:** |
| `setMinRegionPixels(int)` | PartialUpdateController | Set min region size (default: 256) |
| `getLastRegionCount()` | PartialUpdateController | Get last frame region count |
| `getLastTotalSentPixels()` | PartialUpdateController | Get last frame pixels sent |
| **ColorDepthManager:** |
| `setCustomPalette(const uint16_t*)` | ColorDepthManager | Set custom 256-color palette |
| `getPalette()` | ColorDepthManager | Get current palette pointer |

### Usage Example

```cpp
// Example: Using partial update APIs in your game
void MyScene::update() {
    // Auto-mark dirty is enabled by default
    // markDirty() is called automatically after drawTileDirect()

    // Query benchmark stats from previous frame
    int regions = renderer.getLastRegionCount();
    int pixels = renderer.getLastTotalSentPixels();
    Serial.printf("Sent %d regions, %d pixels\n", regions, pixels);
}

void MyScene::onEnter() {
    // Enable debug overlay for tuning
    renderer.setDebugDirtyRegions(true);

    // Or reduce color depth for memory-constrained games
    renderer.setColorDepth(8);  // 256-color indexed mode
}
```

### Default Implementations

All methods in `DrawSurface` base class are no-op by default. Override in `TFT_eSPI_Drawer` for actual partial update functionality. This ensures existing games work without changes.

---

## Upgrading from v1.2.x

### Step 1: Update platform.ini

Add the configuration flags you need (or use defaults for transparent upgrade):

```ini
[env:esp32dev]
build_flags =
    -DENABLE_PARTIAL_UPDATES=1
    -DDISPLAY_COLOR_DEPTH=16
```

### Step 2: Test Your Game

1. Build and upload to device
2. Verify visual output is identical
3. Check performance meets expectations

### Step 3: Optimize (Optional)

If you want to optimize further:

```ini
; Maximum performance - 8-bit indexed color
[env:esp32dev]
build_flags =
    -DENABLE_PARTIAL_UPDATES=1
    -DDISPLAY_COLOR_DEPTH=8
    -DMAX_DIRTY_RATIO_PERCENT=70
    -DENABLE_DIRTY_RECT_COMBINE=1
```

---

## Known Issues

- **8-bit color depth** requires sprite support in display driver (TFT_eSPI). Only available on ESP32 with `TFT_eSPI` and when using `beginFrame()`/`getSpriteBuffer()` workflow.
- **Debug overlay** draws 2px red borders around each dirty region sent to display. Use `PIXELROOT32_DEBUG_DIRTY_REGIONS=0` to disable or `setDebugDirtyRegions(false)` at runtime.
- **Partial updates** requires driver implementation for actual optimization. Default no-op implementations in `DrawSurface` always return "no dirty regions" - override in `TFT_eSPI_Drawer`.
- **Dirty ratio threshold**: If more than `MAX_DIRTY_RATIO_PERCENT` (default 70%) of screen is dirty, system automatically falls back to full frame update.
- **Minimum region size**: Regions smaller than `setMinRegionPixels()` (default 256 pixels / 16x16) may be skipped. Tune with `setMinRegionPixels(64)` for smaller regions or `setMinRegionPixels(4096)` to filter noise.
- **4-bit color depth**: Not implemented in current release. Use 8-bit (256 colors) for indexed mode.
- **Auto-mark dirty**: Enabled by default in `BaseDrawSurface`. If drawing directly to raw framebuffer without using engine's draw methods, call `markDirty()` manually.

---

## Related Documentation

| Document | Description |
|----------|-------------|
| [ESP32_PERFORMANCE.md](./performance/ESP32_PERFORMANCE.md) | Performance optimization guide |
| [ARCHITECTURE.md](./ARCHITECTURE.md) | System architecture |
| [GRAPHICS_GUIDELINES.md](./GRAPHICS_GUIDELINES.md) | Graphics development guide |

---

## Related Documentation

| Document | Description |
|----------|-------------|
| [ESP32_PERFORMANCE.md](./performance/ESP32_PERFORMANCE.md) | Performance optimization guide |
| [ARCHITECTURE.md](./ARCHITECTURE.md) | System architecture |
| [GRAPHICS_GUIDELINES.md](./GRAPHICS_GUIDELINES.md) | Graphics development guide |
| [API_GRAPHICS.md](./api/API_GRAPHICS.md) | Graphics API reference |

---

*Document Version: 1.1*
*Migration Version: v1.3.0*
*Date: 2026-04-19*
*Updated: Verified against source code - all 17 API methods documented*