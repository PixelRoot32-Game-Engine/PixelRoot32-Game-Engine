/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_SD_STORAGE && defined(ESP32)

#include <SPI.h>

#include <cstdint>

namespace pixelroot32::persistence {

/**
 * @struct SdConfig
 * @brief Hardware configuration for an SD card over SPI.
 *
 * Plain data, passed to an SD adapter's init(): the adapter owns no pin
 * constants and no board assumptions, so the same adapter serves any board
 * that names its own bus and pins. Callers name the SPI peripheral and pins;
 * -1 means "use the bus default" for that signal.
 */
struct SdConfig {
    /** @brief SPI peripheral/bus to use (e.g. `&VSPI`, `&HSPI`, `&SPI`). */
    SPIClass* spi = nullptr;
    /** @brief SCK pin; -1 = bus default. */
    int sck = -1;
    /** @brief MISO pin; -1 = bus default. */
    int miso = -1;
    /** @brief MOSI pin; -1 = bus default. */
    int mosi = -1;
    /** @brief Chip-select pin; required on most boards. */
    int cs = -1;
    /** @brief SPI clock in Hz; conservative default of 4 MHz. */
    uint32_t frequencyHz = 4000000;
};

/** @brief Default SD SPI clock, in Hz. */
inline constexpr uint32_t kSdDefaultFrequencyHz = 4000000;

}  // namespace pixelroot32::persistence

#endif  // PIXELROOT32_ENABLE_SD_STORAGE && defined(ESP32)
