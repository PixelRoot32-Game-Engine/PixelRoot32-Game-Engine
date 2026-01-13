#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#ifdef PLATFORM_NATIVE

#include <cstdint>

class SPIClass {
public:
    void begin() {}
    void end() {}

    uint8_t transfer(uint8_t data) {
        return data;
    }
};

// Arduino-style global instance
extern SPIClass SPI;

#endif // PLATFORM_NATIVE

#endif // MOCK_SPI_H
