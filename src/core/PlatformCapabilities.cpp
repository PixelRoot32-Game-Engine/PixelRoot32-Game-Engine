/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "core/PlatformCapabilities.h"
#include "PlatformDefaults.h"

#ifdef ESP32
#include <Arduino.h>
#include <esp_system.h>
#include <sdkconfig.h>
#endif

namespace pixelroot32::core {

PlatformCapabilities PlatformCapabilities::detect() {
    PlatformCapabilities caps;

#ifdef ESP32
    // On ESP32, we can check the number of cores
#ifdef CONFIG_FREERTOS_UNICORE
    caps.hasDualCore = false;
    caps.coreCount = 1;
    caps.audioCoreId = 0;
    caps.mainCoreId = 0;
#else
    caps.hasDualCore = true;
    caps.coreCount = 2;
    caps.audioCoreId = PR32_DEFAULT_AUDIO_CORE; // Use defaults from PlatformDefaults.h
    caps.mainCoreId = PR32_DEFAULT_MAIN_CORE;   // Use defaults from PlatformDefaults.h
#endif

    // Basic feature detection
#ifdef CONFIG_ESP32_WIFI_ENABLED
    caps.hasWifi = true;
#endif

#ifdef CONFIG_BT_ENABLED
    caps.hasBluetooth = true;
#endif

    caps.audioPriority = 5; // Default FreeRTOS priority for audio

#elif defined(PLATFORM_NATIVE)
    // For Native (SDL2), we simulate dual-core behavior with threads
    caps.hasDualCore = true; 
    caps.coreCount = 4; // Arbitrary for PC
    caps.audioCoreId = 0;
    caps.mainCoreId = 0;
    caps.audioPriority = 5;
#else
    // Default fallback
    caps.hasDualCore = false;
    caps.coreCount = 1;
    caps.audioCoreId = 0;
    caps.mainCoreId = 0;
    caps.audioPriority = 1;
#endif

    return caps;
}

} // namespace pixelroot32::core
