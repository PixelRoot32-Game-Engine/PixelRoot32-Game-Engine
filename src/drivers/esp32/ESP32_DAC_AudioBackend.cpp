/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "drivers/esp32/ESP32_DAC_AudioBackend.h"
#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_USE_DAC_AUDIO)
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
    }

    void ESP32_DAC_AudioBackend::init(pixelroot32::audio::AudioEngine* engine, const pixelroot32::core::PlatformCapabilities& caps) {
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

        // Create audio task pinned to core specified by capabilities
        xTaskCreatePinnedToCore(
            audioTaskTrampoline,
            "DACAudioTask",
            4096,
            this,
            caps.audioPriority,
            &audioTaskHandle,
            caps.audioCoreId
        );
        Serial.printf("[ESP32_DAC_AudioBackend] Task created on Core %d (Software mode)\n", caps.audioCoreId);
    }

    void ESP32_DAC_AudioBackend::audioTaskLoop() {
        const int BUFFER_SAMPLES = 64; 
        int16_t sampleBuffer[BUFFER_SAMPLES];
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        const TickType_t bufferTicks = pdMS_TO_TICKS((1000 * BUFFER_SAMPLES) / sampleRate);

        while (true) {
            if (engineInstance) {
                engineInstance->generateSamples(sampleBuffer, BUFFER_SAMPLES);

                for (int i = 0; i < BUFFER_SAMPLES; i++) {
                    // Apply 0.7f scale for PAM8302A
                    int32_t scaled = (int32_t)(sampleBuffer[i] * 0.7f);
                    
                    if (scaled > 32767) scaled = 32767;
                    if (scaled < -32768) scaled = -32768;

                    // Convert 16-bit signed to 8-bit unsigned
                    uint8_t dacValue = (uint8_t)((scaled + 32768) >> 8);
                    dac_output_voltage(dacChannel, dacValue);
                }
                
                vTaskDelay(1); 
                vTaskDelayUntil(&lastWakeTime, bufferTicks);
            } else {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }

}

#endif // PIXELROOT32_USE_DAC_AUDIO
