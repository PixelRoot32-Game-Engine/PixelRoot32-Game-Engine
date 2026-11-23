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
#include "platforms/PlatformSelect.h"

// Whichever build this is, the header needs an Arduino-shaped API (pinMode,
// digitalRead, INPUT_PULLUP) in some form: the real core, or the SDL-backed
// mock. Asking PLATFORM_NATIVE instead left the web build requesting
// <Arduino.h>, which no browser toolchain ships. A build with neither has
// nothing to compile against, so it now says so instead of picking one by
// elimination.
#if PIXELROOT32_HAS_ARDUINO
    #include <Arduino.h>
#elif PIXELROOT32_HAS_SDL
    #include <platforms/mock/MockArduino.h>  // Defines SDL_MAIN_HANDLED before SDL.h
#else
    #error "PixelRoot32 input: this build has neither the Arduino core nor SDL2, so there is no pinMode()/digitalRead() to compile against."
#endif
#include "InputConfig.h"
#include "TouchEventDispatcher.h"

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
    static constexpr uint8_t MAX_BUTTONS = 16;  ///< Maximum number of buttons supported.
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

#if PIXELROOT32_HAS_ARDUINO
    /**
     * @brief Updates input state by polling hardware pins.
     * @param dt Delta time.
     *
     * Only a board build has pins to poll: the SDL mock's digitalRead() takes a
     * pointer and answers from the keyboard state, so desktop and web use the
     * keyboardState overload below instead.
     */
    void update(unsigned long dt);
#endif  // PIXELROOT32_HAS_ARDUINO

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
    
    #if PIXELROOT32_HAS_SDL
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
    #endif  // PIXELROOT32_HAS_SDL
    
    #if PIXELROOT32_HAS_ARDUINO
    /**
     * @brief Inject a raw touch point from external source (e.g., TouchManager).
     * @param point The touch point to inject.
     * @param timestamp Current timestamp in ms.
     * 
     * Used by ESP32 examples to connect TouchManager with InputManager.
     */
    void injectTouchPoint(int16_t x, int16_t y, bool pressed, uint8_t id, uint32_t timestamp);
    #endif  // PIXELROOT32_HAS_ARDUINO

private:
    InputConfig config;

    // Fixed-size arrays instead of std::vector (zero heap allocation)
    // MAX_BUTTONS is defined in public section for external access
    bool buttonState[MAX_BUTTONS] = {false};      ///< Current state of buttons (true = pressed).
    bool stateChanged[MAX_BUTTONS] = {false};     ///< Flags indicating if state changed this frame.
    uint16_t waitTime[MAX_BUTTONS] = {0};         ///< Debounce timers for each button.
    mutable bool clickFlag[MAX_BUTTONS] = {false}; ///< Flags for tracking click events.
    uint8_t buttonPins[MAX_BUTTONS] = {0};        ///< Array of hardware pin numbers.
    
    // Touch event system
    TouchEventDispatcher touchDispatcher;  ///< Touch event state machine and queue.
    
    #if PIXELROOT32_HAS_SDL
    bool mouseButtonDown;  ///< Track mouse button state for the mouse-to-touch path.
    #endif  // PIXELROOT32_HAS_SDL
};

}
