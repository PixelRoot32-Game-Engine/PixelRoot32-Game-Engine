/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#include "core/Engine.h"
#include "input/InputConfig.h"
#include "graphics/FontManager.h"
#include "graphics/Font5x7.h"

namespace pixelroot32::core {

    using namespace pixelroot32::graphics;
    using namespace pixelroot32::input;
    using namespace pixelroot32::audio;

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig) 
        : renderer(displayConfig), inputManager(inputConfig), audioEngine(audioConfig), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig) 
        : renderer(displayConfig), inputManager(inputConfig), audioEngine(AudioConfig()), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig) 
        : renderer(displayConfig), inputManager(InputConfig(0)), audioEngine(AudioConfig()), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
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
        renderer.endFrame();
    }
}
