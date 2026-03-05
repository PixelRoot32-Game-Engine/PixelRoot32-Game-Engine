/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include "core/Log.h"

#ifdef ESP32
    #include <Arduino.h>
#else
    #include <cstdio>
#endif

/**
 * @namespace pixelroot32::core::logging
 * @brief Platform-specific logging utilities.
 * This namespace provides logging functions that work across different platforms,
 * including ESP32 and native environments.
 */
namespace pixelroot32::core::logging
{
    /**
     * @brief Converts a log level to its string representation.
     * @param level The log level to convert.
     * @return A string representation of the log level.
     */
    const char* levelToString(LogLevel level)
    {
        switch(level)
        {
            case LogLevel::Info:    return "[INFO] ";
            case LogLevel::Profiling: return "[PROFILING] ";
            case LogLevel::Warning: return "[WARN] ";
            case LogLevel::Error:   return "[ERROR] ";
        }
        return "";
    }

    /**
     * @brief Prints text to the platform-specific output.
     * @param text The text to print.
     */
    void platformPrint(const char* text)
    {
    #ifdef ESP32
        Serial.print(text);
    #else
        printf("%s", text);
        fflush(stdout); // ensure immediate output on native builds
    #endif
    }

    /**
     * @brief Internal logging function that handles formatted message output.
     * @param level The log level for the message.
     * @param fmt The format string (printf-style).
     * @param args The va_list of arguments for the format string.
     */
    void logInternal(LogLevel level, const char* fmt, va_list args)
    {
        char buffer[256];
        std::vsnprintf(buffer, sizeof(buffer), fmt, args);

        platformPrint(levelToString(level));
        platformPrint(buffer);
        platformPrint("\n");
    }
} // namespace pixelroot32::core::logging