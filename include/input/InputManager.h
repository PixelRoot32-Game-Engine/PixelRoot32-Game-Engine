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
    #include "../../src/platforms/mock/MockArduino.h"  // Defines SDL_MAIN_HANDLED before SDL.h
#else
    #include <Arduino.h>
#endif
#include "InputConfig.h"

namespace pixelroot32::input {
/**
 * @class InputManager
 * @brief Handles input from physical buttons or keyboard (on PC).
 *
 * The InputManager polls configured pins, handles debouncing, and tracks button states
 * (Pressed, Released, Down, Clicked).
 */
class InputManager {
public:
    /**
     * @brief Constructs the InputManager with a specific configuration.
     * @param config The input configuration (pins, button count).
     */
    InputManager(const InputConfig& config);

    /**
     * @brief Initializes the input pins.
     */
    void init();

#ifdef PLATFORM_NATIVE
    /**
     * @brief Updates input state based on SDL keyboard state.
     * @param dt Delta time.
     * @param keyboardState Pointer to the SDL keyboard state array.
     */
    void update(unsigned long dt, const uint8_t* keyboardState);
#else
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

private:
    InputConfig config;

    bool* buttonState;      ///< Current state of buttons (true = pressed).
    bool* stateChanged;     ///< Flags indicating if state changed this frame.
    uint16_t* waitTime;     ///< Debounce timers for each button.
    bool* clickFlag;        ///< Flags for tracking click events.
    uint8_t* buttonPins;    ///< Array of hardware pin numbers.
};

}