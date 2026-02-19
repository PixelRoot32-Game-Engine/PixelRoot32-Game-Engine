/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#ifdef PLATFORM_NATIVE

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

#endif // PLATFORM_NATIVE

#endif // MOCK_SPI_H
