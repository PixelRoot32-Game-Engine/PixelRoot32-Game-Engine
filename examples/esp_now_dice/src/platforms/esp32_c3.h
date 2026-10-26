#ifdef PLATFORM_ESP32C3

#include <Arduino.h>
#include <esp_random.h>
#include <drivers/esp32/U8G2_Drawer.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "DiceDuelScene.h"

namespace pr32 = pixelroot32;

// The two environments build this same translation unit. Rejecting a bad flag
// here is cheaper than flashing two boards that both think they are node 1:
// both would then claim the A role, and the canonical seed order - the lower
// node id first - would have no way to tell them apart.
#if !defined(ESP_NOW_NODE_ID) || (ESP_NOW_NODE_ID != 1 && ESP_NOW_NODE_ID != 2)
#error "ESP_NOW_NODE_ID must be 1 or 2; use env:esp32c3_node_a or env:esp32c3_node_b"
#endif

// DFRobot Beetle ESP32-C3 with the 0.42" SSD1306 OLED.
//
// One button, on GPIO 3. InputConfig takes pins in index order, so this is
// button 0 - the only input the scene reads on hardware.
const int BTN_PIN = 3;

// I2C wiring for the on-board panel. There is no reset line, which 255 signals
// to the driver.
const uint8_t OLED_SDA = 5;
const uint8_t OLED_SCL = 6;
const uint8_t OLED_RST = 255;

// PHYSICAL_* is the SSD1306 controller's framebuffer; LOGICAL_* is the window
// the panel actually shows; X/Y_OFF_SET places one inside the other. All six
// come from platformio.ini so the build and this wiring cannot disagree.
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

pr32::input::InputConfig inputConfig(BTN_PIN); // 1 button: start, advance, reset

pr32::core::Engine engine(config, inputConfig);

// The role is compiled in, not selected at runtime: node 1 is A, node 2 is B.
// The seed base is fresh every boot, so a rematch after a power cycle never
// reuses the seeds of the previous match; esp_random() is evaluated once here,
// during static construction, and every round mixes it with its own round
// number and local node id.
esp_now_dice::DiceDuelScene diceDuelScene(ESP_NOW_NODE_ID, esp_random());

void setup() {
    engine.init();

    // init() brings the transport up, starts the session and binds the score
    // counter to it. The duel was already given both references at construction.
    engine.setScene(&diceDuelScene);

    // The Engine borrows the session; it never owns or initializes it.
    engine.setNetworkSession(diceDuelScene.primarySession());
}

void loop() {
    engine.run();
}

#endif // PLATFORM_ESP32C3
