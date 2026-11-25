#include <platforms/PlatformSelect.h>

#if PIXELROOT32_PLATFORM_WEB

#include <SDL2/SDL.h>

#include <core/Engine.h>
#include <drivers/native/SDL2_Drawer.h>
#include <platforms/EngineConfig.h>
#include <platforms/WebStorage.h>

#include "PersistenceExampleScene.h"

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

// Six buttons: Up, Down, Left, Right, A (Space), B (Enter). The pointer is
// separate: with PIXELROOT32_ENABLE_TOUCH defaulting to 1 on the web it arrives
// as touch points, which the scene handles as a tap.
pr32::input::InputConfig inputConfig(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                                     SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE,
                                     SDL_SCANCODE_RETURN);

pr32::core::Engine engine(config, inputConfig);

persistenceexample::PersistenceExampleScene scene;

int main() {
    // Mounts IDBFS and starts loading what the last visit saved. The load is
    // asynchronous, so the scene waits for isStorageReady() before reading
    // (see PersistenceExampleScene::init/update) instead of treating a save that
    // exists as missing.
    pr32::platforms::web::initPersistentStorage();

    engine.init();
    engine.setScene(&scene);

    // Returns immediately on the web: Engine::run() registers the frame loop and
    // unwinds, which is why nothing here may run after it.
    engine.run();

    return 0;
}

#endif  // PIXELROOT32_PLATFORM_WEB
