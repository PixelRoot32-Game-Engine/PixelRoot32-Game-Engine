/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "audio/AudioBackend.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/dac.h>

namespace pixelroot32::drivers::esp32 {

    /**
     * @class ESP32_DAC_AudioBackend
     * @brief Audio backend implementation for ESP32 using internal DAC via dacWrite.
     * 
     * Designed for simple 8-bit output (e.g., PAM8302A).
     * Uses a FreeRTOS task to drive the DAC output via direct register writing or dacWrite().
     * Does NOT use I2S.
     */
    class ESP32_DAC_AudioBackend : public pixelroot32::audio::AudioBackend {
    public:
        /**
         * @brief Construct a new ESP32_DAC_AudioBackend
         * 
         * @param dacPin The GPIO pin for DAC output (25 or 26).
         * @param sampleRate Audio sample rate (default 22050 or 11025 for better stability with this method).
         */
        ESP32_DAC_AudioBackend(int dacPin = 25, int sampleRate = 22050);
        virtual ~ESP32_DAC_AudioBackend();

        void init(pixelroot32::audio::AudioEngine* engine) override;
        int getSampleRate() const override { return sampleRate; }

        // Internal task function
        void audioTaskLoop();

    private:
        int dacPin;
        int sampleRate;
        dac_channel_t dacChannel;
        pixelroot32::audio::AudioEngine* engineInstance = nullptr;
        TaskHandle_t audioTaskHandle = nullptr;
    };

}

#endif // ARDUINO_ARCH_ESP32
