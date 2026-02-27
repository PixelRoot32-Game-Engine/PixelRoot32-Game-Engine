# PixelRoot32 Platform Compatibility Guide

**Document Version:** 1.0  
**Last Updated:** February 2026  
**Engine Version:** v1.0.0

## Overview

This document provides detailed information about PixelRoot32 Game Engine compatibility across different ESP32 variants and platforms. It helps developers understand which features are available on their target hardware.

---

## Platform Feature Matrix

| Feature | ESP32 Classic | ESP32-S3 | ESP32-C3 | ESP32-S2 | ESP32-C6 | Native (PC) |
|---------|---------------|----------|----------|----------|----------|-------------|
| **CPU Architecture** | Dual Core Xtensa | Dual Core Xtensa | Single Core RISC-V | Single Core Xtensa | Single Core RISC-V | Multi-core x86/ARM |
| **FPU (Floating Point Unit)** | ✅ Available | ✅ Available | ❌ Not Available | ❌ Not Available | ❌ Not Available | ✅ Available |
| **Scalar Math Backend** | Float | Float | Fixed16 | Fixed16 | Fixed16 | Float |
| **Dual Core Support** | ✅ Yes | ✅ Yes | ❌ No | ❌ No | ❌ No | ✅ Yes (threads) |
| **Audio DAC Output** | ✅ Available | ❌ Not Available | ❌ Not Available | ❌ Not Available | ❌ Not Available | ❌ N/A |
| **Audio I2S Output** | ✅ Available | ✅ Available | ✅ Available | ✅ Available | ✅ Available | ❌ N/A |
| **SDL2 Audio** | ❌ N/A | ❌ N/A | ❌ N/A | ❌ N/A | ❌ N/A | ✅ Available |
| **WiFi** | ✅ Available | ✅ Available | ✅ Available | ✅ Available | ✅ Available | ❌ N/A |
| **Bluetooth** | ✅ Available | ✅ Available | ❌ Not Available | ✅ Available | ✅ Available | ❌ N/A |
| **Recommended Audio Core** | 0 | 0 | 0 | 0 | 0 | N/A |
| **Recommended Main Core** | 1 | 1 | 0 | 0 | 0 | N/A |

---

## Detailed Platform Information

### ESP32 Classic (Original)

**Target ID:** `esp32dev`

**Key Features:**

- **Audio DAC:** Internal DAC on GPIO 25/26 for direct speaker connection
- **Audio I2S:** Full I2S support for external DACs (e.g., PAM8302A)
- **Dual Core:** True dual-core processing with core affinity
- **FPU:** Hardware floating-point unit for optimal Scalar performance
- **Memory:** Typically 520KB SRAM

**Configuration:**

```cpp
// PlatformIO configuration
[env:esp32dev]
platform = espressif32
board = esp32dev
build_flags = 
    -std=gnu++17
    -fno-exceptions
```

**Audio Backend Priority:**

1. DAC (simplest wiring, no external components)
2. I2S (higher quality, external amplifier required)

---

### ESP32-S3

**Target ID:** `esp32s3`

**Key Features:**

- **No DAC:** Internal DAC not available - I2S only for audio
- **Dual Core:** Enhanced dual-core performance
- **FPU:** Hardware floating-point unit
- **AI Instructions:** Vector instructions for ML workloads
- **USB OTG:** Native USB support
- **Memory:** Up to 512KB SRAM + external PSRAM support

**Configuration:**

```cpp
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -D PIXELROOT32_NO_DAC_AUDIO  // Disable DAC since not available
```

**Audio:** I2S only (external amplifier required)

---

### ESP32-C3

**Target ID:** `esp32-c3`

**Key Features:**

- **Single Core:** RISC-V architecture
- **No FPU:** Uses Fixed16 math backend automatically
- **No Bluetooth:** WiFi only
- **Lower Power:** Optimized for power efficiency
- **Memory:** 400KB SRAM

**Performance Impact:**

- Uses Fixed16 (Q16.16) math instead of float
- ~30% performance improvement over software float emulation
- Slightly reduced precision for physics calculations

**Configuration:**

```cpp
[env:esp32-c3]
platform = espressif32
board = esp32-c3-devkitm-1
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -D PIXELROOT32_NO_DAC_AUDIO
```

**Audio:** I2S only

---

### ESP32-S2

**Target ID:** `esp32s2`

**Key Features:**

- **Single Core:** Xtensa architecture
- **No FPU:** Uses Fixed16 math backend
- **USB OTG:** Native USB device/host/OTG
- **Lower Power:** Optimized for battery operation
- **Memory:** 320KB SRAM

**Configuration:**

```cpp
[env:esp32s2]
platform = espressif32
board = esp32-s2-saola-1
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -D PIXELROOT32_NO_DAC_AUDIO
```

**Audio:** I2S only

---

### ESP32-C6

**Target ID:** `esp32-c6`

**Key Features:**

- **Single Core:** RISC-V architecture with extensions
- **No FPU:** Uses Fixed16 math backend
- **WiFi 6:** 2.4GHz WiFi 6 support
- **Bluetooth 5.0:** LE and mesh support
- **Memory:** 512KB SRAM

**Configuration:**

```cpp
[env:esp32-c6]
platform = espressif32
board = esp32-c6-devkitc-1
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -D PIXELROOT32_NO_DAC_AUDIO
```

**Audio:** I2S only

---

### Native (PC/Mac/Linux)

**Target ID:** `native`

**Key Features:**

- **SDL2 Backend:** Cross-platform windowing and input
- **Native Audio:** SDL2 audio subsystem
- **Multi-threading:** Full thread support
- **Hardware Acceleration:** GPU rendering when available
- **Development Tools:** Full debugging and profiling support

**Configuration:**

```cpp
[env:native]
platform = native
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -D PLATFORM_NATIVE
```

**Audio:** SDL2 audio (software mixing)

---

## Build Configuration Examples

### ESP32 with DAC Audio (Simplest Setup)

```cpp
[env:esp32_dac]
platform = espressif32
board = esp32dev
build_flags = 
    -std=gnu++17
    -fno-exceptions
    ; DAC audio is enabled by default on ESP32
```

### ESP32-S3 with I2S Audio (Recommended)

```cpp
[env:esp32s3_i2s]
platform = espressif32
board = esp32-s3-devkitc-1
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -D PIXELROOT32_NO_DAC_AUDIO  ; Explicitly disable DAC
    ; I2S is enabled by default
```

### ESP32-C3 (Fixed16 Math)

```cpp
[env:esp32c3_fixed16]
platform = espressif32
board = esp32-c3-devkitm-1
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -D PIXELROOT32_NO_DAC_AUDIO
    ; Fixed16 math is automatic (no FPU)
```

---

## Performance Characteristics

### Scalar Math Performance

- **FPU Platforms (ESP32, S3):** Native float performance
- **Non-FPU Platforms (C3, S2, C6):** Fixed16 optimized performance
- **Performance Gain:** ~30% FPS improvement on C3 vs software float

### Memory Usage by Platform

- **ESP32 Classic:** 520KB SRAM baseline
- **ESP32-S3:** 512KB SRAM + PSRAM option
- **ESP32-C3:** 400KB SRAM (most constrained)
- **ESP32-S2:** 320KB SRAM
- **ESP32-C6:** 512KB SRAM

### Audio Capabilities

- **DAC Output:** 8-bit, direct GPIO drive (PAM8302A recommended)
- **I2S Output:** 16-bit, external DAC required
- **Sample Rate:** 44.1kHz (configurable)
- **Latency:** <5ms typical

---

## Troubleshooting Platform Issues

### DAC Audio Not Working

**Symptoms:** No sound output on GPIO 25/26
**Likely Cause:** Using ESP32 variant without DAC (S3, C3, S2, C6)
**Solution:** Switch to I2S audio with external amplifier

### Compilation Errors with Float

**Symptoms:** Linker errors or slow performance on C3/S2/C6
**Likely Cause:** Using float instead of Scalar
**Solution:** Always use `Scalar` type and `toScalar()` conversion

### Dual Core Issues

**Symptoms:** Audio glitches or main loop instability
**Likely Cause:** Incorrect core assignment on single-core variants
**Solution:** Use PlatformCapabilities to detect core count

### Memory Constraints

**Symptoms:** Crashes or allocation failures
**Likely Cause:** Running out of SRAM on constrained platforms
**Solution:** Use lower logical resolution, reduce entity count

---

## Migration Between Platforms

### Upgrading from ESP32 to ESP32-S3

1. Disable DAC audio: Add `-D PIXELROOT32_NO_DAC_AUDIO`
2. Add I2S amplifier if using audio
3. No code changes needed (same FPU support)

### Downgrading to ESP32-C3

1. Expect Fixed16 math automatically
2. Single-core operation (no task pinning)
3. Reduce memory usage if needed
4. Test physics precision requirements

### Porting to Native Platform

1. Use SDL2 for window management
2. Audio switches to SDL2 backend automatically
3. Full float precision available
4. Multi-threading fully supported

---

## Future Platform Support

### Planned Support

- **ESP32-H2:** When Arduino core becomes stable
- **ESP32-C2:** Ultra-low-cost variant

### Platform Detection Code

```cpp
#include "platforms/PlatformCapabilities.h"

auto caps = pixelroot32::platforms::PlatformCapabilities::detect();

if (caps.hasDualCore) {
    // Use dual-core optimizations
    xTaskCreatePinnedToCore(audioTask, "Audio", 4096, nullptr, 5, nullptr, caps.audioCoreId);
}

if (!caps.hasFPU) {
    // Use Fixed16-friendly algorithms
    useFixedPointOptimizations();
}
```

---

## References

- **ESP32 Arduino Core Documentation:** <https://docs.espressif.com/projects/arduino-esp32/>
- **PlatformIO ESP32 Platforms:** <https://docs.platformio.org/en/latest/platforms/espressif32.html>
- **PixelRoot32 Engine Configuration:** See `platforms/PlatformDefaults.h`
- **Audio Backend Configuration:** See `audio/AudioConfig.h`

---

---

## Display Performance Best Practices

### SSD1306 / SH1106 OLED (I2C)

For monochromatic OLED displays using the U8G2 driver, the default I2C clock speed is often limited to 400kHz. On ESP32 platforms, you can significantly improve FPS (often doubling it from ~30 to 60 FPS) by increasing the bus clock to 1MHz.

**Recommendation:**

In your `DisplayConfig` setup or driver initialization, explicitly set the bus clock:

```cpp
if (u8g2_instance) {
    u8g2_instance->setBusClock(1000000); // Set to 1MHz
}
```

#### Integer Scaling Fast-Path (v1.0.0)

The engine includes a "Fast-Path" for 2x scaling that bypasses the generic bit-by-bit loop. This LUT-based expansion doubles the horizontal processing speed.

### TFT Displays (SPI/DMA)

#### SPI DMA Pipelining

For TFT displays using `TFT_eSPI`, the engine uses double-buffering and DMA. To minimize interrupt overhead, the engine processes data in large blocks (default: 60 lines).

**Recommendation:**

- Keep your `SPI_FREQUENCY` at the maximum stable value for your panel (typically 40MHz or 80MHz).
- Use exact 2x scaling (e.g., 120x120 -> 240x240) to trigger the hardware-optimized blit path.

> [!TIP]
> While most modern SSD1306 modules support 1MHz, if you experience visual glitches or "frozen" frames, try reducing the clock to 400kHz or 800kHz.

---

**Note:** This document is updated regularly as new ESP32 variants are released and tested with the PixelRoot32 engine. Always check the latest version for the most current information.
