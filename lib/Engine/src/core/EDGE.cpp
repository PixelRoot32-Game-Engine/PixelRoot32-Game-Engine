#include "core/EDGE.h"
#include "input/InputConfig.h"

EDGE::EDGE(const DisplayConfig& displayConfig, const InputConfig& inputConfig) : renderer(displayConfig), inputManager(inputConfig) {
    previousMillis = 0;
    deltaTime = 0;
}

EDGE::EDGE(const DisplayConfig& displayConfig) : renderer(displayConfig), inputManager(InputConfig(0)) {
    previousMillis = 0;
    deltaTime = 0;
}

EDGE::~EDGE() {}

void EDGE::init() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(100);
    
    renderer.init();
    inputManager.init();
}

void EDGE::run() {
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

void EDGE::setScene(Scene* newScene) {
    sceneManager.setCurrentScene(newScene);
}

Renderer& EDGE::getRenderer() {
    return renderer;
}

void EDGE::update() {
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

void EDGE::draw() {
    renderer.beginFrame();
    sceneManager.draw(renderer);
    renderer.endFrame();
}
