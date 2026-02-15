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
#include "input/InputConfig.h"
#include "input/InputManager.h"

namespace pixelroot32::input {

    InputManager::InputManager(const InputConfig& config) : config(config) {}

    void InputManager::init(){
        if (config.count <= 0) return;

        // Initialize button pins
        buttonPins.resize(config.count);
        buttonState.assign(config.count, false);
        stateChanged.assign(config.count, false);
        waitTime.assign(config.count, 0);
        clickFlag.assign(config.count, false);
        
        // Set all pins as INPUT_PULLUP
        for (int i = 0; i < config.count; i++) {
            #ifdef PLATFORM_NATIVE
                buttonPins[i] = config.buttonNames[i];
            #else
                buttonPins[i] = config.inputPins[i];
            #endif
            pinMode(buttonPins[i], INPUT_PULLUP);
        }
    }

    void InputManager::update(unsigned long dt, const uint8_t* keyboardState) {
        if (config.count <= 0) return;

        for (int i = 0; i < config.count; i++) {
            // Reset stateChanged at the start of each update for every button
            stateChanged[i] = false;

            // Check if in debounce time window for each button
            if (waitTime[i] >= dt) {
                waitTime[i] -= dt;
                continue;
            }
        

            waitTime[i] = 0;
            bool reading = (keyboardState[buttonPins[i]]);
            
            if (reading != buttonState[i]) {
                // Button pressed or released.
                waitTime[i] = 100;  // Debounce delay
                buttonState[i] = reading;
                stateChanged[i] = true;
            }
        }   
    }

    #ifndef PLATFORM_NATIVE
    void InputManager::update(unsigned long dt) {
        if (config.count <= 0) return;

        for (int i = 0; i < config.count; i++) {
            // Reset stateChanged at the start of each update for every button
            stateChanged[i] = false;

            // Check if in debounce time window for each button
            if (waitTime[i] >= dt) {
                waitTime[i] -= dt;
                continue;
            }

            waitTime[i] = 0;
            bool reading = digitalRead(buttonPins[i]) == LOW;
            
            if (reading != buttonState[i]) {
                // Button pressed or released.
                waitTime[i] = 100;  // Debounce delay
                buttonState[i] = reading;
                stateChanged[i] = true;
            }
        }
    }
    #endif

    //has the button been pressed
    bool InputManager::isButtonPressed(uint8_t buttonIndex) const {
        if (buttonIndex >= config.count) return false;

        return buttonState[buttonIndex] && stateChanged[buttonIndex];
    }

    //has the button been released
    bool InputManager::isButtonReleased(uint8_t buttonIndex) const {
        if (buttonIndex >= config.count) return false;

        return !buttonState[buttonIndex] && stateChanged[buttonIndex];
    }

    //is the button currently being held
    bool InputManager::isButtonDown(uint8_t buttonIndex) const {
        if (buttonIndex >= config.count) return false;

        return buttonState[buttonIndex];
    }

    //has the button been pressed and released
    bool InputManager::isButtonClicked(uint8_t buttonIndex) const {
        if (buttonIndex >= config.count) return false;

        // Function to look for press and release
        if (clickFlag[buttonIndex] && isButtonReleased(buttonIndex)) {
            clickFlag[buttonIndex] = false;
            return true;
        }

        if (isButtonPressed(buttonIndex)) {
            clickFlag[buttonIndex] = true;
        }
        return false;    
    }
}
