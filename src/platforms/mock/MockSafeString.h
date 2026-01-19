/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#ifndef MOCK_SAFE_STRING_H
#define MOCK_SAFE_STRING_H

#ifdef PLATFORM_NATIVE

#include <string>
#include <cstdarg>
#include <cstdio>

/**
 * @class MockSafeString
 * @brief Mocks the SafeString library for native PC testing.
 *
 * SafeString is a library often used in Arduino for safer string manipulation.
 * This mock wraps std::string to provide a compatible API.
 */
class MockSafeString {
public:
    MockSafeString() = default;

    void clear() {
        data.clear();
    }

    const char* c_str() const {
        return data.c_str();
    }

    size_t length() const {
        return data.length();
    }

    // Assuming SafeString is the intended class name, usually there would be a typedef or class rename.
    // For now, documenting as is to match the existing code structure.
    MockSafeString& operator=(const char* str) {
        data = (str != nullptr) ? str : "";
        return *this;
    }

    MockSafeString& operator=(const std::string& str) {
        data = str;
        return *this;
    }

private:
    std::string data;
};

// Typedef to allow usage of 'SafeString' type as expected by the macro and library users
typedef MockSafeString SafeString;

// Arduino-style macro
#define createSafeString(name, size) SafeString name; (void)size

#endif // PLATFORM_NATIVE

#endif // MOCK_SAFE_STRING_H
