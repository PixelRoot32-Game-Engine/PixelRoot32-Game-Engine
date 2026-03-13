/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdarg>
#include <cstdio>

/**
 * Logging is controlled by PIXELROOT32_DEBUG_MODE in EngineConfig.h (or build flags).
 * If PIXELROOT32_DEBUG_MODE is not defined, log() calls are no-ops and nothing is printed.
 */

/**
 * @namespace pixelroot32::core::logging
 * @brief Core logging utilities for the PixelRoot32 engine.
 *
 * This namespace provides logging functions and utilities for the game engine core,
 * including log level management and formatted message output.
 */
namespace pixelroot32::core::logging
{
    /**
     * @enum LogLevel
     * @brief Enumeration of log levels.
     */
    enum class LogLevel
    {
        Info,
        Profiling,
        Warning,
        Error
    };

    /**
     * @brief Converts a log level to its string representation.
     * @param level The log level to convert.
     * @return A string representation of the log level.
     */
    const char* levelToString(LogLevel level);

    /**
     * @brief Prints text to the platform-specific output.
     * @param text The text to print.
     */
    void platformPrint(const char* text);

    /**
     * @brief Internal logging function that handles formatted message output.
     * @param level The log level for the message.
     * @param fmt The format string (printf-style).
     * @param args The va_list of arguments for the format string.
     */
    void logInternal(LogLevel level, const char* fmt, va_list args);

#ifdef PIXELROOT32_DEBUG_MODE
    /**
     * @brief Logs a message with the specified level.
     * @param level The log level.
     * @param fmt The format string (printf-style).
     * @param ... Variable arguments for the format string.
     */
    inline void log(LogLevel level, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logInternal(level, fmt, args);
        va_end(args);
    }

    /**
     * @brief Logs a message with the default Info level.
     * @param fmt The format string (printf-style).
     * @param ... Variable arguments for the format string.
     */
    inline void log(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logInternal(LogLevel::Info, fmt, args); // default Info
        va_end(args);
    }
#else
    /** No-op when PIXELROOT32_DEBUG_MODE is not defined. */
    inline void log(LogLevel level, const char* fmt, ...) { (void)level; (void)fmt; }

    /** No-op when PIXELROOT32_DEBUG_MODE is not defined. */
    inline void log(const char* fmt, ...) { (void)fmt; }
#endif
} // namespace pixelroot32::core::logging