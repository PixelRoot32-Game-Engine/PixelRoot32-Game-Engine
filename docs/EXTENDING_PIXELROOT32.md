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

- **Ownership**: When using `PIXELROOT32_CUSTOM_DISPLAY`, you transfer object ownership to the engine. Do not attempt to delete the pointer manually.
- **Smart Pointers**: Internally, the engine uses `std::unique_ptr` to manage the driver.
- **Performance**: `BaseDrawSurface` uses generic algorithms for lines and circles that call `drawPixel()`. If your hardware supports acceleration for these primitives, you can override the methods (e.g., `drawLine`, `drawFilledRectangle`) for better performance.

## 4. Mandatory vs. Optional Methods

| Method | Mandatory | Description |
| :--- | :---: | :--- |
| `init()` | Yes | Initial hardware configuration. |
| `drawPixel()` | Yes | The foundation of all rendering. |
| `sendBuffer()` | Yes | Sends data to the display. |
| `clearBuffer()` | Yes | Clears the screen/buffer. |
| `setRotation()` | No | Handled internally by `BaseDrawSurface`. |
| `drawLine()` | No | Optimized in `BaseDrawSurface`. |
| `drawFilledRectangle()` | No | Optimized in `BaseDrawSurface`. |

---
*PixelRoot32 - Extensible Driver System (Bridge Pattern)*
