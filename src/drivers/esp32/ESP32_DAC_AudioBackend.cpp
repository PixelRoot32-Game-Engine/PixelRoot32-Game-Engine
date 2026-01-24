/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef ARDUINO_ARCH_ESP32

#include "drivers/esp32/ESP32_DAC_AudioBackend.h"
#include "audio/AudioEngine.h"
#include <Arduino.h>

namespace pixelroot32::drivers::esp32 {

    // FreeRTOS task wrapper
    static void audioTaskTrampoline(void* arg) {
        auto* backend = static_cast<ESP32_DAC_AudioBackend*>(arg);
        if (backend) {
            backend->audioTaskLoop();
        }
        vTaskDelete(NULL);
    }

    ESP32_DAC_AudioBackend::ESP32_DAC_AudioBackend(int dacPin, int sampleRate)
        : dacPin(dacPin), sampleRate(sampleRate) {}

    ESP32_DAC_AudioBackend::~ESP32_DAC_AudioBackend() {
        if (audioTaskHandle) {
            vTaskDelete(audioTaskHandle);
        }
        // No specific driver uninstall needed for simple dacWrite
    }

    void ESP32_DAC_AudioBackend::init(pixelroot32::audio::AudioEngine* engine) {
        this->engineInstance = engine;

        // Ensure pin is valid for DAC (25 or 26)
        if (dacPin != 25 && dacPin != 26) {
            Serial.println("Error: ESP32 DAC only available on pins 25 and 26");
            return;
        }

        // Map GPIO to DAC channel and enable it
        if (dacPin == 25) {
            dacChannel = DAC_CHANNEL_1;
        } else {
            dacChannel = DAC_CHANNEL_2;
        }
        dac_output_enable(dacChannel);

        // Create audio task pinned to Core 0
        xTaskCreatePinnedToCore(
            audioTaskTrampoline,
            "DACAudioTask",
            4096,
            this,
            configMAX_PRIORITIES - 1, // High priority
            &audioTaskHandle,
            0
        );
    }

    void ESP32_DAC_AudioBackend::audioTaskLoop() {
        const int BUFFER_SAMPLES = 64; // Small buffer for low latency software timing
        int16_t sampleBuffer[BUFFER_SAMPLES];
        
        // Use FreeRTOS tick timing at buffer granularity to avoid starving other tasks
        TickType_t lastWakeTime = xTaskGetTickCount();
        const TickType_t bufferTicks = pdMS_TO_TICKS((1000 * BUFFER_SAMPLES) / sampleRate);

        while (true) {
            if (engineInstance) {
                // Generate a small batch of samples
                engineInstance->generateSamples(sampleBuffer, BUFFER_SAMPLES);

                for (int i = 0; i < BUFFER_SAMPLES; i++) {
                    // Convert 16-bit signed (-32768 to 32767) to 8-bit unsigned (0-255)
                    // 1. Add 32768 -> 0 to 65535
                    // 2. Shift right by 8 -> 0 to 255
                    uint8_t dacValue = (sampleBuffer[i] + 32768) >> 8;

                    dac_output_voltage(dacChannel, dacValue);
                }
                vTaskDelayUntil(&lastWakeTime, bufferTicks);
            } else {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }

}

#endif // ARDUINO_ARCH_ESP32
