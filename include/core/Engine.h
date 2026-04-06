/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/SceneManager.h"
#include <optional>
#include "graphics/Renderer.h"
#include "input/InputConfig.h"
#include "input/InputManager.h"
#include "graphics/DisplayConfig.h"
#include "audio/AudioConfig.h"
#include "audio/AudioEngine.h"
#include "audio/MusicPlayer.h"
#include "platforms/PlatformCapabilities.h"

#if PIXELROOT32_ENABLE_TOUCH
#include "input/TouchEventDispatcher.h"
#include "input/TouchEvent.h"
#include "input/TouchPoint.h"

// Forward declaration
namespace pixelroot32 { namespace input { class TouchManager; } }
#endif

namespace pixelroot32::core {

/**
 * @class Engine
 * @brief The main engine class that manages the game loop and core subsystems.
 *
 * Engine acts as the central hub of the game engine. It initializes and manages
 * the Renderer, InputManager, AudioEngine, SceneManager, and optionally Touch system.
 * It runs the main game loop, handling timing (delta time), updating the current 
 * scene, and rendering frames.
 *
 * ## Touch Pipeline (PIXELROOT32_ENABLE_TOUCH)
 *
 * When touch is enabled (default), Engine automatically:
 *   1. Processes mouse events (Native) or receives injected touch points (ESP32)
 *   2. Applies gesture detection (Click, DoubleClick, LongPress, Drag)
 *   3. Sends events to the current scene via Scene::processTouchEvents()
 *
 * The order inside Scene::processTouchEvents is guaranteed:
 *   UIManager::processEvents (marks consumed) → onUnconsumedTouchEvent (virtual).
 *
 * Set PIXELROOT32_ENABLE_TOUCH=0 in platform defines to disable (saves ~200 bytes).
 */
class Engine {
public:
    /**
     * @brief Constructs the Engine with custom display, input and audio configurations.
     * @param displayConfig Configuration settings for the display (width, height, rotation, etc.).
     * @param inputConfig Configuration settings for the input system (pins, buttons).
     * @param audioConfig Configuration settings for the audio system.
     */
    Engine(pixelroot32::graphics::DisplayConfig&& displayConfig, const pixelroot32::input::InputConfig& inputConfig, const pixelroot32::audio::AudioConfig& audioConfig);
    Engine(pixelroot32::graphics::DisplayConfig&& displayConfig, const pixelroot32::input::InputConfig& inputConfig);
    Engine(pixelroot32::graphics::DisplayConfig&& displayConfig);

    Engine(const pixelroot32::graphics::DisplayConfig& displayConfig, const pixelroot32::input::InputConfig& inputConfig, const pixelroot32::audio::AudioConfig& audioConfig);

    /**
     * @brief Constructs the Engine with custom display and input configurations.
     * @param displayConfig Configuration settings for the display (width, height, rotation, etc.).
     * @param inputConfig Configuration settings for the input system (pins, buttons).
     */
    Engine(const pixelroot32::graphics::DisplayConfig& displayConfig, const pixelroot32::input::InputConfig& inputConfig);

    /**
     * @brief Constructs the Engine with custom display configuration and default input settings.
     * @param displayConfig Configuration settings for the display.
     */
    Engine(const pixelroot32::graphics::DisplayConfig& displayConfig);

    /**
     * @brief Destructor. Cleans up engine resources.
     */
    ~Engine();

    /**
     * @brief Initializes the engine subsystems.
     * 
     * This method must be called before run(). It initializes the Renderer,
     * InputManager, and sets up the initial state.
     */
    void init();

    /**
     * @brief Starts the main game loop.
     * 
     * This method contains the infinite loop that calls update() and draw() repeatedly.
     * It handles frame timing and delta time calculation.
     */
    void run();

    /**
     * @brief Gets the time elapsed since the last frame.
     * @return The delta time in milliseconds.
     */
    unsigned long getDeltaTime() const { return deltaTime; }

    /**
     * @brief Gets the number of milliseconds since the engine started.
     * @return The time in milliseconds.
     */
    unsigned long getMillis() const;
    
    /**
     * @brief Sets the current active scene.
     * @param newScene Pointer to the new Scene to become active.
     */
    void setScene(Scene* newScene);

    /**
     * @brief Retrieves the currently active scene.
     * @return Optional pointer to the current Scene, or nullopt if none is set.
     */
    std::optional<Scene*> getCurrentScene() const { return sceneManager.getCurrentScene(); }
    
    /**
     * @brief Replaces the current renderer instance.
     * @param newRenderer R-value reference to the new Renderer to use.
     */
    void setRenderer(pixelroot32::graphics::Renderer&& newRenderer) { renderer = std::move(newRenderer); }

    /**
     * @brief Provides access to the Renderer subsystem.
     * @return Reference to the current Renderer.
     */
    pixelroot32::graphics::Renderer& getRenderer();

    /**
     * @brief Provides access to the InputManager subsystem.
     * @return Reference to the InputManager    .
     */
    pixelroot32::input::InputManager& getInputManager() { return inputManager; }
    
    #if PIXELROOT32_ENABLE_TOUCH
    /**
     * @brief Provides access to the touch event system.
     * @return Reference to the TouchEventDispatcher.
     * 
     * Use this to inject touch points on ESP32 (via TouchManager):
     *   engine.getTouchDispatcher().processTouch(id, pressed, x, y, timestamp);
     */
    pixelroot32::input::TouchEventDispatcher& getTouchDispatcher() { return touchDispatcher; }
    
    /**
     * @brief Check if there are pending touch events.
     * @return true if there are events in the queue.
     */
    bool hasTouchEvents() const { return touchDispatcher.hasEvents(); }
    
    /**
     * @brief Set the TouchManager for automatic touch processing.
     * @param touchManager Pointer to the TouchManager instance.
     * 
     * On ESP32, call this once in setup() after touchManager.init():
     *   touchManager.init();
     *   engine.setTouchManager(&touchManager);
     * 
     * Then in loop(), just call engine.run() - Engine handles:
     *   - Polling touchManager.getTouchPoints() each frame
     *   - Detecting touch release (when count goes from >0 to 0)
     *   - Sending gesture events to the current scene
     * 
     * This eliminates the need to manually inject touch points or track release state.
     */
    void setTouchManager(pixelroot32::input::TouchManager* touchManager);
    #endif
    
    #ifdef PLATFORM_NATIVE
    /**
     * @brief Connect InputManager to Drawer for mouse-to-touch mapping.
     * @param inputManager Pointer to the InputManager.
     * 
     * Called automatically in init() for Native builds.
     */
    void connectInputToDrawer();
    #endif

#if PIXELROOT32_ENABLE_UI_SYSTEM
    /**
     * @brief Provides access to the UI system via the current scene.
     * 
     * This is a hybrid approach - UI is scene-managed but accessible through Engine.
     * The UIManager is owned by each Scene, so this delegates to the current scene's
     * UIManager. This allows code that has an Engine reference to also access UI
     * functionality without needing direct scene access.
     * 
     * @return Reference to the current scene's UIManager.
     * @note Asserts if no scene is currently active.
     */
    graphics::ui::UIManager& getUIManager();
#endif

#if PIXELROOT32_ENABLE_AUDIO
    /**
     * @brief Provides access to the AudioEngine subsystem.
     * @return Reference to the AudioEngine.
     */
    pixelroot32::audio::AudioEngine& getAudioEngine() { return audioEngine; }

    /**
     * @brief Provides access to the MusicPlayer subsystem.
     * @return Reference to the MusicPlayer.
     */
    pixelroot32::audio::MusicPlayer& getMusicPlayer() { return musicPlayer; }
#endif

    using PlatformCapabilities = pixelroot32::platforms::PlatformCapabilities;

    /**
     * @brief Gets the capabilities of the current hardware platform.
     * @return Reference to the PlatformCapabilities.
     */
    const PlatformCapabilities& getPlatformCapabilities() const { return capabilities; }

protected:
    SceneManager sceneManager; ///< Manages scene transitions and the scene stack.
    pixelroot32::graphics::Renderer renderer;         ///< Handles all graphics rendering operations.
    pixelroot32::input::InputManager inputManager; ///< Manages user input.
    PlatformCapabilities capabilities;             ///< Hardware capabilities of the current platform.
    
    // Touch subsystem
    #if PIXELROOT32_ENABLE_TOUCH
        pixelroot32::input::TouchEventDispatcher touchDispatcher;  ///< Touch event state machine and queue.
        pixelroot32::input::TouchManager* touchManager;  ///< External TouchManager for ESP32 (optional).
        bool wasTouchActive;  ///< Track previous frame's touch state for release detection.
        int16_t lastTouchX;  ///< Last touch X for release event.
        int16_t lastTouchY;  ///< Last touch Y for release event.
    #endif
    
    // Audio subsystems
    #if PIXELROOT32_ENABLE_AUDIO
        pixelroot32::audio::AudioEngine audioEngine;   ///< Manages audio playback.
        pixelroot32::audio::MusicPlayer musicPlayer;   ///< Manages music sequencing.
    #endif

    unsigned long previousMillis; ///< Timestamp of the previous frame.
    unsigned long deltaTime;      ///< Calculated time difference between frames.

    /**
     * @brief Updates the game logic.
     * 
     * Called once per frame. Updates the input manager and the current scene.
     */
    void update();

    /**
     * @brief Renders the current frame.
     * 
     * Called once per frame. Clears the buffer, asks the scene to draw itself, and sends the buffer to the display.
     */
    void draw();

    /**
     * @brief Draws a debug overlay with real-time engine metrics.
     * Shows FPS, CPU usage (estimated), and RAM usage.
     */
    void drawDebugOverlay(pixelroot32::graphics::Renderer& r);

    static constexpr int DEBUG_UPDATE_INTERVAL = 16;  ///< Update metrics every N frames.
    int debugUpdateCounter = 0;                         ///< Frame counter for updates.
    unsigned long debugAccumulatedMs = 0;               ///< Accumulated time for FPS calculation.
    
    // Cached strings for rendering to minimize per-frame overhead
    char fpsStr[12] = "FPS: 0";
    char ramStr[16] = "RAM: 0K";
    char cpuStr[12] = "CPU: 0%";
};

}
