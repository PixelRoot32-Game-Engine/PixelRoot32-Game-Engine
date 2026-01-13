#pragma once
#include <cstdarg>
#include <cstdint>

struct InputConfig{
#ifdef PLATFORM_NATIVE
    uint8_t* buttonNames = nullptr; // Array of button names (optional)
#else
    int* inputPins = nullptr ; // Array of input pin numbers
#endif
    int count = 0;         // Number of input pins or buttons

    InputConfig(int count, ...): count(count) {
        if (count <= 0) {
            this->count = 0;
            #ifdef PLATFORM_NATIVE
                buttonNames = nullptr;
            #else
                inputPins = nullptr;
            #endif
            return;
        }

        va_list args;
        va_start(args, count);

        #ifdef PLATFORM_NATIVE
            buttonNames = new uint8_t[count];
            for (int i = 0; i < count; i++) {
                buttonNames[i] = (uint8_t)va_arg(args, int);
            }
        #else
            inputPins = new int[count];
            for (int i = 0; i < count; i++) {
                inputPins[i] = va_arg(args, int);
            }
        #endif
        va_end(args);
    }
};