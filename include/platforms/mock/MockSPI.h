/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#include "platforms/PlatformSelect.h"

// Nothing here touches SDL: this is a stand-in for the Arduino SPI *library*,
// needed exactly where that library is absent. It is gated on the absence of the
// Arduino core because it defines a global `SPI`, which would clash with the
// real one. A non-Arduino build without SDL still gets it.
#if !PIXELROOT32_HAS_ARDUINO

#include <cstdint>

/**
 * @class SPIClass
 * @brief Mocks the Arduino SPI class for native platform.
 *
 * Provides a dummy implementation of SPI methods to allow compilation
 * of drivers that depend on SPI.h.
 */
class SPIClass {
public:
    void begin() {}
    void end() {}

    /**
     * @brief Mocks data transfer. Returns input data (loopback).
     */
    uint8_t transfer(uint8_t data) {
        return data;
    }
};

// Arduino-style global instance
extern SPIClass SPI;

#endif // !PIXELROOT32_HAS_ARDUINO

#endif // MOCK_SPI_H
