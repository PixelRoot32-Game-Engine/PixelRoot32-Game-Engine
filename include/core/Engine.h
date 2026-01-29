/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "core/SceneManager.h"
#include "graphics/Renderer.h"
#include "input/InputConfig.h"
#include "input/InputManager.h"
#include "graphics/DisplayConfig.h"
#include "audio/AudioConfig.h"
#include "audio/AudioEngine.h"
#include "audio/MusicPlayer.h"

namespace pixelroot32::core {

/**
 * @class Engine
 * @brief The main engine class that manages the game loop and core subsystems.
 *
 * Engine acts as the central hub of the game engine. It initializes and manages
 * the Renderer, InputManager, AudioEngine, and SceneManager. It runs the main game loop,
 * handling timing (delta time), updating the current scene, and rendering frames.
 */
class Engine {
public:
    /**
     * @brief Constructs the Engine with custom display, input and audio configurations.
     * @param displayConfig Configuration settings for the display (width, height, rotation, etc.).
     * @param inputConfig Configuration settings for the input system (pins, buttons).
     * @param audioConfig Configuration settings for the audio system.
     */
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
     * @brief Sets the current active scene.
     * @param newScene Pointer to the new Scene to become active.
     */
    void setScene(Scene* newScene);

    /**
     * @brief Retrieves the currently active scene.
     * @return Pointer to the current Scene, or nullptr if none is set.
     */
    Scene* getCurrentScene() const { return sceneManager.getCurrentScene(); }
    
    /**
     * @brief Replaces the current renderer instance.
     * @param newRenderer Reference to the new Renderer to use.
     */
    void setRenderer(pixelroot32::graphics::Renderer& newRenderer) { renderer = newRenderer; }

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

private:
    SceneManager sceneManager; ///< Manages scene transitions and the scene stack.
    pixelroot32::graphics::Renderer renderer;         ///< Handles all graphics rendering operations.
    pixelroot32::input::InputManager inputManager; ///< Manages input device state and events.
    pixelroot32::audio::AudioEngine audioEngine;   ///< Manages audio playback.
    pixelroot32::audio::MusicPlayer musicPlayer;   ///< Manages music sequencing.

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

#ifdef PIXELROOT32_ENABLE_FPS_DISPLAY
    /**
     * @brief Draws the FPS overlay (green text, top-right) when PIXELROOT32_ENABLE_FPS_DISPLAY is defined.
     * FPS value is recalculated every FPS_UPDATE_INTERVAL frames to minimize per-frame cost.
     */
    void drawFpsOverlay(pixelroot32::graphics::Renderer& r);

    static constexpr int FPS_UPDATE_INTERVAL = 8;    ///< Recalculate FPS every N frames.
    char fpsOverlayBuf[12];                         ///< Cached "FPS xxx" string.
    int fpsUpdateCounter;                          ///< Counts frames until next FPS update.
#endif
};

}