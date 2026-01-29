/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "core/Engine.h"
#include "input/InputConfig.h"
#include "graphics/FontManager.h"
#include "graphics/Font5x7.h"
#include "graphics/Color.h"
#include <cstdio>
#include <cstring>

namespace pixelroot32::core {

    using namespace pixelroot32::graphics;
    using namespace pixelroot32::input;
    using namespace pixelroot32::audio;

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig) 
        : renderer(displayConfig), inputManager(inputConfig), audioEngine(audioConfig), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_FPS_DISPLAY
        std::strcpy(fpsOverlayBuf, "FPS 0");
        fpsUpdateCounter = 0;
#endif
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig) 
        : renderer(displayConfig), inputManager(inputConfig), audioEngine(AudioConfig()), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_FPS_DISPLAY
        std::strcpy(fpsOverlayBuf, "FPS 0");
        fpsUpdateCounter = 0;
#endif
    }

    Engine::Engine(const DisplayConfig& displayConfig) 
        : renderer(displayConfig), inputManager(InputConfig(0)), audioEngine(AudioConfig()), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_FPS_DISPLAY
        std::strcpy(fpsOverlayBuf, "FPS 0");
        fpsUpdateCounter = 0;
#endif
    }

    Engine::~Engine() {}

    void Engine::init() {
        // Initialize Serial for debugging (ESP32 only)
        #ifndef PLATFORM_NATIVE
            Serial.begin(115200);
            delay(100);
        #endif
        
        renderer.init();
        inputManager.init();
        audioEngine.init();
        
        // Set default font (5x7 bitmap font)
        FontManager::setDefaultFont(&FONT_5X7);
    }

    void Engine::run() {
        DrawSurface* drawer = static_cast<DrawSurface*>(&renderer.getDrawSurface());
        
        #ifdef PLATFORM_NATIVE
            bool running = true;

            while (running) {
                // Process SDL events
                running = drawer->processEvents();

                update();
                draw();

                // Present frame (SDL2)
                drawer->present();

                SDL_Delay(1);
            }
        #else 
            update();

            // waitForDMA
            drawer->processEvents();

            draw();

            // Present frame (TFT_eSPI)
            drawer->present();

        #endif // PLATFORM_NATIVE
    }

    void Engine::setScene(Scene* newScene) {
        sceneManager.setCurrentScene(newScene);
    }

    Renderer& Engine::getRenderer() {
        return renderer;
    }

    void Engine::update() {
        unsigned long currentMillis = millis();
        deltaTime = currentMillis - previousMillis;
        previousMillis = currentMillis;

    #ifdef PLATFORM_NATIVE
        inputManager.update(deltaTime, SDL_GetKeyboardState(nullptr));
    #else
        inputManager.update(deltaTime);
    #endif
        sceneManager.update(deltaTime);
        audioEngine.update(deltaTime);
        musicPlayer.update(deltaTime);
    }

    void Engine::draw() {
        renderer.beginFrame();
        sceneManager.draw(renderer);
#ifdef PIXELROOT32_ENABLE_FPS_DISPLAY
        drawFpsOverlay(renderer);
#endif
        renderer.endFrame();
    }

#ifdef PIXELROOT32_ENABLE_FPS_DISPLAY
    void Engine::drawFpsOverlay(Renderer& r) {
        if (++fpsUpdateCounter >= FPS_UPDATE_INTERVAL) {
            fpsUpdateCounter = 0;
            unsigned int fps = (deltaTime > 0) ? (1000u / static_cast<unsigned int>(deltaTime)) : 0;
            if (fps > 999) fps = 999;
            std::snprintf(fpsOverlayBuf, sizeof(fpsOverlayBuf), "FPS %u", fps);
        }
        int x = r.getWidth() - 48;
        if (x < 0) x = 0;
        r.drawText(fpsOverlayBuf, static_cast<int16_t>(x), 12, Color::Green, 1);
    }
#endif
}
