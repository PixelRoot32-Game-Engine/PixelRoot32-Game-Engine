# Implementation Plan: U8G2 Driver for PixelRoot32

This document outlines the strategy for integrating the **U8G2** library as an alternative display driver in the PixelRoot32 engine, ensuring memory efficiency (Flash/RAM) on the ESP32.

## 1. Technical Analysis

### A. Color Conversion

The PixelRoot32 engine internally operates in **RGB565** (16-bit). Displays supported by U8G2 are mostly **monochromatic** (1-bit).

- **Strategy**: Implement a threshold logic in `drawPixel`. Any color with luminance > 0 will be considered "on".

### B. Memory Management (Binary Bloat)

To prevent binary size increase for users not using OLED:

- Preprocessor guards `#if defined(PIXELROOT32_USE_U8G2_DRIVER)` will be used.
- The linker will eliminate dead code if the library is not instantiated.

### C. Buffering

The **Full Framebuffer (_F_)** mode of U8G2 will be used to ensure compatibility with the engine's layered rendering system.

---

## 2. Phased Implementation Plan

### Phase 1: Configuration Infrastructure

- [ ] Modify `PlatformDefaults.h` to include the `PIXELROOT32_USE_U8G2_DRIVER` macro (disabled by default).
- [ ] Update `DisplayConfig.h` to conditionally include the new driver's header.
- [ ] Add new types to the `DisplayType` enum (e.g., `SSD1306`, `SH1106`).

### Phase 2: `U8G2_Drawer` Driver Development

- [ ] Create `include/drivers/esp32/U8G2_Drawer.h` inheriting from `BaseDrawSurface`.
  - _Note_: The constructor will allow passing a custom `U8G2` instance to support `DisplayType::CUSTOM`.
- [ ] Create `src/drivers/esp32/U8G2_Drawer.cpp` with the implementation of:
  - `init()`: Hardware initialization.
  - `drawPixel()`: RGB565 to 1-bit conversion.
  - `clearBuffer()` and `sendBuffer()`: Frame flow management.
  - `drawBitmap()`: Support for XBM (U8G2 native format).

### Phase 3: Factory System Integration

- [ ] Modify `DisplayConfig.cpp` to instantiate `U8G2_Drawer` with generic configurations (SSD1306 128x64) based on the selected `DisplayType`, protected by macros.
- [ ] Document how to use `DisplayType::CUSTOM` along with `U8G2_Drawer` for displays not natively supported by the factory.

### Phase 4: Validation and Documentation

- [ ] Verify the resulting binary size with and without the driver active.
- [ ] Create a practical example (`main.cpp`) configured for an SSD1306 OLED display.
- [ ] Document the driver switching process in `EXTENDING_PIXELROOT32.md`.

---

## 3. `platformio.ini` Configuration Example

To activate this driver, the developer must configure their environment as follows:

```ini
lib_deps = 
    olikraus/U8g2@^2.36.4

build_flags = 
    -D PIXELROOT32_USE_U8G2_DRIVER
    -D PIXELROOT32_NO_TFT_ESPI  ; Optional: to save even more space
```
