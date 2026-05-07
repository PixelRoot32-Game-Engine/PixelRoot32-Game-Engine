#ifdef PLATFORM_NATIVE

#include <SDL2/SDL.h>

#include <drivers/native/SDL2_Drawer.h>
#include <drivers/native/SDL2_AudioBackend.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "Game2048Constants.h"
#include "Game2048Scene.h"

namespace pr32 = pixelroot32;

pr32::drivers::native::SDL2_AudioBackend audioBackend(22050, 1024);

pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::NONE,
    DISPLAY_ROTATION,
    PHYSICAL_DISPLAY_WIDTH,
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(6, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE, SDL_SCANCODE_RETURN); // 6 buttons: Up, Down, Left, Right, Space(A), Enter (B)

pr32::audio::AudioConfig audioConfig(&audioBackend, 22050);

pr32::core::Engine engine(config, inputConfig, audioConfig);

// Game2048 Scene - implements input handling, rendering, and game states
game2048::Game2048Scene game2048Scene;


int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    engine.init();
    engine.setScene(&game2048Scene);

    engine.run();

    return 0;
}

#endif // PLATFORM_NATIVE