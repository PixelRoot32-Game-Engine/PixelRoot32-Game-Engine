#ifdef PLATFORM_NATIVE

#include <SDL2/SDL.h>

#include <drivers/native/SDL2_Drawer.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "EspNowDemoScene.h"

namespace pr32 = pixelroot32;

// The simulator shows two 72x40 panels side by side: 144x40 logical pixels
// rendered into a 432x120 physical surface, which SDL2 opens at 2x (864x240).
// It reproduces the layout and the protocol, not the panel: SDL2_Drawer is a
// colour surface, so the 1-bit constraint is not enforced here. The scene draws
// in White only, which is what the Beetle's OLED actually shows.
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

// Two buttons, matching the two simulated boards: key 1 drives panel A, key 2
// drives panel B. Declaring one button would make the second simulated node
// unreachable; declaring six would suggest a control scheme the target lacks.
pr32::input::InputConfig inputConfig(SDL_SCANCODE_1, SDL_SCANCODE_2);

pr32::core::Engine engine(config, inputConfig);

esp_now_demo::EspNowDemoScene demoScene;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    engine.init();

    // Scene::init() starts both loopback transports and registers the primary
    // session with the Engine. The scene pumps the secondary board itself
    // because one Engine dispatches one session.
    engine.setScene(&demoScene);

    engine.run();

    return 0;
}

#endif // PLATFORM_NATIVE
