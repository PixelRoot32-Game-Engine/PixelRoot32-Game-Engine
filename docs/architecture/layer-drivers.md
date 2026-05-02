# Layer 1: Driver Layer

## Responsibility

Platform-specific hardware abstraction that bridges the gap between hardware and the engine's abstract interfaces.

**Design Pattern**: Concrete implementation of abstractions defined in Layer 2.

---

## ESP32 Drivers

| Driver | File | Description |
|--------|------|-------------|
| `TFT_eSPI_Drawer` | `drivers/esp32/TFT_eSPI_Drawer.cpp` | TFT display driver (ST7789, ST7735, ILI9341) |
| `U8G2_Drawer` | `drivers/esp32/U8G2_Drawer.cpp` | Monochrome OLED driver (SSD1306, SH1106) |
| `ESP32_I2S_AudioBackend` | `drivers/esp32/ESP32_I2S_AudioBackend.cpp` | I2S audio backend for external DACs |
| `ESP32_DAC_AudioBackend` | `drivers/esp32/ESP32_DAC_AudioBackend.cpp` | Internal DAC audio backend |
| `ESP32AudioScheduler` | `audio/ESP32AudioScheduler.cpp` | Multi-core audio scheduler (FreeRTOS task) |

### TFT_eSPI Driver

The primary color display driver using the popular TFT_eSPI library.

**Features**:
- Hardware SPI communication
- DMA support for fast transfers
- Resolution scaling (nearest-neighbor)
- Double-buffering for smooth rendering

**Future optimization (Opción B — diseño, no implementado):** `sendBufferScaled()` hoy envía el rectángulo completo (**`setAddrWindow`** + bloques DMA). Una variante sería **comparar** el sprite 8 bpp contra una **copia del frame anterior** (o un diff por bandas) y emitir **varias ventanas** SPI solo donde cambiaron píxeles. Mejora el techo SPI cuando el área sucia es pequeña; coste típico **~W×H bytes** RAM y más llamadas a **`setAddrWindow`**. La **Opción A** (omitir `draw`+`present` en el **`Engine`** cuando la escena lo indica) está descrita en [ESP32 rendering](../ARCHITECTURE.md#esp32-rendering-pipeline-and-tilemap-caching).

**Supported Displays**:
- ST7789 (240x240, 320x240)
- ST7735 (128x128, 160x128)
- ILI9341 (320x240)

### U8G2 Driver

Driver for monochrome OLED displays using the U8G2 library.

**Features**:
- I2C and SPI support
- 1MHz I2C bus overclocking for 60 FPS
- Page buffer mode for memory efficiency

**Supported Displays**:
- SSD1306 (128x64, 128x32)
- SH1106 (128x64)

### Audio Drivers

#### ESP32_I2S_AudioBackend

For high-quality audio with external DACs (MAX98357A, PCM5102).

```cpp
ESP32_I2S_AudioBackend audioBackend(26, 25, 22, 22050);
// BCLK=26, LRCK=25, DOUT=22, 22050Hz sample rate
```

**Features**:
- I2S peripheral with DMA
- Standard sample rates (11025, 22050, 44100 Hz)
- Pinned to Core 0 (separate from game loop)

#### ESP32_DAC_AudioBackend

For retro-style audio using the internal 8-bit DAC.

```cpp
ESP32_DAC_AudioBackend audioBackend(25, 11025);
// GPIO 25, 11025Hz for retro feel
```

**Features**:
- 8-bit resolution
- Software-based sample pushing
- 0.7x attenuation to prevent saturation

---

## Native (PC) Drivers

| Driver | File | Description |
|--------|------|-------------|
| `SDL2_Drawer` | `drivers/native/SDL2_Drawer.cpp` | SDL2 graphics simulation |
| `SDL2_AudioBackend` | `drivers/native/SDL2_AudioBackend.cpp` | SDL2 audio backend |
| `NativeAudioScheduler` | `audio/NativeAudioScheduler.cpp` | Native thread-based scheduler |
| `MockArduino` | `platforms/mock/MockArduino.cpp` | Arduino API emulation |

### SDL2_Drawer

Graphics driver for PC development using SDL2.

**Features**:
- Windowed and fullscreen modes
- Hardware acceleration via SDL2
- Mouse-to-touch event conversion (when touch enabled)
- Pixel-perfect scaling options

### SDL2_AudioBackend

Audio driver using SDL2's audio subsystem.

**Features**:
- Standard audio device access
- Callback-based sample generation
- Thread-safe command queue

### NativeAudioScheduler

Thread-based audio scheduling for PC platforms.

**Features**:
- Dedicated high-priority thread
- Sample-accurate timing
- Lock-free command queue

---

## Driver Selection

Drivers are selected at compile-time via build flags:

```ini
# platformio.ini

# Use TFT_eSPI (default)
build_flags = -D PIXELROOT32_USE_TFT_ESPI_DRIVER

# Use U8G2 for OLED
build_flags = -D PIXELROOT32_USE_U8G2

# Custom display
custom_display = new MyCustomDriver()
```

---

## Creating Custom Drivers

See [Extending PixelRoot32](../EXTENDING_PIXELROOT32.md) for detailed instructions on creating custom display and audio drivers.

Quick overview:

```cpp
#include <graphics/BaseDrawSurface.h>

class MyCustomDriver : public pixelroot32::graphics::BaseDrawSurface {
public:
    void init() override;
    void drawPixel(int x, int y, uint16_t color) override;
    void clearBuffer() override;
    void sendBuffer() override;
};
```

---

## Related Documentation

- [Abstraction Layer](ARCH_LAYER_ABSTRACTION.md) - Interfaces these drivers implement
- [Hardware Layer](ARCH_LAYER_HARDWARE.md) - Physical hardware details
- [System Layer](ARCH_LAYER_SYSTEMS.md) - High-level systems that use these drivers
- [Extending PixelRoot32](../EXTENDING_PIXELROOT32.md) - How to create custom drivers
