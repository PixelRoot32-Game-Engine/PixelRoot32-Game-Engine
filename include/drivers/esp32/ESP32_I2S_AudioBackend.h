#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include "audio/AudioBackend.h"
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace pixelroot32::drivers::esp32 {

    /**
     * @class ESP32_I2S_AudioBackend
     * @brief Audio backend implementation for ESP32 using I2S.
     * 
     * Uses a FreeRTOS task to continuously feed the I2S DMA buffer
     * to ensure smooth playback independent of the game loop frame rate.
     */
    class ESP32_I2S_AudioBackend : public pixelroot32::audio::AudioBackend {
    public:
        /**
         * @brief Construct a new ESP32_I2S_AudioBackend
         * 
         * @param bclkPin Bit Clock pin (BCLK)
         * @param wclkPin Word Clock / LR Clock pin (LRCK/WS)
         * @param doutPin Data Out pin (DIN/DOUT)
         * @param sampleRate Audio sample rate (default 22050)
         */
        ESP32_I2S_AudioBackend(int bclkPin, int wclkPin, int doutPin, int sampleRate = 22050);
        virtual ~ESP32_I2S_AudioBackend();

        void init(pixelroot32::audio::AudioEngine* engine) override;
        int getSampleRate() const override { return sampleRate; }

        // Internal task function
        void audioTaskLoop();

    private:
        int bclkPin;
        int wclkPin;
        int doutPin;
        int sampleRate;
        pixelroot32::audio::AudioEngine* engineInstance = nullptr;
        TaskHandle_t audioTaskHandle = nullptr;
    };

}

#endif // ARDUINO_ARCH_ESP32
