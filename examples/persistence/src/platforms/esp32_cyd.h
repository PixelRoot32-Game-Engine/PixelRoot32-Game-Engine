#ifdef PLATFORM_ESP32CYD

#include <Arduino.h>
#include <core/Engine.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <platforms/EngineConfig.h>

#include "PersistenceExampleScene.h"

namespace pr32 = pixelroot32;

// The CYD has no 5-way pad and its spare GPIOs are scarce. Wiring the pad to
// its usual pins would take 12/13/14, which on this board are the panel's SPI
// bus, and Engine::init() configures the display before the input pins, so
// pinMode() on them would switch the MOSI line to an input and kill the panel.
// Every logical button shares the BOOT button (GPIO 0) instead: pressing it
// triggers A, the only one the scene reads.
const int BTN_UP_PIN = 0;
const int BTN_DOWN_PIN = 0;
const int BTN_LEFT_PIN = 0;
const int BTN_RIGHT_PIN = 0;
const int BTN_A_PIN = 0;
const int BTN_B_PIN = 0;

// Panel block (ILI9341_2 240x320, backlight 21, inversion on) comes from the
// board's TFT_eSPI flags in platformio.ini; see the esp32sd environment.
pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::ILI9341_2,
    DISPLAY_ROTATION,
    PHYSICAL_DISPLAY_WIDTH,
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN,
                                     BTN_A_PIN, BTN_B_PIN);

pr32::core::Engine engine(config, inputConfig);

persistenceexample::PersistenceExampleScene scene;

void setup() {
    engine.init();
    engine.setScene(&scene);
}

void loop() {
    engine.run();
}

#endif  // PLATFORM_ESP32CYD
