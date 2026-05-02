# Layer 2: Abstraction Layer

## Responsibility

Abstract interfaces that decouple subsystems from concrete implementations, enabling portability and testability.

**Design Patterns**:
- **Bridge Pattern**: `DrawSurface` decouples Renderer from specific drivers
- **Strategy Pattern**: `AudioScheduler` allows different scheduling implementations

---

## Main Components

### PlatformMemory.h (Macro Abstraction)

Provides a unified API for memory operations that differ between ESP32 (Flash/PROGMEM) and Native (RAM) platforms.

| Macro | Description | ESP32 Mapping | Native Mapping |
|-------|-------------|---------------|----------------|
| `PIXELROOT32_FLASH_ATTR` | Store data in Flash | `PROGMEM` | (empty) |
| `PIXELROOT32_STRCMP_P` | Compare with Flash string | `strcmp_P` | `strcmp` |
| `PIXELROOT32_MEMCPY_P` | Copy from Flash | `memcpy_P` | `memcpy` |
| `PIXELROOT32_READ_BYTE_P` | Read 8-bit from Flash | `pgm_read_byte` | direct access |
| `PIXELROOT32_READ_WORD_P` | Read 16-bit from Flash | `pgm_read_word` | direct access |
| `PIXELROOT32_READ_DWORD_P` | Read 32-bit from Flash | `pgm_read_dword` | direct access |
| `PIXELROOT32_READ_FLOAT_P` | Read float from Flash | `pgm_read_float` | direct access |

**Usage Example**:

```cpp
#include "platforms/PlatformMemory.h"

// Store data in Flash
const char message[] PIXELROOT32_FLASH_ATTR = "Hello";

// Compare with Flash string
if (PIXELROOT32_STRCMP_P("lava", type) == 0) {
    // Handle lava tile
}
```

---

### DrawSurface (Bridge Pattern)

Abstract base class that decouples the Renderer from specific display drivers.

```cpp
class DrawSurface {
public:
    virtual void init() = 0;
    virtual void drawPixel(int x, int y, uint16_t color) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void drawFilledRectangle(int x, int y, int w, int h, uint16_t color) = 0;
    virtual void clearBuffer() = 0;
    virtual void sendBuffer() = 0;
    virtual void setOffset(int x, int y) {}
    virtual void setRotation(uint8_t rotation) {}
};
```

**Implementations**:
- `TFT_eSPI_Drawer` - ESP32 color displays
- `U8G2_Drawer` - ESP32 monochrome displays
- `SDL2_Drawer` - PC simulation

---

### AudioScheduler (Strategy Pattern)

Abstract scheduler for platform-specific audio timing.

```cpp
class AudioScheduler {
public:
    virtual void init() = 0;
    virtual void submitCommand(const AudioCommand& cmd) = 0;
    virtual void generateSamples(int16_t* stream, int length) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};
```

**Implementations**:
- `ESP32AudioScheduler` - FreeRTOS task on Core 0
- `NativeAudioScheduler` - POSIX thread (PC)

---

### PlatformCapabilities

Structure that detects and exposes hardware capabilities at runtime.

```cpp
namespace pixelroot32::platforms {
    struct PlatformCapabilities {
        bool hasDualCore;       // Multi-core support (ESP32, ESP32-S3)
        int audioCoreId;        // Recommended core for audio (0 on ESP32)
        int mainCoreId;         // Recommended core for game loop (1 on ESP32)
        bool hasFPU;            // Floating-point unit available
        size_t totalSRAM;       // Total SRAM available
        bool hasPSRAM;          // External SPI RAM available (S3)
    };
}
```

**Usage**:

```cpp
auto caps = pixelroot32::platforms::detectCapabilities();
if (caps.hasDualCore) {
    // Use dual-core audio scheduling
}
```

---

### Math System (Scalar Abstraction)

**Files**: `include/math/Scalar.h`, `include/math/Fixed16.h`, `include/math/MathUtil.h`

Provides deterministic, platform-optimized numerical operations.

**Features**:

- **Hardware Adaptation**: Automatically switches between `float` and `Fixed16` based on FPU presence (ESP32-S3 vs ESP32-C3)
- **16.16 Fixed Point**: Optimized `Fixed16` implementation for RISC-V targets (C3/C6)
- **Generic Math API**: Single API for `sin`, `cos`, `sqrt`, `atan2` that resolves to most efficient implementation per platform
- **Stable Rounding**: Explicit `roundToInt`, `floorToInt`, `ceilToInt` to avoid floating-point truncation artifacts

```cpp
using Scalar = pixelroot32::math::Scalar;  // float or Fixed16

Scalar angle = MathUtil::atan2(dy, dx);
Scalar distance = MathUtil::sqrt(dx*dx + dy*dy);
int pixelX = MathUtil::roundToInt(cameraX + offset);
```

---

### Unified Logging System

**Files**: `include/core/Log.h`, `src/platforms/PlatformLog.cpp`

Cross-platform logging abstraction that eliminates `#ifdef` blocks in user code.

**Features**:
- Unified API for ESP32 (Serial) and Native (stdout)
- Log levels: Info, Profiling, Warning, Error
- printf-style formatting
- Automatic platform routing
- **Zero overhead when disabled**: Double-layer conditional compilation

**Architecture - Double-Layer Conditional Compilation**:

```
PIXELROOT32_DEBUG_MODE defined:
    log() → format with va_list → platformPrint() → Serial/stdout

PIXELROOT32_DEBUG_MODE not defined:
    log() → (void)level; (void)fmt; → no-op
```

**Main API**:

```cpp
namespace pixelroot32::core::logging {
    enum class LogLevel { Info, Profiling, Warning, Error };
    void log(LogLevel level, const char* format, ...);
    void log(const char* format, ...); // Info level shorthand
}

// Enable in platformio.ini:
// build_flags = -DPIXELROOT32_DEBUG_MODE
```

**Platform Output**:
- ESP32: Routes to `Serial.print()`
- Native: Routes to `printf()` with `fflush(stdout)`

---

## Abstraction Benefits

1. **Portability**: Same game code runs on ESP32 and PC
2. **Testability**: Mock implementations for unit testing
3. **Flexibility**: Swap implementations without changing game code
4. **Maintainability**: Changes isolated to specific layers

---

## Related Documentation

- [API Reference - Math Module](../api/API_MATH.md) - Scalar and math utilities
- [API Reference - Platform](../api/API_PLATFORM.md) - Platform abstractions
- [Memory System](memory-system.md) - Memory management details
- [Hardware Layer](layer-hardware.md) - Physical hardware
- [Driver Layer](layer-drivers.md) - Concrete implementations
