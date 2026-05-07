/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef ESP32

#include "drivers/esp32/ESP32AudioScheduler.h"

#include <Arduino.h>

namespace pixelroot32::audio {

    namespace platforms = pixelroot32::platforms;

    ESP32AudioScheduler::ESP32AudioScheduler(int /*reservedCoreId*/,
                                             int /*reservedTaskPriority*/) {}

    ESP32AudioScheduler::~ESP32AudioScheduler() {
        stop();
    }

    void ESP32AudioScheduler::init(AudioBackend* be, int sampleRate,
                                   const platforms::PlatformCapabilities& /*caps*/, int blkSize) {
        backend = be;
        apu.init(sampleRate);
        blockSize = blkSize;
    }

    void ESP32AudioScheduler::submitCommand(const AudioCommand& cmd) {
        apu.submitCommand(cmd);
    }

    void ESP32AudioScheduler::start() {
        if (running) return;
        running = true;
        // Backends create their own FreeRTOS task and drive generateSamples().
    }

    void ESP32AudioScheduler::stop() {
        running = false;
        if (taskHandle) {
            vTaskDelete(taskHandle);
            taskHandle = nullptr;
        }
    }

    void ESP32AudioScheduler::generateSamples(int16_t* stream, int length) {
        apu.generateSamples(stream, length);
    }

} // namespace pixelroot32::audio

#endif // ESP32
