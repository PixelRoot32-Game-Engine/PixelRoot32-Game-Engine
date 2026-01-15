#ifdef PLATFORM_NATIVE

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "Engine.h"
#include "examples/Pong/PongScene.h"
//#include "examples/BrickBraker/BrickBreakerScene.h"
//#include "examples/Snake/SnakeScene.h"

#define I2C_SDA 0 // Not used
#define I2C_SCL 0 // Not used
#define I2C_CS 0 // Not used
#define I2C_DC 0 // Not used
#define I2C_RESET 0 // Not used
#define I2C_FREQUENCY // Not used
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

namespace pr32 = pixelroot32;

pr32::graphics::DisplayConfig config(SCREEN_WIDTH,SCREEN_HEIGHT);

pr32::input::InputConfig inputConfig(5, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE); // 5 buttons: Up, Down, Left, Right, Space

pr32::core::Engine engine(config, inputConfig);

pong::PongScene pongScene;
//BrickBreakerScene brickBreakerScene;
//SnakeScene snakeScene;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    engine.init();
    engine.setScene(&pongScene);

    engine.run();

    return 0;
}

#endif // PLATFORM_NATIVE