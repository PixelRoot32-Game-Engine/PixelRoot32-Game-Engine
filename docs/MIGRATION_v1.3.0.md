# Migration Guide: v1.3.0 - Display Bottleneck Optimization

## Overview

Version 1.3.0 introduces display bottleneck optimization through **partial screen updates**. This reduces SPI transfer time by only sending modified screen regions to the display instead of the full frame each render cycle.

> **Note on `DISPLAY_COLOR_DEPTH`**: This flag configures sprite rendering accuracy metadata (e.g. can enable palette selection for future extensions). On ESP32 + TFT_eSPI, the SPI transfer always outputs **16-bit RGB565** regardless of this setting. Color depth does **not** reduce SPI bandwidth on ESP32. See [Color Depth Limitations](#color-depth-limitations) for details.

**Release Date**: 2026-04-19

---

## What's New in v1.3.0

### Features Added

| Feature | Description | Breaking Change |
|---------|-------------|-----------------|
| Partial Updates | Dirty rect tracking - only modified regions sent to display | No |
| Resolution Independence | Dirty rect grid/bitmap adjusts to any logical resolution | No |
| Color Depth Config | Configurable color depth (24/16/8 bits) | No |
| DMA Pipelining | Overlap CPU processing with DMA SPI transfers | No |
| Auto-Mark Dirty | Automatic dirty region marking in drawTileDirect | No |
| Debug Overlay | High-perf visual debug for dirty regions | No |
| Region Combining | Merge adjacent dirty blocks to reduce SPI transfers | No |
| Benchmark APIs | Query last frame statistics for tuning | No |

### Configuration Flags

All flags can be overridden in `platform.ini` under `[env:esp32dev]` with `-DFLAG_NAME=value`:

| Flag | Default | Valid Range | Description |
|------|---------|-------------|-------------|
| `ENABLE_PARTIAL_UPDATES` | 0 | 0-1 | Enable partial screen updates (0=full frame) |
| `DISPLAY_COLOR_DEPTH` | 16 | 16, 8 | Color depth metadata (see limitations) |
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
    -DDISPLAY_COLOR_DEPTH=16
```

This disables partial updates (full frame every render).

### Full Rollback (Complete v1.2.x Behavior)

For complete v1.2.x behavior with all optimizations disabled:

```ini
[env:esp32dev]
build_flags =
    -DENABLE_PARTIAL_UPDATES=0
    -DDISPLAY_COLOR_DEPTH=16
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
| Display shows wrong colors | Verify palette config; SPI always uses RGB565 on ESP32 |
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

By default, partial updates is disabled (`ENABLE_PARTIAL_UPDATES=0`). Existing games work without changes because they continue to use the traditional full frame updates. If you enable it (`ENABLE_PARTIAL_UPDATES=1`), it's still safe because default implementations are no-op:

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
; Enable partial updates (opt-in for v1.3.0+)
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
| 2 | Enable partial updates | ~50%+ SPI transfer reduction (fewer pixels sent) |
| 3 | Tune `MAX_DIRTY_RATIO_PERCENT` | Adjust full-frame fallback threshold |
| 4 | Tune min region size | Filter noise for smaller transfers |

> **What about `DISPLAY_COLOR_DEPTH=8`?** This flag is accepted but does **not** reduce SPI bandwidth on ESP32 — the wire format is always 16-bit RGB565. See [Color Depth Limitations](#color-depth-limitations).

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
| `DISPLAY_COLOR_DEPTH` | 16 | 16 | Color depth metadata (24 rejected on ESP32) |

### Color Depth Values

| Value | Format | SPI Wire Format (ESP32) | Effect |
|-------|--------|------------------------|--------|
| 24 | RGB888 | **Rejected on ESP32** (falls back to 16) | Config error — sets 16-bit silently | 
| 16 | RGB565 (default) | 16-bit RGB565 | **Recommended** — accurate stats |
| 8 | Indexed 256-color | 16-bit RGB565 | Metadata only, no SPI savings |
| 4 | Reserved | Not implemented — rejected | Returns false, falls back to 16-bit |

> **Color Depth Limitations** <a name="color-depth-limitations"></a>
>
> On ESP32 + `TFT_eSPI_Drawer`, the sprite buffer is always **8bpp internally** (palette-indexed). During SPI transfer, each pixel is converted from the 8bpp palette index to **16-bit RGB565** via a lookup table (`paletteLUT`). This conversion happens in hardware (DMA) and cannot be bypassed without replacing the pixel pipeline.
>
> As a result, **`DISPLAY_COLOR_DEPTH` does not affect SPI bandwidth on ESP32**. The setting is preserved for:
> - Future extensibility (e.g. RGB332 direct-send)
> - Metadata in `ColorDepthManager::getBytesPerPixel()` / `estimateTransferSize()`
> - Native (SDL2) builds where color format can differ

### Platform Support

| Platform | 24-bit | 16-bit | 8-bit |
|----------|--------|--------|-------|
| ESP32/TFT_eSPI | **No** (rejected, falls back to 16) | Yes (default) | Yes (palette only; SPI still 16-bit) |
| SDL2/Native | Yes | Yes | No |

---

## DMA Pipelining (Advanced)

Version 1.3.0 introduces **DMA Pipelining** in `TFT_eSPI_Drawer`. This optimization overlaps the CPU intensive color-conversion/copy operations with the SPI DMA transfer of the previous block.

**Benefit**: Reduces total frame time by up to **15-25%** during active screen updates compared to synchronous DMA.

**Requirements**:
1. `PLATFORM_ESP32` must be defined.
2. `ENABLE_PARTIAL_UPDATES` must be 1.
3. Logical resolution must be equal to or smaller than physical resolution (internal sprite buffer used).

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
| **Benchmark APIs (TFT_eSPI_Drawer / PartialUpdateController):** |
| `getLastRegionCount()` | Both | Get last frame region count |
| `getLastTotalSentPixels()` | Both | Get last frame pixels sent |
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

- **8-bit color depth**: Supports a full 256-color palette. The default palette is `PALETTE_PR32`. Custom palettes can be set via `ColorDepthManager`. **Note**: On ESP32, the SPI transfer remains 16-bit RGB565 — the 8-bit setting affects the sprite internal format only, not wire bandwidth.
- **Debug overlay**: Performance optimized in v1.3.0. Draws 2px red borders around each sent region. Enable/disable at runtime via `setDebugDirtyRegions()`.
- **4-bit color depth**: Not implemented. Calls to `setColorDepth(4)` are **rejected**, log a warning, and fall back to 16-bit.
- **24-bit color depth on ESP32**: Not supported. Calls to `setColorDepth(24)` on ESP32 are **rejected**, log a warning, and fall back to 16-bit. Native builds accept 24-bit.
- **DMA Pipelining**: Automatically active in `TFT_eSPI_Drawer` for both full and partial paths. Overlaps CPU color conversion with SPI transfer for maximum throughput.
- **`DISPLAY_COLOR_DEPTH` and SPI bandwidth**: This flag does **not** reduce SPI bytes/frame on ESP32. The pixel pipeline always converts 8bpp → RGB565 via paletteLUT before DMA transfer.

---


## Related Documentation

| Document | Description |
|----------|-------------|
| [ESP32_PERFORMANCE.md](./performance/ESP32_PERFORMANCE.md) | Performance optimization guide |
| [ARCHITECTURE.md](./ARCHITECTURE.md) | System architecture |
| [GRAPHICS_GUIDELINES.md](./GRAPHICS_GUIDELINES.md) | Graphics development guide |
| [API_GRAPHICS.md](./api/API_GRAPHICS.md) | Graphics API reference |

---

*Document Version: 1.2*  
*Migration Version: v1.3.0*  
*Date: 2026-04-19*  
*Updated: 2026-04-20 — corrected color depth limitations; removed false SPI bandwidth reduction claims*