/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "core/Engine.h"
#include "core/Scene.h"
#include "core/EngineModules.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"
#include "audio/ApuCore.h"
#include "input/InputConfig.h"
#include "input/TouchManager.h"
#include "graphics/FontManager.h"
#include "graphics/Font5x7.h"
#include "graphics/Color.h"
#include "math/Scalar.h"
#include <cstdio>
#include <cstring>
#include <cassert>

#ifdef PLATFORM_NATIVE
#include "drivers/native/SDL2_Drawer.h"
#endif

namespace pixelroot32::core {

    namespace modules = pixelroot32::modules;
    namespace gfx = pixelroot32::graphics;
    namespace input = pixelroot32::input;
    namespace audio = pixelroot32::audio;
    namespace drivers = pixelroot32::drivers;

    using gfx::DisplayConfig;
    using input::InputConfig;
    using audio::AudioConfig;
    using gfx::Renderer;
    using gfx::FontManager;
    using gfx::FONT_5X7;
    using gfx::DrawSurface;
    using gfx::Color;
    using logging::LogLevel;
    using logging::log;

    unsigned long gProfilerCollisionTime = 0;
    unsigned long gProfilerPhysicsIntegrateTime = 0;
    unsigned long gProfilerPhysicsIntegrateCount = 0;

    Engine::Engine(pixelroot32::graphics::DisplayConfig&& displayConfig, const pixelroot32::input::InputConfig& inputConfig, const pixelroot32::audio::AudioConfig& audioConfig) 
        : renderer(std::move(displayConfig)), inputManager(inputConfig), capabilities(PlatformCapabilities::detect())
        #if PIXELROOT32_ENABLE_TOUCH
        , touchManager(nullptr), wasTouchActive(false), lastTouchX(0), lastTouchY(0)
        #endif
    #if PIXELROOT32_ENABLE_AUDIO
        , audioEngine(audioConfig, capabilities), musicPlayer(audioEngine)
    #endif
    {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig) 
        : renderer(std::move(displayConfig)), inputManager(inputConfig), capabilities(PlatformCapabilities::detect())
        #if PIXELROOT32_ENABLE_TOUCH
        , touchManager(nullptr), wasTouchActive(false), lastTouchX(0), lastTouchY(0)
        #endif
    #if PIXELROOT32_ENABLE_AUDIO
        , audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine)
    #endif
    {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(DisplayConfig&& displayConfig) 
        : renderer(std::move(displayConfig)), inputManager(InputConfig(0)), capabilities(PlatformCapabilities::detect())
        #if PIXELROOT32_ENABLE_TOUCH
        , touchManager(nullptr), wasTouchActive(false), lastTouchX(0), lastTouchY(0)
        #endif
    #if PIXELROOT32_ENABLE_AUDIO
        , audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine)
    #endif
    {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(inputConfig), capabilities(PlatformCapabilities::detect())
        #if PIXELROOT32_ENABLE_TOUCH
        , touchManager(nullptr), wasTouchActive(false), lastTouchX(0), lastTouchY(0)
        #endif
    #if PIXELROOT32_ENABLE_AUDIO
          , audioEngine(audioConfig, capabilities), musicPlayer(audioEngine)
    #endif
    {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(inputConfig), capabilities(PlatformCapabilities::detect())
        #if PIXELROOT32_ENABLE_TOUCH
        , touchManager(nullptr), wasTouchActive(false), lastTouchX(0), lastTouchY(0)
        #endif
    #if PIXELROOT32_ENABLE_AUDIO
          , audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine)
    #endif
    {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::Engine(const DisplayConfig& displayConfig) 
        : renderer(const_cast<DisplayConfig&>(displayConfig)), 
          inputManager(InputConfig(0)), capabilities(PlatformCapabilities::detect())
#if PIXELROOT32_ENABLE_AUDIO
          , audioEngine(AudioConfig(), capabilities), musicPlayer(audioEngine)
#endif
    {
        previousMillis = 0;
        deltaTime = 0;
    }

    Engine::~Engine() {}

    void Engine::init() {
        assert(renderer.getLogicalWidth() > 0 && "Engine init failed: renderer has invalid width");
        assert(renderer.getLogicalHeight() > 0 && "Engine init failed: renderer has invalid height");
        
        // Initialize Serial for debugging (ESP32 only)
        #ifndef PLATFORM_NATIVE
            Serial.begin(115200);
            delay(100);
        #endif
        
        renderer.init();
        inputManager.init();

        // Initialize audio engine if enabled
        #if PIXELROOT32_ENABLE_AUDIO
            audioEngine.init();
        #endif
        
        // Set default font (5x7 bitmap font)
        FontManager::setDefaultFont(&FONT_5X7);
        
        #ifdef PLATFORM_NATIVE
        connectInputToDrawer();
        #endif
    }

    #ifdef PLATFORM_NATIVE
    void Engine::connectInputToDrawer() {
        // Get the DrawSurface from Renderer - for Native it's SDL2_Drawer
        auto& drawSurface = renderer.getDrawSurface();
        
        // Only SDL2_Drawer supports touch event injection - use dynamic_cast
        auto* sdlDrawer = dynamic_cast<drivers::native::SDL2_Drawer*>(&drawSurface);
        if (sdlDrawer) {
            #if PIXELROOT32_ENABLE_TOUCH
            sdlDrawer->setTouchDispatcher(&touchDispatcher);
            #else
            // Fallback to old method for backwards compatibility when touch disabled
            sdlDrawer->setInputManager(&inputManager);
            #endif
        }
    }
    #endif

    #if PIXELROOT32_ENABLE_TOUCH
    void Engine::setTouchManager(pixelroot32::input::TouchManager* tm) {
        touchManager = tm;
        // Reset state on new TouchManager
        wasTouchActive = false;
        lastTouchX = 0;
        lastTouchY = 0;
    }
    #endif

    void Engine::run() {
        DrawSurface* drawer = static_cast<DrawSurface*>(&renderer.getDrawSurface());
        
        #ifdef PLATFORM_NATIVE
            bool running = true;

            while (running) {
                // Process SDL events
                running = drawer->processEvents();

                update();

                bool redraw = sceneManager.aggregateShouldRedrawFramebuffer();
                if constexpr (pixelroot32::platforms::config::EnableDebugOverlay) {
                    redraw = true;
                }
                if (redraw) {
                    draw();
                    drawer->present();
                }

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
                    log(LogLevel::Profiling, "[Engine] Heartbeat...");
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

                        log(LogLevel::Profiling, "FPS: %d | Update: %dus | Events: %dus | Draw: %dus | Present: %dus | Collision: %luus | PhysicsInt: %luus (%lu)\n",
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
                    audio::ApuCore::ProfileEntry audioEntries[audio::ApuCore::PROFILE_RING_SIZE];
                    uint8_t audioCount = 0;
                    audioEngine.getScheduler()->getApuCore().getAndResetProfileStats(audioEntries, audioCount);
                    for (uint8_t i = 0; i < audioCount; ++i) {
                        if (audioEntries[i].clipped) {
                            log(LogLevel::Profiling, "[AUDIO] PEAK DETECTED: %.0f (CLIPPING!)", audioEntries[i].peak);
                        } else {
                            log(LogLevel::Profiling, "[AUDIO] Peak: %.0f (%.1f%%)",
                                audioEntries[i].peak, (audioEntries[i].peak / 32767.0f) * 100.0f);
                        }
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

            bool redraw = sceneManager.aggregateShouldRedrawFramebuffer();
            if constexpr (pixelroot32::platforms::config::EnableDebugOverlay) {
                redraw = true;
            }

            uint32_t t3 = t2;
            if (redraw) {
                draw();
                if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                    t3 = pixelroot32::platforms::config::profilerMicros();
                }

                // Present frame (TFT_eSPI)
                drawer->present();
            }

            if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                const uint32_t t4 = pixelroot32::platforms::config::profilerMicros();
                totalUpdateTime += (t1 - t0);
                totalEventsTime += (t2 - t1);
                totalDrawTime += redraw ? (t3 - t2) : 0u;
                totalPresentTime += redraw ? (t4 - t3) : 0u;
                frameCount++;
            }

            yield();

        #endif // PLATFORM_NATIVE
    }

    void Engine::setScene(Scene* newScene) {
        assert(newScene != nullptr && "Cannot set null scene in engine");
        sceneManager.setCurrentScene(newScene);
    }

    Renderer& Engine::getRenderer() {
        return renderer;
    }

#if PIXELROOT32_ENABLE_UI_SYSTEM
    graphics::ui::UIManager& Engine::getUIManager() {
        assert(sceneManager.getCurrentScene().has_value() && "No active scene - cannot get UIManager");
        return sceneManager.getCurrentScene().value()->getUIManager();
    }
#endif

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
        
        // Update scene
        sceneManager.update(deltaTime);
        
        #if PIXELROOT32_ENABLE_TOUCH
        // Process external TouchManager if set (ESP32 path)
        if (touchManager != nullptr) {
            pixelroot32::input::TouchPoint points[pixelroot32::input::TOUCH_MAX_POINTS];
            uint8_t count = touchManager->getTouchPoints(points);
            
            if (count > 0) {
                // Touch is active - inject all touch points
                for (uint8_t i = 0; i < count; i++) {
                    touchDispatcher.processTouch(points[i].id, true, 
                        points[i].x, points[i].y, points[i].ts);
                }
                wasTouchActive = true;
                lastTouchX = points[0].x;
                lastTouchY = points[0].y;
            } else if (wasTouchActive) {
                // Touch was released - inject release event
                touchDispatcher.processTouch(0, false, lastTouchX, lastTouchY, millis());
                wasTouchActive = false;
            }
        }
        
        // Process touch events and send to current scene
        if (touchDispatcher.hasEvents()) {
            pixelroot32::input::TouchEvent events[pixelroot32::input::TOUCH_EVENT_QUEUE_SIZE];
            uint8_t count = touchDispatcher.getEvents(events, pixelroot32::input::TOUCH_EVENT_QUEUE_SIZE);
            if (count > 0) {
                auto sceneOpt = sceneManager.getCurrentScene();
                if (sceneOpt.has_value() && sceneOpt.value() != nullptr) {
                    sceneOpt.value()->processTouchEvents(events, count);
                }
            }
        }
        #endif
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
                    // Integer-only FPS formatting (no float, no FPU dependency)
                    // fps_x10 = (frames * 10000) / ms gives 1 decimal precision
                    uint32_t fps_x10 = (debugUpdateCounter * 10000) / debugAccumulatedMs;
                    uint16_t fps_int = fps_x10 / 10;
                    uint8_t fps_frac = fps_x10 % 10;
                    
                    // Manual format: "FPS: XX.X" (9 chars max)
                    fpsStr[0] = 'F';
                    fpsStr[1] = 'P';
                    fpsStr[2] = 'S';
                    fpsStr[3] = ':';
                    fpsStr[4] = ' ';
                    fpsStr[5] = static_cast<char>('0' + fps_int / 10);
                    fpsStr[6] = static_cast<char>('0' + fps_int % 10);
                    fpsStr[7] = '.';
                    fpsStr[8] = static_cast<char>('0' + fps_frac);
                    fpsStr[9] = '\0';
                }

                #ifdef PLATFORM_NATIVE
                    ramStr[0] = 'R'; ramStr[1] = 'A'; ramStr[2] = 'M';
                    ramStr[3] = ':'; ramStr[4] = ' ';
                    ramStr[5] = 'N'; ramStr[6] = '/'; ramStr[7] = 'A';
                    ramStr[8] = '\0';
                #else
                    uint32_t freeHeap = ESP.getFreeHeap();
                    uint32_t totalHeap = ESP.getHeapSize();
                    uint32_t usedHeapK = (totalHeap - freeHeap) / 1024;
                    
                    // Manual integer format: "RAM: XXXK"
                    uint8_t idx = 0;
                    ramStr[idx++] = 'R';
                    ramStr[idx++] = 'A';
                    ramStr[idx++] = 'M';
                    ramStr[idx++] = ':';
                    ramStr[idx++] = ' ';
                    
                    // Extract digits for usedHeapK (max 3 digits for ESP32)
                    if (usedHeapK >= 100) {
                        ramStr[idx++] = static_cast<char>('0' + (usedHeapK / 100) % 10);
                        ramStr[idx++] = static_cast<char>('0' + (usedHeapK / 10) % 10);
                        ramStr[idx++] = static_cast<char>('0' + usedHeapK % 10);
                    } else if (usedHeapK >= 10) {
                        ramStr[idx++] = static_cast<char>('0' + (usedHeapK / 10) % 10);
                        ramStr[idx++] = static_cast<char>('0' + usedHeapK % 10);
                    } else {
                        ramStr[idx++] = static_cast<char>('0' + usedHeapK);
                    }
                    ramStr[idx++] = 'K';
                    ramStr[idx++] = '\0';
                #endif

                // CPU load as integer percentage
                float load = (float)debugAccumulatedMs / (DEBUG_UPDATE_INTERVAL * 16.6f);
                int cpuPercent = static_cast<int>(load * 100);
                if (cpuPercent > 100) cpuPercent = 100;
                
                // Manual format: "CPU: XX%"
                cpuStr[0] = 'C';
                cpuStr[1] = 'P';
                cpuStr[2] = 'U';
                cpuStr[3] = ':';
                cpuStr[4] = ' ';
                if (cpuPercent >= 100) {
                    cpuStr[5] = '1';
                    cpuStr[6] = '0';
                    cpuStr[7] = '0';
                } else if (cpuPercent >= 10) {
                    cpuStr[5] = static_cast<char>('0' + cpuPercent / 10);
                    cpuStr[6] = static_cast<char>('0' + cpuPercent % 10);
                } else {
                    cpuStr[5] = static_cast<char>('0' + cpuPercent);
                }
                cpuStr[7] = '%';
                cpuStr[8] = '\0';

                debugUpdateCounter = 0;
                debugAccumulatedMs = 0;
            }
            
            int oldX = r.getXOffset();
            int oldY = r.getYOffset();
            r.setDisplayOffset(0, 0);

            int16_t x = r.getLogicalWidth() - 55;
            if (x < 0) x = 0;
            
            r.drawText(fpsStr, x, 4, Color::Green, 1);
            r.drawText(ramStr, x, 12, Color::Cyan, 1);
            r.drawText(cpuStr, x, 20, Color::Yellow, 1);

            r.setDisplayOffset(oldX, oldY);
        }
    }
}
