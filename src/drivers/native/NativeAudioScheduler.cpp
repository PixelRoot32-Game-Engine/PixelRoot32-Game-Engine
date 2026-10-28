/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef PLATFORM_NATIVE

#include "drivers/native/NativeAudioScheduler.h"
#include "core/Log.h"

#include <chrono>
#include <cstring>

namespace pixelroot32::audio {

    namespace logging = pixelroot32::core::logging;
    using logging::LogLevel;
    using logging::log;

    NativeAudioScheduler::NativeAudioScheduler(size_t ringBufferSize)
        : rbCapacity(ringBufferSize) {
        ringBuffer.resize(rbCapacity);
    }

    NativeAudioScheduler::~NativeAudioScheduler() {
        stop();
    }

    void NativeAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate,
                                    const pixelroot32::platforms::PlatformCapabilities& /*caps*/, int blkSize) {
        // S12: the render buffer is a fixed member, so the block has to be a
        // multiple of 128 and fit PR32_AUDIO_MAX_BLOCK_SAMPLES — the same rule
        // the ESP32 backends apply. A rejected block leaves blockSize at 0, so
        // start() spawns no thread instead of rendering past renderBuffer_.
        if (blkSize <= 0 || blkSize > PR32_AUDIO_MAX_BLOCK_SAMPLES || (blkSize % 128) != 0) {
            log(LogLevel::Error,
                "[NativeAudioScheduler] invalid blockSize %d: must be a multiple of 128 in (0, %d]",
                blkSize, PR32_AUDIO_MAX_BLOCK_SAMPLES);
            blockSize = 0;
            return;
        }
        apu.init(sampleRate);
        blockSize = blkSize;
    }

    void NativeAudioScheduler::submitCommand(const AudioCommand& cmd) {
        apu.submitCommand(cmd);
    }

    void NativeAudioScheduler::start() {
        if (running.load(std::memory_order_acquire)) return;
        // init() rejected the block (blockSize == 0): no thread, no rendering.
        if (blockSize <= 0) return;
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
        while (running.load(std::memory_order_acquire)) {
            if (rbAvailableToWrite() >= (size_t)blockSize) {
                // S12: one block-sized call per block, straight into the member
                // buffer, so the output is exactly what a single
                // generateSamples(stream, blockSize) call produces. The block
                // cannot exceed the buffer: init() rejects anything else.
                apu.generateSamples(renderBuffer_, blockSize);
                rbWrite(renderBuffer_, (size_t)blockSize);
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
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
