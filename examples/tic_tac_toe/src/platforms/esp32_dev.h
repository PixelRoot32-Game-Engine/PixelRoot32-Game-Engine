#ifdef PLATFORM_ESP32DEV

#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <drivers/esp32/ESP32_I2S_AudioBackend.h> // Uncomment for I2S (MAX98357A)
//#include <drivers/esp32/ESP32_DAC_AudioBackend.h> // Uncomment for DAC (PAM8302A)
#include  <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "TicTacToeScene.h"

namespace pr32 = pixelroot32;

// Audio Pin Configuration (I2S)
// Common mapping for MAX98357A or similar I2S DACs
const int I2S_BCLK = 26;
const int I2S_LRCK = 25;
const int I2S_DOUT = 22;

// Audio Pin Configuration (DAC)
const int DAC_PIN = 25; // GPIO 25 or 26

// Button Mapping (Arduino ESP32)
// Common mapping for 5-directional pad and A button
const int BTN_UP = 32;
const int BTN_DOWN = 27;
const int BTN_LEFT = 33;
const int BTN_RIGHT = 14;
const int BTN_A = 13;
const int BTN_B = 12;

// Select Audio Backend
// Option A: I2S (High quality, external DAC like MAX98357A)
pr32::drivers::esp32::ESP32_I2S_AudioBackend audioBackend(I2S_BCLK, I2S_LRCK, I2S_DOUT, 22050);

// Option B: Internal DAC (Retro quality, internal DAC + PAM8302A)
//pr32::drivers::esp32::ESP32_DAC_AudioBackend audioBackend(DAC_PIN, 11025); // Lower sample rate for simple dacWrite stability

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

pr32::input::InputConfig inputConfig(6, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B); // 6 buttons: Up, Down, Left, Right, A, B

pr32::audio::AudioConfig audioConfig(&audioBackend, audioBackend.getSampleRate());

pr32::core::Engine engine(config, inputConfig, audioConfig);

tictactoe::TicTacToeScene ticTacToeScene;

void setup() {
    engine.init();
    engine.setScene(&ticTacToeScene);
}

void loop() {
    engine.run();
}

#endif
