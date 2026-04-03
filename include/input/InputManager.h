/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#pragma once
#ifdef PLATFORM_NATIVE
    #include <platforms/mock/MockArduino.h>  // Defines SDL_MAIN_HANDLED before SDL.h
#else
    #include <Arduino.h>
#endif
#include "InputConfig.h"
#include "TouchEventDispatcher.h"
#include <vector>

namespace pixelroot32::input {
/**
 * @class InputManager
 * @brief Handles input from physical buttons, keyboard (on PC), and touch/mouse.
 *
 * The InputManager polls configured pins, handles debouncing, and tracks button states
 * (Pressed, Released, Down, Clicked). It also provides touch event processing for
 * both ESP32 (via TouchManager) and Native (via mouse-to-touch mapping).
 */
class InputManager {
public:
    /**
     * @brief Constructs the InputManager with a specific configuration.
     * @param config The input configuration (pins, button count).
     */
    InputManager(const InputConfig& config);

    /**
     * @brief Destructor for InputManager.
     */
    ~InputManager() = default;

    /**
     * @brief Initializes the input pins.
     */
    void init();

    /**
     * @brief Updates input state based on SDL keyboard state.
     * @param dt Delta time.
     * @param keyboardState Pointer to the SDL keyboard state array.
     */
    void update(unsigned long dt, const uint8_t* keyboardState);

#ifndef PLATFORM_NATIVE
    /**
     * @brief Updates input state by polling hardware pins.
     * @param dt Delta time.
     */
    void update(unsigned long dt);
#endif

    /**
     * @brief Checks if a button was just pressed this frame.
     * @param buttonIndex Index of the button to check.
     * @return true if the button transitioned from UP to DOWN this frame.
     */
    bool isButtonPressed(uint8_t buttonIndex) const;

    /**
     * @brief Checks if a button was just released this frame.
     * @param buttonIndex Index of the button to check.
     * @return true if the button transitioned from DOWN to UP this frame.
     */
    bool isButtonReleased(uint8_t buttonIndex) const;

    /**
     * @brief Checks if a button was clicked (pressed and released).
     * @param buttonIndex Index of the button to check.
     * @return true if the button was clicked.
     */
    bool isButtonClicked(uint8_t buttonIndex) const;

    /**
     * @brief Checks if a button is currently held down.
     * @param buttonIndex Index of the button to check.
     * @return true if the button is currently in the DOWN state.
     */
    bool isButtonDown(uint8_t buttonIndex) const;

    // =========================================================================
    // Touch Input API
    // =========================================================================
    
    /**
     * @brief Get touch events from the event dispatcher.
     * @param buffer Caller-provided buffer for events.
     * @param maxCount Maximum number of events to retrieve.
     * @return Number of events retrieved (removed from queue).
     */
    uint8_t getTouchEvents(TouchEvent* buffer, uint8_t maxCount);
    
    /**
     * @brief Check if there are pending touch events.
     * @return true if there are events in the queue.
     */
    bool hasTouchEvents() const;
    
    /**
     * @brief Get the current state of a specific touch ID.
     * @param touchId Touch identifier (0-4).
     * @return Current touch state.
     */
    TouchState getTouchState(uint8_t touchId) const;
    
    #ifdef PLATFORM_NATIVE
    /**
     * @brief Process an SDL event (mouse/keyboard).
     * @param sdlEvent The SDL event to process.
     * 
     * This method handles:
     * - SDL_MOUSEBUTTONDOWN/UP: Maps to touch events
     * - SDL_MOUSEMOTION: Maps to drag events when button is held
     * - SDL_KEYDOWN/SDL_KEYUP: Maps to button events (existing)
     */
    void processSDLEvent(const void* sdlEvent);
    #endif
    
    #ifndef PLATFORM_NATIVE
    /**
     * @brief Inject a raw touch point from external source (e.g., TouchManager).
     * @param point The touch point to inject.
     * @param timestamp Current timestamp in ms.
     * 
     * Used by ESP32 examples to connect TouchManager with InputManager.
     */
    void injectTouchPoint(int16_t x, int16_t y, bool pressed, uint8_t id, uint32_t timestamp);
    #endif

private:
    InputConfig config;

    std::vector<bool> buttonState;      ///< Current state of buttons (true = pressed).
    std::vector<bool> stateChanged;     ///< Flags indicating if state changed this frame.
    std::vector<uint16_t> waitTime;     ///< Debounce timers for each button.
    mutable std::vector<bool> clickFlag;///< Flags for tracking click events.
    std::vector<uint8_t> buttonPins;    ///< Array of hardware pin numbers.
    
    // Touch event system
    TouchEventDispatcher touchDispatcher;  ///< Touch event state machine and queue.
    
    #ifdef PLATFORM_NATIVE
    bool mouseButtonDown;  ///< Track mouse button state for Native.
    #endif
};

}
