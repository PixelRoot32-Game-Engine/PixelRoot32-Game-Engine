#include <Arduino.h>
#include "Engine.h"
#include <drivers/esp32/TFT_eSPI_Drawer.h> 
#include "examples/Pong/PongScene.h"


#define I2C_FREQUENCY 400000
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

namespace pr32 = pixelroot32;

pr32::drivers::esp32::TFT_eSPI_Drawer drawer;

pr32::graphics::DisplayConfig config(&drawer, SCREEN_WIDTH, SCREEN_HEIGHT);

pr32::input::InputConfig inputConfig(4, 13, 12, 14, 32); // 4 buttons: A, Left, Right, Up

pr32::core::Engine engine(config, inputConfig);

pong::PongScene pongScene;


void setup() {
    engine.init();
    engine.setScene(&pongScene);
}

void loop() {
    engine.run();
}