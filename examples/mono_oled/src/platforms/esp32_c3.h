#ifdef PLATFORM_ESP32C3

#include <Arduino.h>
#include <drivers/esp32/U8G2_Drawer.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "MonoOledScene.h"

namespace pr32 = pixelroot32;

// DFRobot Beetle ESP32-C3 with the 0.42" SSD1306 OLED.
//
// The board has one usable button for this example; there is no D-pad and
// no touch panel. InputConfig takes the pins in index order, so this single
// pin becomes button 0 — the BTN_NEXT the scene reads.
const int BTN_PIN_NEXT = 3;

// I2C wiring for the on-board panel. There is no reset line, which 255
// signals to the driver.
const uint8_t OLED_SDA = 5;
const uint8_t OLED_SCL = 6;
const uint8_t OLED_RST = 255;

// PHYSICAL_* is the SSD1306 controller's framebuffer; LOGICAL_* is the
// window the panel actually shows; X/Y_OFF_SET places one inside the other.
// All six come from platformio.ini so the build and this wiring cannot
// disagree.
pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::OLED_SSD1306,
    DISPLAY_ROTATION,
    OLED_SCL, OLED_SDA, 255, 255, OLED_RST,
    PHYSICAL_DISPLAY_WIDTH,
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(BTN_PIN_NEXT); // 1 button: next page

pr32::core::Engine engine(config, inputConfig);

mono_oled::MonoOledScene monoOledScene;

void setup() {
    engine.init();
    engine.setScene(&monoOledScene);
}

void loop() {
    engine.run();
}

#endif // PLATFORM_ESP32C3
