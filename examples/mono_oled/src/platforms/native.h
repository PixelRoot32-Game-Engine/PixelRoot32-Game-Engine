#ifdef PLATFORM_NATIVE

#include <SDL2/SDL.h>

#include <drivers/native/SDL2_Drawer.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "MonoOledScene.h"

namespace pr32 = pixelroot32;

// The simulator is configured with the same geometry as the target — a 72x40
// logical screen offset inside a 128x64 surface — but not the same panel.
// SDL2_Drawer is a colour surface, so the 1-bit constraint is not enforced
// here: a draw call with a colour other than Black or White will show that
// colour on the PC and collapse to ink-or-nothing on the Beetle.
//
// That is why the scene draws in White only. Use native to check layout and
// logic; use the hardware to check the display.
//
// NOTE: this environment is currently unverified — see the "Known
// limitation" section of the README. The MSYS2/MinGW toolchain on the
// development machine fails to build any example's native target, this one
// included, so the geometry claim above is read from the configuration
// rather than observed on screen.
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

// One button, to match the hardware. Declaring the usual six here would let
// a contributor reach for a D-pad the target board does not have.
pr32::input::InputConfig inputConfig(SDL_SCANCODE_SPACE); // 1 button: next page

pr32::core::Engine engine(config, inputConfig);

mono_oled::MonoOledScene monoOledScene;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    engine.init();
    engine.setScene(&monoOledScene);

    engine.run();

    return 0;
}

#endif // PLATFORM_NATIVE
