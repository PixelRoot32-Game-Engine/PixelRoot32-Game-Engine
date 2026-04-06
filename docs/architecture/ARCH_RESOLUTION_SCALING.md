# Technical Specification: Independent Resolution Scaling System for PixelRoot32

## Executive Summary

This document describes the technical implementation of the **independent resolution scaling** system in the PixelRoot32 engine. This feature allows the engine to render internally at a lower "Logical Resolution" (e.g., 128x128) and scale the output to the "Physical Resolution" of the hardware display (e.g., 240x240). This optimization significantly reduces memory consumption and improves rendering performance on resource-constrained microcontrollers like the ESP32.

---

## Architecture Overview

### Logical vs. Physical Resolution

The system decouples the drawing operations from the hardware constraints:

1. **Logical Resolution (Rendering Resolution):** The virtual canvas dimensions where game logic, sprites, tilemaps, and UI are processed.
2. **Physical Resolution (Display Resolution):** The actual pixel dimensions of the hardware display (LCD/TFT).
3. **Scaling Layer:** An optimized nearest-neighbor algorithm that bridges the two resolutions during the final buffer transfer.

### Rendering Pipeline

```mermaid
flowchart LR
    subgraph Logical [Logical Resolution 128x128]
        A[Game Logic] --> B[Renderer API]
        B --> C[Logical Framebuffer 8bpp]
    end
    
    subgraph Scaling [On-the-fly Scaling]
        C --> D[Nearest Neighbor Scaler]
    end
    
    subgraph Physical [Physical Display 240x240]
        D --> E[SPI/DMA Transfer]
        E --> F[LCD Hardware]
    end
```

---

## Core Components

### 1. DisplayConfig & Resolution Abstraction

The `DisplayConfig` structure was extended to store both dimensions.

- `logicalWidth` / `logicalHeight`: Used by the `Renderer` for clipping and UI layouts.
- `physicalWidth` / `physicalHeight`: Used by the drivers to initialize the hardware and manage scaling.

### 2. On-the-Fly Nearest Neighbor Scaling

To avoid the massive RAM overhead of a full-size physical framebuffer (which would require 115KB for 240x240 RGB565), the scaling is performed **line-by-line** during the SPI transfer.

#### Algorithm

1. Calculate source Y coordinate: `srcY = (physY * logicalHeight) / physicalHeight`.
2. Retrieve the corresponding row from the logical 8bpp buffer.
3. Expand the row horizontally using pre-calculated Lookup Tables (LUTs) to determine `srcX`.
4. Convert each 8-bit indexed pixel to RGB565 format.
5. Send the resulting physical-width line to the display via DMA.

### 3. Optimization Techniques (ESP32)

- **8-bit to 16-bit Conversion:** Optimized color conversion from the engine's 8-bit palette to the hardware's RGB565.
- **Sub-pixel Stability:** Camera and actors use `Scalar` (float or Fixed16) for movement, and `Camera2D` uses `math::roundToInt` when applying the display offset. This prevents the "jitter" artifacts common when truncating floating-point positions to the integer pixel grid.
- **Fast-Path Switching (v1.0.0):**
  - **1:1 Native:** Directly volcates the buffer if logical and physical match, supporting offsets for centering.
  - **2x Integer Scaling:** Uses a Bit-Expansion LUT (OLED) or 32-bit register writes (TFT) to duplicate pixels without recalculating indices.
  - **Generic NN:** Fallback for fractional scales (e.g., 1.5x) using optimized LUTs.

---

## Performance Impact

The following table demonstrates the estimated savings on an ESP32 for a standard 240x240 display:

| Logical Resolution | Memory (8bpp) | RAM Savings | Performance Gain |
| :--- | :--- | :--- | :--- |
| **240x240** (Full) | 57.6 KB | 0% | Baseline |
| **160x160** | 25.6 KB | ~55% | ~30% FPS increase |
| **128x128** | 16.4 KB | ~72% | ~60% FPS increase |
| **96x96** | 9.2 KB | ~84% | ~100% FPS increase |

---

## Usage and Configuration

### Using Presets

The engine provides `ResolutionPresets` for common configurations:

```cpp
#include <graphics/ResolutionPresets.h>

auto config = pr32::graphics::ResolutionPresets::create(
    pr32::graphics::RES_128x128,
    pr32::graphics::ST7789
);
```

### Manual Setup

```cpp
pr32::graphics::DisplayConfig config(
    pr32::graphics::ST7789, 
    0,         // Rotation
    240, 240,  // Physical Size
    160, 160   // Logical Size
);
```

### Modular Compilation Impact

The resolution scaling system is always available, but its memory footprint can be optimized through modular compilation:

- **Lower logical resolutions** (128x128) combined with disabled subsystems can save 50-70% RAM
- **Higher logical resolutions** (240x240) may require disabling non-essential subsystems on constrained platforms
- **UI System Impact**: Disabling `PIXELROOT32_ENABLE_UI_SYSTEM=0` reduces scaling buffer requirements
- **Particles Impact**: Disabling `PIXELROOT32_ENABLE_PARTICLES=0` reduces rendering overhead for scaled displays

### Profiling

Enable `PIXELROOT32_ENABLE_PROFILING` in `EngineConfig.h` to monitor scaling performance in the Serial console. This is particularly useful when optimizing for different logical resolutions on constrained hardware.

**Modular Compilation Note:** Profiling overhead is minimal and can be safely enabled during performance tuning, even on resource-constrained platforms.

---

## Project Impact

This system enables PixelRoot32 to run complex games with rich backgrounds and multiple sprites on standard ESP32 chips without requiring external PSRAM, while maintaining a sharp, consistent pixel-art aesthetic.

### Benefits for Modular Compilation

- **Memory Flexibility**: Lower logical resolutions combined with selective subsystem inclusion enable games to fit within tight memory constraints
- **Performance Tuning**: Developers can balance visual quality vs. performance by choosing appropriate resolution and subsystem combination
- **Platform Optimization**: Minimal builds (e.g., 128x128 + disabled audio/physics) can run smoothly on ESP32-C3 with only 400KB SRAM
- **Scalable Architecture**: Same game code can target different memory budgets by simply changing build flags
