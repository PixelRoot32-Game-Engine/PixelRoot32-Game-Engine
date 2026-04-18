/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef PLATFORM_NATIVE

#include "drivers/native/NativeAudioScheduler.h"

#include <chrono>
#include <cstring>

namespace pixelroot32::audio {

    NativeAudioScheduler::NativeAudioScheduler(size_t ringBufferSize)
        : rbCapacity(ringBufferSize) {
        ringBuffer.resize(rbCapacity);
    }

    NativeAudioScheduler::~NativeAudioScheduler() {
        stop();
    }

    void NativeAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate,
                                    const pixelroot32::platforms::PlatformCapabilities& /*caps*/) {
        apu.init(sampleRate);
    }

    void NativeAudioScheduler::submitCommand(const AudioCommand& cmd) {
        apu.submitCommand(cmd);
    }

    void NativeAudioScheduler::start() {
        if (running.load(std::memory_order_acquire)) return;
        running.store(true, std::memory_order_release);
        audioThread = std::thread(&NativeAudioScheduler::threadLoop, this);
    }

    void NativeAudioScheduler::stop() {
        if (!running.load(std::memory_order_acquire)) return;
        running.store(false, std::memory_order_release);
        if (audioThread.joinable()) audioThread.join();
    }

    void NativeAudioScheduler::generateSamples(int16_t* stream, int length) {
        if (!stream || length <= 0) return;

        const size_t available = rbAvailableToRead();
        if (available < (size_t)length) {
            rbRead(stream, available);
            std::memset(stream + available, 0, (length - available) * sizeof(int16_t));
        } else {
            rbRead(stream, length);
        }
    }

    void NativeAudioScheduler::threadLoop() {
        constexpr int CHUNK_SIZE = 128;
        int16_t chunk[CHUNK_SIZE];

        while (running.load(std::memory_order_acquire)) {
            if (rbAvailableToWrite() >= (size_t)CHUNK_SIZE) {
                apu.generateSamples(chunk, CHUNK_SIZE);
                rbWrite(chunk, CHUNK_SIZE);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    size_t NativeAudioScheduler::rbAvailableToRead() const {
        const size_t r = rbReadPos.load(std::memory_order_acquire);
        const size_t w = rbWritePos.load(std::memory_order_acquire);
        if (w >= r) return w - r;
        return rbCapacity - (r - w);
    }

    size_t NativeAudioScheduler::rbAvailableToWrite() const {
        const size_t r = rbReadPos.load(std::memory_order_acquire);
        const size_t w = rbWritePos.load(std::memory_order_acquire);
        const size_t avail = (w >= r) ? (rbCapacity - (w - r)) : (r - w);
        return avail > 0 ? avail - 1 : 0;
    }

    void NativeAudioScheduler::rbWrite(const int16_t* data, size_t count) {
        size_t w = rbWritePos.load(std::memory_order_relaxed);
        for (size_t i = 0; i < count; ++i) {
            ringBuffer[w] = data[i];
            w = (w + 1) % rbCapacity;
        }
        rbWritePos.store(w, std::memory_order_release);
    }

    void NativeAudioScheduler::rbRead(int16_t* data, size_t count) {
        size_t r = rbReadPos.load(std::memory_order_relaxed);
        for (size_t i = 0; i < count; ++i) {
            data[i] = ringBuffer[r];
            r = (r + 1) % rbCapacity;
        }
        rbReadPos.store(r, std::memory_order_release);
    }

} // namespace pixelroot32::audio

#endif // PLATFORM_NATIVE
