/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#ifdef ESP32

#include "audio/AudioScheduler.h"
#include "audio/ApuCore.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace pixelroot32::audio {

    /**
     * @class ESP32AudioScheduler
     * @brief Audio scheduler for ESP32 targets.
     *
     * Inherits from AudioScheduler.
     *
     * The I2S/DAC backend creates the FreeRTOS task and calls generateSamples();
     * this class does not spawn its own task. All synthesis is delegated to
     * ApuCore so ESP32-classic / S3 / C3 share the exact same logic.
     * Constructor arguments are reserved for API stability.
     */
    class ESP32AudioScheduler : public AudioScheduler {
    public:
        ESP32AudioScheduler(int reservedCoreId = 0,
                            int reservedTaskPriority = configMAX_PRIORITIES - 1);
        ~ESP32AudioScheduler() override;

        void init(AudioBackend* backend, int sampleRate,
                  const pixelroot32::platforms::PlatformCapabilities& caps) override;
        void submitCommand(const AudioCommand& cmd) override;
        void start() override;
        void stop() override;
        bool isIndependent() const override { return true; }
        void generateSamples(int16_t* stream, int length) override;
        bool isMusicPlaying() const override { return apu.isMusicPlaying(); }
        bool isMusicPaused()  const override { return apu.isMusicPaused(); }
        ApuCore& getApuCore() override { return apu; }

        const ApuCore& core() const { return apu; }
        ApuCore& core() { return apu; }

    private:
        ApuCore apu;
        AudioBackend* backend = nullptr;
        TaskHandle_t taskHandle = nullptr;
        volatile bool running = false;
    };

} // namespace pixelroot32::audio

#endif // ESP32
