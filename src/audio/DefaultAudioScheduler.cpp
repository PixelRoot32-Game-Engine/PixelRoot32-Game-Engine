/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/DefaultAudioScheduler.h"

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
        apu.generateSamples(stream, length);
    }

} // namespace pixelroot32::audio
