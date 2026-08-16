#ifdef PLATFORM_NATIVE

#include <SDL2/SDL.h>

#include <drivers/native/SDL2_Drawer.h>
#include <drivers/native/SDL2_AudioBackend.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "BomberbotScene.h"
#include "TitleScreenScene.h"
#include "audio/AudioDirector.h"

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

pr32::input::InputConfig inputConfig(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE, SDL_SCANCODE_RETURN); // 6 buttons: Up, Down, Left, Right, Space (bomb), Enter (restart)

pr32::audio::AudioConfig audioConfig(&audioBackend, audioBackend.getSampleRate());

pr32::core::Engine engine(config, inputConfig, audioConfig);

bomberbot::BomberbotScene bomberbotScene;
bomberbot::TitleScreenScene titleScreenScene;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    engine.init();

#if PIXELROOT32_ENABLE_AUDIO
    bomberbot::AudioDirector::instance().bind(&engine.getAudioEngine());
#endif

    // Title screen first, then bomberbotScene on START. Wired statically
    // because the engine is a single global in this example.
    bomberbot::TitleScreenScene::setNextScene(&bomberbotScene);
    engine.setScene(&titleScreenScene);

    engine.run();

    return 0;
}

#endif // PLATFORM_NATIVE
