#ifdef PLATFORM_ESP32DEV

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <drivers/esp32/ESP32_I2S_AudioBackend.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "IsoDungeonScene.h"

namespace pr32 = pixelroot32;

// Audio Pin Configuration (I2S)
const int I2S_BCLK = 26;
const int I2S_LRCK = 25;
const int I2S_DOUT = 22;

// Button Mapping (Arduino ESP32)
const int BTN_UP_PIN = 32;
const int BTN_DOWN_PIN = 27;
const int BTN_LEFT_PIN = 33;
const int BTN_RIGHT_PIN = 14;
const int BTN_A_PIN = 13;
const int BTN_B_PIN = 12;

pr32::drivers::esp32::ESP32_I2S_AudioBackend audioBackend(I2S_BCLK, I2S_LRCK, I2S_DOUT, 22050);

pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::ST7789,
    DISPLAY_ROTATION,
    PHYSICAL_DISPLAY_WIDTH,
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_RIGHT_PIN, BTN_A_PIN, BTN_B_PIN);

pr32::audio::AudioConfig audioConfig(&audioBackend, audioBackend.getSampleRate());

pr32::core::Engine engine(config, inputConfig, audioConfig);

iso_dungeon::IsoDungeonScene isoDungeonScene;

void setup() {
    engine.init();
    engine.setScene(&isoDungeonScene);
}

void loop() {
    engine.run();
}

#endif
