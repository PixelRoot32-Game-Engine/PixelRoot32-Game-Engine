/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

namespace pixelroot32::platforms {

    /**
     * @struct PlatformCapabilities
     * @brief Represents the hardware capabilities of the current platform.
     * 
     * This structure allows the engine to adapt to different hardware configurations
     * (e.g., single-core vs dual-core ESP32) without excessive #ifdefs.
     */
    struct PlatformCapabilities {
        bool hasDualCore = false;
        bool hasWifi = false;
        bool hasBluetooth = false;
        int coreCount = 1;
        
        /**
         * @brief Recommended core ID for audio processing.
         * On dual-core ESP32, this is typically 0.
         * On single-core, it's 0.
         */
        int audioCoreId = 0;

        /**
         * @brief Recommended core ID for the main game loop.
         * On dual-core ESP32, this is typically 1.
         * On single-core, it's 0.
         */
        int mainCoreId = 0;

        /**
         * @brief Recommended task priority for audio.
         */
        int audioPriority = 5;

        /**
         * @brief Detects capabilities of the current platform.
         * @return A populated PlatformCapabilities struct.
         */
        static PlatformCapabilities detect();
    };

} // namespace pixelroot32::platforms
