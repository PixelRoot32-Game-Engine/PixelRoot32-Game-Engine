# Extensibility Guide: Creating Custom Drivers

This guide explains how to implement a custom display driver (`DrawSurface`) to support hardware not included by default in the PixelRoot32 engine (e.g., monochromatic OLED displays, e-Ink screens, or non-standard SPI displays).

## 1. Inherit from `BaseDrawSurface`

The easiest way to create a driver is to inherit from `pixelroot32::graphics::BaseDrawSurface`. This class provides default implementations for most primitive methods (lines, circles, rectangles) using `drawPixel()`.

```cpp
#include <graphics/BaseDrawSurface.h>
#include <iostream>

class MyCustomDriver : public pixelroot32::graphics::BaseDrawSurface {
public:
    void init() override {
        // Initialize hardware (SPI, I2C, etc.)
        std::cout << "Hardware initialized" << std::endl;
    }

    void drawPixel(int x, int y, uint16_t color) override {
        // Logic to write a pixel to your buffer or hardware
    }

    void clearBuffer() override {
        // Logic to clear the buffer
    }

    void sendBuffer() override {
        // Logic to send the buffer to the physical display (Flush)
    }
};
```

## 2. Inject the Driver into the Engine

Once you have your class, you can inject it into the engine using the `PIXELROOT32_CUSTOM_DISPLAY` macro. The engine will take ownership of the pointer and handle automatic memory cleanup upon shutdown.

```cpp
#include <core/Engine.h>
#include "MyCustomDriver.h"

void setup() {
    // Create the configuration using our driver
    auto config = PIXELROOT32_CUSTOM_DISPLAY(new MyCustomDriver(), 240, 240);
    
    // Initialize the engine with this configuration
    Engine engine(std::move(config));
    
    engine.init();
    engine.run();
}
```

## 3. Memory Considerations

- **Ownership**: When using `PIXELROOT32_CUSTOM_DISPLAY`, you transfer object ownership to the engine. The macro wraps the raw pointer in a `std::unique_ptr`, so you should not delete it manually.
- **Smart Pointers**: Internally, the engine uses `std::unique_ptr` to manage the driver.
- **Performance**: `BaseDrawSurface` uses generic algorithms for lines and circles that call `drawPixel()`. If your hardware supports acceleration for these primitives, you can override the methods (e.g., `drawLine`, `drawFilledRectangle`) for better performance.

## 4. Mandatory vs. Optional Methods

| Method | Mandatory | Description |
| :--- | :---: | :--- |
| `init()` | Yes | Initial hardware configuration. |
| `drawPixel()` | Yes | The foundation of all rendering. |
| `sendBuffer()` | Yes | Sends data to the display. |
| `clearBuffer()` | Yes | Clears the screen/buffer. |
| `setOffset()` | No | Sets X/Y hardware alignment offset. |
| `setRotation()` | No | Handled internally by `BaseDrawSurface`. |
| `drawLine()` | No | Optimized in `BaseDrawSurface`. |
| `drawFilledRectangle()` | No | Optimized in `BaseDrawSurface`. |

---

## 5. Built-in Drivers (ESP32)

PixelRoot32 comes with pre-configured drivers for common libraries. You can switch between them using build flags.

### 5.1 TFT_eSPI (Color TFTs)

Used by default or via the `PIXELROOT32_USE_TFT_ESPI_DRIVER` flag.

- **Library**: `bodmer/TFT_eSPI`
- **Ideal for**: ST7789, ILI9341, etc.
- **Feature**: Supports hardware DMA and software scaling.

### 5.2 U8G2 (Monochrome OLEDs)

Enabled via the `PIXELROOT32_USE_U8G2` flag.

- **Library**: `olikraus/U8g2`
- **Ideal for**: SSD1306, SH1106 (128x64, 128x32).
- **Setup**:
  1. Add the configuration to your `platformio.ini`:

```ini
build_flags = 
    -D PIXELROOT32_USE_U8G2
    -D PIXELROOT32_NO_TFT_ESPI  ; Optional: to save even more space
```

  1. The engine will automatically use `U8G2_Drawer` with a standard configuration.

### 5.3 Custom U8G2 Instance

If you need a specific constructor (e.g., I2C pins, non-standard SPI), use the `DisplayType::CUSTOM` approach:

```cpp
#include <U8g2lib.h>
#include <drivers/esp32/U8G2_Drawer.h>

// 1. Create your specific U8G2 instance
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
    // 2. Wrap it in the PixelRoot32 Drawer
    // Note: Pass 'false' if you want to keep ownership of the u8g2 instance
    auto drawer = std::make_unique<pixelroot32::drivers::esp32::U8G2_Drawer>(&u8g2, false);
    
    // 3. Inject into engine
    auto config = PIXELROOT32_CUSTOM_DISPLAY(drawer.release(), 128, 64);
    Engine engine(std::move(config));
    // ...
}
```

---

## 6. Engine Configuration Macros

The engine's behavior can be customized using preprocessor macros in your `platformio.ini`.

### 6.1 Core Engine Settings

| Macro | Description | Default |
| :--- | :--- | :--- |
| `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Shows real-time FPS and RAM usage on screen. | Disabled |
| `PIXELROOT32_ENABLE_PROFILING` | Enables low-level timing logs in the Serial monitor. | Disabled |
| `MAX_LAYERS` | Maximum number of rendering layers (higher = more RAM). | 4 |
| `MAX_ENTITIES` | Maximum number of active entities in a scene. | 64 |

### 6.2 Display & Resolution

| Macro | Description |
| :--- | :--- |
| `PHYSICAL_DISPLAY_WIDTH` | The actual width of the hardware screen (e.g., 240 or 128). |
| `PHYSICAL_DISPLAY_HEIGHT` | The actual height of the hardware screen (e.g., 240 or 64). |
| `PIXELROOT32_ENABLE_2BPP_SPRITES` | Enables support for 4-color palettes (saves RAM). |
| `PIXELROOT32_ENABLE_4BPP_SPRITES` | Enables support for 16-color palettes (saves RAM). |

### 6.3 ESP32 Specific (TFT_eSPI)

When using `TFT_eSPI`, you must provide the hardware configuration in `platformio.ini`. Different displays require specific flags:

#### Common Configuration Flags

- `USER_SETUP_LOADED=1`: Mandatory to override internal TFT_eSPI settings.
- `TFT_MOSI`, `TFT_SCLK`, `TFT_DC`, `TFT_RST`, `TFT_CS`: Pin definitions.
- `SPI_FREQUENCY`: Maximum SPI speed (Display dependent).

#### Example: ST7789 (240x240)

```ini
build_flags = 
    -D USER_SETUP_LOADED=1
    -D ST7789_DRIVER
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=240
    -D SPI_FREQUENCY=40000000
    ; ... pins ...
```

#### Example: ST7735 (128x128)

Note that ST7735 often requires a specific "Tab" color flag for correct offsets/colors:

```ini
build_flags = 
    -D USER_SETUP_LOADED=1
    -D ST7735_DRIVER
    -D ST7735_GREENTAB3  ; Specific for some 128x128 displays
    -D TFT_WIDTH=128
    -D TFT_HEIGHT=128
    -D SPI_FREQUENCY=27000000
    ; ... pins ...
```

---
*PixelRoot32 - Extensible Driver System (Bridge Pattern)*
