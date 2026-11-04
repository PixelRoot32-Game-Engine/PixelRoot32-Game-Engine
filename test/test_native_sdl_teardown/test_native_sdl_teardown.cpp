/**
 * @file test_native_sdl_teardown.cpp
 * @brief Issue #273: the native SDL2 teardown must not depend on the order the
 *        process globals are destroyed in.
 *
 * The documented `platforms/native.h` pattern declares the `SDL2_AudioBackend`
 * at namespace scope *before* the `Engine`, so the Engine -- and the
 * `SDL2_Drawer` it owns -- is destroyed first and the backend last. The drawer
 * used to call `SDL_Quit()` from its destructor, which tore down the whole SDL
 * library, including the audio subsystem the backend had initialised and the
 * device handle it closed afterwards. Roughly 40% of window closes on Windows
 * aborted with `0xC0000005` while the process tore down static storage.
 *
 * The contract pinned here is narrower than "SDL is gone": each native driver
 * owns exactly the subsystem it initialised. The drawer quits video, the audio
 * backend closes its device and quits audio, and neither one can tear the other
 * down. That is what makes every declaration order safe.
 *
 * Headless by construction: `setUp()` selects SDL's dummy drivers, so the suite
 * needs neither a display nor a sound card (CI included).
 */

#include <unity.h>

#include <SDL2/SDL.h>

#include "audio/AudioConfig.h"
#include "core/Engine.h"
#include "drivers/native/SDL2_AudioBackend.h"
#include "drivers/native/SDL2_Drawer.h"
#include "graphics/DisplayConfig.h"
#include "input/InputConfig.h"

namespace {

using pixelroot32::audio::AudioConfig;
using pixelroot32::core::Engine;
using pixelroot32::drivers::native::SDL2_AudioBackend;
using pixelroot32::drivers::native::SDL2_Drawer;
using pixelroot32::graphics::DisplayConfig;
using pixelroot32::graphics::DisplayType;
using pixelroot32::input::InputConfig;

/// Sample rate and block the audio demos use on the native target.
constexpr int kSampleRate = 22050;
constexpr int kBufferSize = 1024;
/// Square panel size; only the framebuffer geometry depends on it.
constexpr uint16_t kSide = 240;

/**
 * @brief Builds the native display config the demos use.
 *
 * `DisplayType::NONE` is the native "no panel" case: on `PLATFORM_NATIVE` the
 * factory still creates the real `SDL2_Drawer`, which is exactly the surface
 * this suite has to see destroyed before the audio backend.
 * @return A display config owning a fresh SDL2 surface.
 */
DisplayConfig makeDisplayConfig() {
    return DisplayConfig(DisplayType::NONE, 0, kSide, kSide, kSide, kSide, 0, 0);
}

}  // namespace

void setUp(void) {
    // Dummy drivers: no display, no sound card, and the same code path the
    // demos take through SDL's own subsystem states.
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
}

void tearDown(void) {}

/**
 * @brief The demos' declaration order: backend first, Engine second.
 *
 * Destroying the Engine first must leave the audio subsystem the backend owns
 * untouched, and the backend must then close its device and quit that
 * subsystem without ever reaching SDL state the drawer already freed.
 */
void test_engine_destroyed_first_leaves_audio_alive(void) {
    auto* backend = new SDL2_AudioBackend(kSampleRate, kBufferSize);
    auto* engine = new Engine(makeDisplayConfig(), InputConfig(),
                              AudioConfig(backend, kSampleRate));

    engine->init();
    TEST_ASSERT_NOT_EQUAL(0, SDL_WasInit(SDL_INIT_VIDEO));
    TEST_ASSERT_NOT_EQUAL(0, SDL_WasInit(SDL_INIT_AUDIO));

    // Reverse order of construction: the Engine owns the drawer, so this is the
    // destruction that used to call SDL_Quit() with the device still open.
    delete engine;
    TEST_ASSERT_EQUAL(0, SDL_WasInit(SDL_INIT_VIDEO));
    TEST_ASSERT_NOT_EQUAL(0, SDL_WasInit(SDL_INIT_AUDIO));

    // Reaching the next line at all is the regression assertion: the close used
    // to fault here because the handle outlived the SDL state behind it.
    delete backend;
    TEST_ASSERT_EQUAL(0, SDL_WasInit(SDL_INIT_AUDIO));
}

/**
 * @brief The opposite order: the audio backend goes before the drawer.
 *
 * Here the backend closes its own device and quits audio, and the drawer must
 * then quit only the video subsystem it initialised. Before the fix this order
 * happened to work by accident, because `SDL_Quit()` quits everything and the
 * drawer ran last; pinning it keeps the pairing symmetric instead of lucky.
 */
void test_backend_destroyed_first_leaves_video_alive(void) {
    SDL2_Drawer drawer;
    drawer.init();
    TEST_ASSERT_NOT_EQUAL(0, SDL_WasInit(SDL_INIT_VIDEO));

    {
        SDL2_AudioBackend backend(kSampleRate, kBufferSize);
        // A null engine is a supported silent backend: the callback writes
        // silence, and only the SDL side of the lifetime is under test. The
        // capabilities parameter is spelled out because an override does not
        // inherit its base's default arguments.
        backend.init(nullptr, pixelroot32::platforms::PlatformCapabilities());
        TEST_ASSERT_NOT_EQUAL(0, SDL_WasInit(SDL_INIT_AUDIO));
    }

    TEST_ASSERT_EQUAL(0, SDL_WasInit(SDL_INIT_AUDIO));
    TEST_ASSERT_NOT_EQUAL(0, SDL_WasInit(SDL_INIT_VIDEO));

    // The drawer is still alive here: it must quit only its own subsystem. Its
    // destructor runs at the end of this function.
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_engine_destroyed_first_leaves_audio_alive);
    RUN_TEST(test_backend_destroyed_first_leaves_video_alive);
    return UNITY_END();
}
