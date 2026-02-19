/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "core/Engine.h"
#include "core/Scene.h"
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

    unsigned long gProfilerCollisionTime = 0;
    unsigned long gProfilerPhysicsIntegrateTime = 0;
    unsigned long gProfilerPhysicsIntegrateCount = 0;

    Engine::Engine(pixelroot32::graphics::DisplayConfig&& displayConfig, const pixelroot32::input::InputConfig& inputConfig, const pixelroot32::audio::AudioConfig& audioConfig) 
        : renderer(std::move(displayConfig)), inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(audioConfig, capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig) 
        : renderer(std::move(displayConfig)), inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(DisplayConfig&& displayConfig) 
        : renderer(std::move(displayConfig)), inputManager(InputConfig(0)), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(audioConfig, capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(inputConfig), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(InputConfig(0)), capabilities(PlatformCapabilities::detect()), audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine) {
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
            static uint32_t lastHeartbeat = 0;

            static uint32_t frameCount = 0;
            static uint32_t totalUpdateTime = 0;
            static uint32_t totalDrawTime = 0;
            static uint32_t totalPresentTime = 0;
            static uint32_t totalEventsTime = 0;
            uint32_t t0 = 0;
            if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                t0 = pixelroot32::platforms::config::profilerMicros();
            }

            if (millis() - lastHeartbeat > 1000) {
                if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                    Serial.println("[Engine] Heartbeat...");
                    if (frameCount > 0) {
                        unsigned long avgCollision = 0;
                        if (gProfilerCollisionTime > 0) {
                            avgCollision = gProfilerCollisionTime / frameCount;
                        }
                        unsigned long avgPhysicsIntegrate = 0;
                        if (gProfilerPhysicsIntegrateCount > 0) {
                            avgPhysicsIntegrate = gProfilerPhysicsIntegrateTime / gProfilerPhysicsIntegrateCount;
                        }
                        unsigned long physicsIntegrateCount = gProfilerPhysicsIntegrateCount;

                        Serial.printf("[Profiler] FPS: %d | Update: %dus | Events: %dus | Draw: %dus | Present: %dus | Collision: %luus | PhysicsInt: %luus (%lu)\n",
                            frameCount,
                            totalUpdateTime / frameCount,
                            totalEventsTime / frameCount,
                            totalDrawTime / frameCount,
                            totalPresentTime / frameCount,
                            avgCollision,
                            avgPhysicsIntegrate,
                            physicsIntegrateCount
                        );
                        frameCount = 0;
                        totalUpdateTime = 0;
                        totalDrawTime = 0;
                        totalPresentTime = 0;
                        totalEventsTime = 0;
                        gProfilerCollisionTime = 0;
                        gProfilerPhysicsIntegrateTime = 0;
                        gProfilerPhysicsIntegrateCount = 0;
                    }
                }
                lastHeartbeat = millis();
            }

            update();

            uint32_t t1 = 0;
            if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                t1 = pixelroot32::platforms::config::profilerMicros();
            }

            // waitForDMA
            drawer->processEvents();

            uint32_t t2 = 0;
            if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                t2 = pixelroot32::platforms::config::profilerMicros();
            }

            draw();

            uint32_t t3 = 0;
            if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                t3 = pixelroot32::platforms::config::profilerMicros();
            }

            // Present frame (TFT_eSPI)
            drawer->present();

            if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                uint32_t t4 = pixelroot32::platforms::config::profilerMicros();
                totalUpdateTime += (t1 - t0);
                totalEventsTime += (t2 - t1);
                totalDrawTime += (t3 - t2);
                totalPresentTime += (t4 - t3);
                frameCount++;
            }

            yield();

        #endif // PLATFORM_NATIVE
    }

    void Engine::setScene(Scene* newScene) {
        sceneManager.setCurrentScene(newScene);
    }

    Renderer& Engine::getRenderer() {
        return renderer;
    }

    unsigned long Engine::getMillis() const {
        return millis();
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
        if constexpr (pixelroot32::platforms::config::EnableDebugOverlay) {
            drawDebugOverlay(renderer);
        }
    }

    void Engine::drawDebugOverlay(pixelroot32::graphics::Renderer& r) {
        if constexpr (pixelroot32::platforms::config::EnableDebugOverlay) {
            debugAccumulatedMs += deltaTime;
            debugUpdateCounter++;

            if (debugUpdateCounter >= DEBUG_UPDATE_INTERVAL) {
                if (debugAccumulatedMs > 0) {
                    float fps = (1000.0f * debugUpdateCounter) / debugAccumulatedMs;
                    std::snprintf(fpsStr, sizeof(fpsStr), "FPS: %.1f", fps);
                }

                #ifdef PLATFORM_NATIVE
                    std::strcpy(ramStr, "RAM: N/A");
                #else
                    uint32_t freeHeap = ESP.getFreeHeap();
                    uint32_t totalHeap = ESP.getHeapSize();
                    uint32_t usedHeapK = (totalHeap - freeHeap) / 1024;
                    std::snprintf(ramStr, sizeof(ramStr), "RAM: %uK", usedHeapK);
                #endif

                float load = (float)debugAccumulatedMs / (DEBUG_UPDATE_INTERVAL * 16.6f);
                int cpuPercent = (int)(load * 100);
                if (cpuPercent > 100) cpuPercent = 100;
                std::snprintf(cpuStr, sizeof(cpuStr), "CPU: %d%%", cpuPercent);

                debugUpdateCounter = 0;
                debugAccumulatedMs = 0;
            }
            
            int oldX = r.getXOffset();
            int oldY = r.getYOffset();
            r.setDisplayOffset(0, 0);

            int16_t x = r.getWidth() - 55;
            if (x < 0) x = 0;
            
            r.drawText(fpsStr, x, 4, Color::Green, 1);
            r.drawText(ramStr, x, 12, Color::Cyan, 1);
            r.drawText(cpuStr, x, 20, Color::Yellow, 1);

            r.setDisplayOffset(oldX, oldY);
        }
    }
}
