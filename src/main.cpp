#include <Arduino.h>
#include "EDGE.h"
// #include "examples/Pong/PongScene.h"
#include "examples/BrickBraker/BrickBreakerScene.h"
//#include "examples/Snake/SnakeScene.h"

#define I2C_FREQUENCY 400000
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240


DisplayConfig config(SCREEN_WIDTH, SCREEN_HEIGHT);
InputConfig inputConfig(4, 13, 12, 14, 32); // 4 buttons: A, Left, Right, Up

EDGE engine(config, inputConfig);

// PongScene pongScene;
BrickBreakerScene brickBreakerScene;
//SnakeScene snakeScene;

void setup() {
    engine.init();
    engine.setScene(&brickBreakerScene);
}

void loop() {
    engine.run();
}