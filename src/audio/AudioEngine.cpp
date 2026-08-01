/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/AudioEngine.h"
#include "core/Log.h"
#include <pixelroot32/apu/ApuConfig.h>
#ifdef ESP32
#include "drivers/esp32/ESP32AudioScheduler.h"
#elif defined(PLATFORM_NATIVE)
#include "drivers/native/NativeAudioScheduler.h"
#endif
#include <cstring>
#include <cmath>
#include <algorithm>

namespace pixelroot32::audio {

    namespace platforms = pixelroot32::platforms;

    AudioEngine::AudioEngine(const AudioConfig& config, const platforms::PlatformCapabilities& caps) 
        : config(config), capabilities(caps) {
        #ifdef ESP32
                scheduler = std::unique_ptr<ESP32AudioScheduler>(new ESP32AudioScheduler(caps.audioCoreId, caps.audioPriority));
        #elif defined(PLATFORM_NATIVE) && !defined(UNIT_TEST)
                scheduler = std::unique_ptr<NativeAudioScheduler>(new NativeAudioScheduler());
        #else
                scheduler = std::unique_ptr<DefaultAudioScheduler>(new DefaultAudioScheduler());
        #endif
    }

    void AudioEngine::init() {
        // Route the shared APU library's diagnostics through the engine logger.
        apuSetLogHandler([](ApuLogLevel level, const char* message) {
            namespace logging = pixelroot32::core::logging;
            logging::log(level == ApuLogLevel::Warning
                             ? logging::LogLevel::Warning
                             : logging::LogLevel::Info,
                         "%s", message);
        });
        if (scheduler) {
            scheduler->init(config.backend, config.sampleRate, capabilities, config.blockSize);
            scheduler->getApuCore().setPostMixMono(config.postMixMono, config.postMixUser);
            scheduler->start();
        }
        if (config.backend) {
            config.backend->init(this, capabilities);
        }
    }

    void AudioEngine::reinitSampleRate(int freq) {
        if (scheduler) {
            scheduler->getApuCore().init(freq);
        }
    }

    void AudioEngine::generateSamples(int16_t* stream, int length) {
        if (scheduler) {
            scheduler->generateSamples(stream, length);
        }
    }

    void AudioEngine::playEvent(const AudioEvent& event) {
        AudioCommand cmd;
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event = event;
        submitCommand(cmd);
    }

    void AudioEngine::setMasterVolume(float volume) {
        masterVolume = volume;
        if (masterVolume > 1.0f) masterVolume = 1.0f;
        if (masterVolume < 0.0f) masterVolume = 0.0f;
        AudioCommand cmd;
        cmd.type = AudioCommandType::SET_MASTER_VOLUME;
        cmd.volume = masterVolume;
        submitCommand(cmd);
    }

    float AudioEngine::getMasterVolume() const {
        return masterVolume;
    }

    void AudioEngine::setMasterBitcrush(uint8_t bits) {
        if (bits > 15u) bits = 15u;
        masterBitcrushBits = bits;
        AudioCommand cmd;
        cmd.type = AudioCommandType::SET_MASTER_BITCRUSH;
        cmd.masterBitcrushBits = bits;
        submitCommand(cmd);
    }

    uint8_t AudioEngine::getMasterBitcrush() const {
        return masterBitcrushBits;
    }

    void AudioEngine::submitCommand(const AudioCommand& cmd) {
        if (scheduler) {
            scheduler->submitCommand(cmd);
        }
    }

    bool AudioEngine::isMusicPlaying() const {
        return scheduler ? scheduler->isMusicPlaying() : false;
    }

    bool AudioEngine::isMusicPaused() const {
        return scheduler ? scheduler->isMusicPaused() : false;
    }

    void AudioEngine::setScheduler(std::unique_ptr<AudioScheduler> newScheduler) {
        if (newScheduler) {
            newScheduler->init(config.backend, config.sampleRate, capabilities, config.blockSize);
            newScheduler->start();
            scheduler = std::move(newScheduler);
        }
    }

}
