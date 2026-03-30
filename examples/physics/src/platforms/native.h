#ifdef PLATFORM_NATIVE

#include <SDL2/SDL.h>

#include <drivers/native/SDL2_Drawer.h> 
#include <drivers/native/SDL2_AudioBackend.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "PhysicsDemoScene.h"

namespace pr32 = pixelroot32;

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

pr32::core::Engine engine(config, inputConfig);

physicsdemo::PhysicsDemoScene physicsScene;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    engine.init();
    engine.setScene(&physicsScene);

    engine.run();

    return 0;
}

#endif // PLATFORM_NATIVE
