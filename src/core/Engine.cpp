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

    Engine::Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig) 
        : renderer(std::move(displayConfig)), inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(audioConfig, capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
        debugUpdateCounter = 0;
        debugAccumulatedMs = 0;
        std::strcpy(fpsStr, "FPS: 0");
        std::strcpy(ramStr, "RAM: 0K");
        std::strcpy(cpuStr, "CPU: 0%");
#endif
    }

    Engine::Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig) 
        : renderer(std::move(displayConfig)), inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
        debugUpdateCounter = 0;
        debugAccumulatedMs = 0;
        std::strcpy(fpsStr, "FPS: 0");
        std::strcpy(ramStr, "RAM: 0K");
        std::strcpy(cpuStr, "CPU: 0%");
#endif
    }

    Engine::Engine(DisplayConfig&& displayConfig) 
        : renderer(std::move(displayConfig)), inputManager(InputConfig(0)), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
        debugUpdateCounter = 0;
        debugAccumulatedMs = 0;
        std::strcpy(fpsStr, "FPS: 0");
        std::strcpy(ramStr, "RAM: 0K");
        std::strcpy(cpuStr, "CPU: 0%");
#endif
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(audioConfig, capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
        debugUpdateCounter = 0;
        debugAccumulatedMs = 0;
        std::strcpy(fpsStr, "FPS: 0");
        std::strcpy(ramStr, "RAM: 0K");
        std::strcpy(cpuStr, "CPU: 0%");
#endif
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
        debugUpdateCounter = 0;
        debugAccumulatedMs = 0;
        std::strcpy(fpsStr, "FPS: 0");
        std::strcpy(ramStr, "RAM: 0K");
        std::strcpy(cpuStr, "CPU: 0%");
#endif
    }

    Engine::Engine(const DisplayConfig& displayConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(InputConfig(0)), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
        debugUpdateCounter = 0;
        debugAccumulatedMs = 0;
        std::strcpy(fpsStr, "FPS: 0");
        std::strcpy(ramStr, "RAM: 0K");
        std::strcpy(cpuStr, "CPU: 0%");
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
            static uint32_t lastHeartbeat = 0;
            if (millis() - lastHeartbeat > 1000) {
                Serial.println("[Engine] Heartbeat...");
                lastHeartbeat = millis();
            }

            update();

            // waitForDMA
            drawer->processEvents();

            draw();

            // Present frame (TFT_eSPI)
            drawer->present();

            // Yield to avoid starving Core 1 system tasks
            // vTaskDelay(1); // REMOVED: Adds 1ms-10ms latency (1 tick) which limits FPS significantly.
            yield(); // Use yield() instead to feed Watchdog without forcing a full tick wait.

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
#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
        drawDebugOverlay(renderer);
#endif
        renderer.endFrame();
    }

#ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
    void Engine::drawDebugOverlay(Renderer& r) {
        debugAccumulatedMs += deltaTime;
        
        if (++debugUpdateCounter >= DEBUG_UPDATE_INTERVAL) {
            // 1. Calculate FPS
            unsigned int fps = 0;
            if (debugAccumulatedMs > 0) {
                fps = (1000u * static_cast<unsigned int>(DEBUG_UPDATE_INTERVAL)) / static_cast<unsigned int>(debugAccumulatedMs);
                if (fps > 999) fps = 999;
            }
            std::snprintf(fpsStr, sizeof(fpsStr), "FPS: %u", fps);

            // 2. Calculate RAM Usage
            #ifdef PLATFORM_NATIVE
                // On PC/Native, actual RAM usage is complex, show a placeholder or static info
                std::strcpy(ramStr, "RAM: N/A");
            #else
                // On ESP32
                uint32_t freeHeap = ESP.getFreeHeap();
                uint32_t totalHeap = ESP.getHeapSize();
                uint32_t usedHeapK = (totalHeap - freeHeap) / 1024;
                std::snprintf(ramStr, sizeof(ramStr), "RAM: %uK", usedHeapK);
            #endif

            // 3. Calculate "CPU Usage" (Estimated based on frame time)
            // This is a simplified metric: (Processing Time / Target Frame Time)
            // Assuming 60 FPS target (16.6ms)
            float load = (float)debugAccumulatedMs / (DEBUG_UPDATE_INTERVAL * 16.6f);
            int cpuPercent = (int)(load * 100);
            if (cpuPercent > 100) cpuPercent = 100;
            std::snprintf(cpuStr, sizeof(cpuStr), "CPU: %d%%", cpuPercent);

            debugUpdateCounter = 0;
            debugAccumulatedMs = 0;
        }

        // Render Overlay (Independent of camera/scrolling)
        // Since this is called at the end of draw(), we render directly on top
        
        // Save current offset to restore it later (though endFrame follows)
        int oldX = r.getXOffset();
        int oldY = r.getYOffset();
        r.setDisplayOffset(0, 0);

        int16_t x = r.getWidth() - 55;
        if (x < 0) x = 0;
        
        r.drawText(fpsStr, x, 4, Color::Green, 1);
        r.drawText(ramStr, x, 12, Color::Cyan, 1);
        r.drawText(cpuStr, x, 20, Color::Yellow, 1);

        // Restore offset
        r.setDisplayOffset(oldX, oldY);
    }
#endif
}
