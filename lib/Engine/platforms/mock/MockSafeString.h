#ifndef MOCK_SAFE_STRING_H
#define MOCK_SAFE_STRING_H

#ifdef PLATFORM_NATIVE

#include <string>
#include <cstdarg>
#include <cstdio>

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

    SafeString& operator=(const char* str) {
        data = (str != nullptr) ? str : "";
        return *this;
    }

    SafeString& operator=(const std::string& str) {
        data = str;
        return *this;
    }

private:
    std::string data;
};

// Arduino-style macro
#define createSafeString(name, size) SafeString name; (void)size

#endif // PLATFORM_NATIVE

#endif // MOCK_SAFE_STRING_H
