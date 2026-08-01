/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/DefaultAudioScheduler.h"

#include <cstring>

namespace pixelroot32::audio {

    namespace platforms = pixelroot32::platforms;

    void DefaultAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate,
                                     const platforms::PlatformCapabilities& /*caps*/, int /*blockSize*/) {
        apu.init(sampleRate);
    }

    void DefaultAudioScheduler::submitCommand(const AudioCommand& cmd) {
        apu.submitCommand(cmd);
    }

    void DefaultAudioScheduler::start() { running = true; }
    void DefaultAudioScheduler::stop()  { running = false; }

    bool DefaultAudioScheduler::isIndependent() const { return false; }

    void DefaultAudioScheduler::generateSamples(int16_t* stream, int length) {
        if (!running) {
            if (stream && length > 0) {
                std::memset(stream, 0, static_cast<size_t>(length) * sizeof(int16_t));
            }
            return;
        }
        apu.generateSamples(stream, length);
    }

} // namespace pixelroot32::audio
