/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(PIXELROOT32_USE_DAC_AUDIO)

#include "audio/AudioBackend.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/dac.h>
#include <driver/i2s.h>

namespace pixelroot32::drivers::esp32 {

    /**
     * @class ESP32_DAC_AudioBackend
     * @brief Audio backend for ESP32 classic / S2 internal 8-bit DAC.
     *
     * Inherits from AudioBackend.
     *
     * Uses **I2S in DAC-built-in mode** so samples are pushed to the DAC via
     * DMA instead of the previous per-sample `dacWrite()` spin loop. This
     * frees ~15-25% of CPU on Core 0 at 22050 Hz and removes the per-sample
     * pinmux/mutex overhead that `dacWrite()` incurs on every call.
     *
     * Target: ESP32 (original), ESP32-S2. The DAC does not exist on S3/C3.
     */
    class ESP32_DAC_AudioBackend : public pixelroot32::audio::AudioBackend {
    public:
        /**
         * @param dacPin GPIO (must be 25 for DAC1 or 26 for DAC2).
         * @param sampleRate Audio sample rate (default 22050).
         */
        ESP32_DAC_AudioBackend(int dacPin = 25, int sampleRate = 22050);
        virtual ~ESP32_DAC_AudioBackend();

        void init(pixelroot32::audio::AudioEngine* engine,
                  const pixelroot32::platforms::PlatformCapabilities& caps) override;
        int getSampleRate() const override { return sampleRate; }

        void audioTaskLoop();

    private:
        int dacPin;
        int sampleRate;
        dac_channel_t dacChannel;
        i2s_dac_mode_t dacMode = I2S_DAC_CHANNEL_DISABLE;
        pixelroot32::audio::AudioEngine* engineInstance = nullptr;
        TaskHandle_t audioTaskHandle = nullptr;
        bool i2sInstalled = false;
        bool isSingleCore = false;
    };

}

#endif // ARDUINO_ARCH_ESP32 && PIXELROOT32_USE_DAC_AUDIO
