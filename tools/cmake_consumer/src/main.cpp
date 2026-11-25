/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Smallest program that proves the installed package is usable:
 *
 *   - the headers resolve out of the install prefix (nothing is included from
 *     the engine's source tree),
 *   - SDL2 arrives transitively through PixelRoot32::Engine, so a consumer does
 *     not repeat the SDL2 discovery the engine already did,
 *   - the exported compile definitions arrive too, which is what lets a game
 *     write `#ifdef PLATFORM_NATIVE` the way the examples do.
 *
 * It builds the engine, installs a scene and prints its configuration. It does
 * not call Engine::init() or Engine::run(): those open a window, and this check
 * runs headless. The link itself is the assertion — SDL2_Drawer and the engine
 * are both reachable only through the package.
 */

// PLATFORM_NATIVE comes from PixelRoot32::Engine's INTERFACE_COMPILE_DEFINITIONS.
// If this line ever fires, the package stopped exporting its configuration and
// every consumer would silently compile the ESP32 branch instead.
#ifndef PLATFORM_NATIVE
#error "PLATFORM_NATIVE was not exported by PixelRoot32::Engine"
#endif

#include <cstdio>

#include <SDL2/SDL.h>

#include <core/Engine.h>
#include <drivers/native/SDL2_Drawer.h>
#include <graphics/Color.h>
#include <graphics/DisplayConfig.h>
#include <input/InputConfig.h>
#include <platforms/EngineConfig.h>

namespace {

class SwatchScene final : public pixelroot32::core::Scene {
public:
    void update(unsigned long deltaTime) override {
        (void)deltaTime;
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawFilledRectangle(8, 8, 32, 32,
                                     pixelroot32::graphics::Color::White);
    }
};

} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    namespace pr32 = pixelroot32;

    pr32::graphics::DisplayConfig display(
        pr32::graphics::DisplayType::NONE,
        DISPLAY_ROTATION,
        PHYSICAL_DISPLAY_WIDTH,
        PHYSICAL_DISPLAY_HEIGHT,
        LOGICAL_WIDTH,
        LOGICAL_HEIGHT,
        X_OFF_SET,
        Y_OFF_SET);

    pr32::input::InputConfig input(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN,
                                   SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
                                   SDL_SCANCODE_SPACE, SDL_SCANCODE_RETURN);

    pr32::core::Engine engine(display, input);
    SwatchScene scene;
    engine.setScene(&scene);

    SDL_version linked{};
    SDL_GetVersion(&linked);

    // PR32_PACKAGE_VERSION is set from ${PixelRoot32_VERSION} by the consumer's
    // CMakeLists, so this line also proves the version file resolved.
    std::printf("pixelroot32 %s via find_package\n", PR32_PACKAGE_VERSION);
    std::printf("display %dx%d logical %dx%d\n",
                PHYSICAL_DISPLAY_WIDTH, PHYSICAL_DISPLAY_HEIGHT,
                LOGICAL_WIDTH, LOGICAL_HEIGHT);
    std::printf("audio=%d physics=%d ui=%d particles=%d\n",
                PIXELROOT32_ENABLE_AUDIO,
                PIXELROOT32_ENABLE_PHYSICS,
                PIXELROOT32_ENABLE_UI_SYSTEM,
                PIXELROOT32_ENABLE_PARTICLES);
    std::printf("sdl2 linked %d.%d.%d\n", linked.major, linked.minor,
                linked.patch);

    return 0;
}
