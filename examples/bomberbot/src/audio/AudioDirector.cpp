#include "AudioDirector.h"

namespace bomberbot {

AudioDirector& AudioDirector::instance() {
    static AudioDirector director;
    return director;
}

#if PIXELROOT32_ENABLE_AUDIO
void AudioDirector::bind(pixelroot32::audio::AudioEngine* engine) {
    engine_ = engine;
    if (engine_) {
        engine_->setMasterBitcrush(8);
    }
}
#endif

namespace {

#if PIXELROOT32_ENABLE_AUDIO
#if defined(PLATFORM_ESP32DEV) || defined(PLATFORM_ESP32S3)
constexpr float kDacMaxFrequencyHz = 4500.0f;

void clampEventForDac(pixelroot32::audio::AudioEvent& event) {
    if (event.frequency > kDacMaxFrequencyHz) {
        event.frequency = kDacMaxFrequencyHz;
    }
    if (event.sweepEndHz > kDacMaxFrequencyHz) {
        event.sweepEndHz = kDacMaxFrequencyHz;
    }
}
#endif

void dispatchEvent(pixelroot32::audio::AudioEngine* engine,
                   pixelroot32::audio::AudioEvent event,
                   float sfxVolume) {
    if (!engine) {
        return;
    }
    event.volume *= sfxVolume;
#if defined(PLATFORM_ESP32DEV) || defined(PLATFORM_ESP32S3)
    clampEventForDac(event);
#endif
    engine->playEvent(event);
}
#endif

constexpr unsigned long cooldownFor(SfxId id) {
#if PIXELROOT32_ENABLE_AUDIO
    return SfxBank::cooldownMs(id);
#else
    (void)id;
    return 0;
#endif
}
} // namespace

unsigned long AudioDirector::estimateSfxDurationMs(SfxId id) {
#if PIXELROOT32_ENABLE_AUDIO
    float endSec = 0.0f;
    const uint8_t layers = SfxBank::layerCount(id);
    for (uint8_t i = 0; i < layers; ++i) {
        const float duration = SfxBank::layerEvent(id, i).duration;
        if (duration > endSec) {
            endSec = duration;
        }
    }
    const uint8_t steps = SfxBank::sequenceStepCount(id);
    for (uint8_t i = 0; i < steps; ++i) {
        const SfxBank::SequenceStep step = SfxBank::sequenceStep(id, i);
        const float stepEnd = step.delaySec + step.event.duration;
        if (stepEnd > endSec) {
            endSec = stepEnd;
        }
    }
    const unsigned long ms = static_cast<unsigned long>(endSec * 1000.0f + 0.5f);
    return ms > 0UL ? ms : 1UL;
#else
    (void)id;
    return 0;
#endif
}

#if PIXELROOT32_ENABLE_AUDIO
void AudioDirector::clearPendingSfx() {
    for (uint8_t i = 0; i < kMaxPendingSfx; ++i) {
        pendingSfx_[i].active = false;
        pendingSfx_[i].remainingMs = 0;
    }
}

void AudioDirector::enqueueDelayedSfx(
    float delaySec, const pixelroot32::audio::AudioEvent& event) {
    if (delaySec <= 0.0f) {
        dispatchEvent(engine_, event, sfxVolume_);
        return;
    }

    unsigned long delayMs = static_cast<unsigned long>(delaySec * 1000.0f + 0.5f);
    if (delayMs == 0) {
        delayMs = 1;
    }

    for (uint8_t i = 0; i < kMaxPendingSfx; ++i) {
        if (!pendingSfx_[i].active) {
            pendingSfx_[i].active = true;
            pendingSfx_[i].remainingMs = delayMs;
            pendingSfx_[i].event = event;
            return;
        }
    }
    // Queue full — play immediately rather than drop the step.
    dispatchEvent(engine_, event, sfxVolume_);
}

void AudioDirector::updatePendingSfx(unsigned long dtMs) {
    for (uint8_t i = 0; i < kMaxPendingSfx; ++i) {
        if (!pendingSfx_[i].active) {
            continue;
        }
        if (pendingSfx_[i].remainingMs > dtMs) {
            pendingSfx_[i].remainingMs -= dtMs;
            continue;
        }
        pendingSfx_[i].active = false;
        pendingSfx_[i].remainingMs = 0;
        dispatchEvent(engine_, pendingSfx_[i].event, sfxVolume_);
    }
}
#endif

void AudioDirector::playSfx(SfxId id) {
    if (!enabled_) {
        return;
    }
    const size_t idx = static_cast<size_t>(id);
    if (idx >= static_cast<size_t>(SfxId::Count)) {
        return;
    }
    if (cooldownMs_[idx] > 0) {
        return;
    }

#if PIXELROOT32_ENABLE_AUDIO
    if (!engine_) {
        return;
    }

    // Layers at t=0 (same as playSfxBank), with game volume / DAC clamp.
    const uint8_t layers = SfxBank::layerCount(id);
    for (uint8_t i = 0; i < layers; ++i) {
        dispatchEvent(engine_, SfxBank::layerEvent(id, i), sfxVolume_);
    }

    // Timed sequence steps (Death fanfare, StageClear arpeggio, etc.).
    const uint8_t steps = SfxBank::sequenceStepCount(id);
    for (uint8_t i = 0; i < steps; ++i) {
        const SfxBank::SequenceStep step = SfxBank::sequenceStep(id, i);
        enqueueDelayedSfx(step.delaySec, step.event);
    }
#endif

    cooldownMs_[idx] = cooldownFor(id);
}

void AudioDirector::update(unsigned long dtMs) {
    for (size_t i = 0; i < static_cast<size_t>(SfxId::Count); ++i) {
        if (cooldownMs_[i] > dtMs) {
            cooldownMs_[i] -= dtMs;
        } else {
            cooldownMs_[i] = 0;
        }
    }

#if PIXELROOT32_ENABLE_AUDIO
    updatePendingSfx(dtMs);
#endif
}

bool AudioDirector::canPlay(SfxId id) const {
    if (!enabled_) {
        return false;
    }
#if PIXELROOT32_ENABLE_AUDIO
    if (!engine_) {
        return false;
    }
#endif
    const size_t idx = static_cast<size_t>(id);
    if (idx >= static_cast<size_t>(SfxId::Count)) {
        return false;
    }
    return cooldownMs_[idx] == 0;
}

void AudioDirector::setEnabled(bool enabled) {
    enabled_ = enabled;
}

void AudioDirector::resetCooldowns() {
    for (size_t i = 0; i < static_cast<size_t>(SfxId::Count); ++i) {
        cooldownMs_[i] = 0;
    }
#if PIXELROOT32_ENABLE_AUDIO
    clearPendingSfx();
#endif
}

void AudioDirector::setSfxVolume(float volume) {
#if PIXELROOT32_ENABLE_AUDIO
    sfxVolume_ = volume;
#else
    (void)volume;
#endif
}

unsigned long AudioDirector::getCooldownRemainingMs(SfxId id) const {
    const size_t idx = static_cast<size_t>(id);
    if (idx >= static_cast<size_t>(SfxId::Count)) {
        return 0;
    }
    return cooldownMs_[idx];
}

} // namespace bomberbot
