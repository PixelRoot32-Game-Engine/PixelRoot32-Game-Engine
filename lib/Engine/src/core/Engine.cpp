#include "core/Engine.h"
#include "input/InputConfig.h"

namespace pixelroot32::core {

    using namespace pixelroot32::graphics;
    using namespace pixelroot32::input;

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig) : renderer(displayConfig), inputManager(inputConfig) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig) : renderer(displayConfig), inputManager(InputConfig(0)) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::~Engine() {}

    void Engine::init() {
        // Initialize Serial for debugging
        Serial.begin(115200);
        delay(100);
        
        renderer.init();
        inputManager.init();
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
    }

    void Engine::draw() {
        renderer.beginFrame();
        sceneManager.draw(renderer);
        renderer.endFrame();
    }
}
