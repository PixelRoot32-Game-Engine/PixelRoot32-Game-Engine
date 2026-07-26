/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioEngine.h"
#include "AudioTypes.h"

#include <cstdint>

namespace pixelroot32::audio {

/**
 * @brief Schedules delayed SFX events for `playSfxBank` sequence steps.
 *
 * Games implement this with a scene timer / command queue. The helper does not
 * own timing and does not allocate.
 *
 * Looping events (`AudioEvent::loop == true`) stay active until the game sends
 * `AudioCommandType::STOP_CHANNEL` (or the voice is stolen). This helper does
 * not auto-stop loops and does not manage cooldowns or global SFX volume.
 */
class SfxDelayScheduler {
public:
    virtual ~SfxDelayScheduler() = default;

    /**
     * @brief Schedule `playEvent(event)` after `delaySec` seconds from effect start.
     * @param delaySec Delay relative to the `playSfxBank` call (>= 0).
     * @param event Event to play when the delay elapses.
     */
    virtual void schedule(float delaySec, const AudioEvent& event) = 0;
};

/**
 * @brief No-op scheduler for banks that only use simultaneous layers (no delayed steps).
 */
class NullSfxDelayScheduler final : public SfxDelayScheduler {
public:
    void schedule(float /*delaySec*/, const AudioEvent& /*event*/) override {}
};

/**
 * @brief Plays every scheduled event immediately (ignores delay). Test / stub only.
 */
class ImmediateSfxDelayScheduler final : public SfxDelayScheduler {
public:
    explicit ImmediateSfxDelayScheduler(AudioEngine& engine) : engine_(&engine) {}

    void schedule(float /*delaySec*/, const AudioEvent& event) override {
        if (engine_ != nullptr) {
            engine_->playEvent(event);
        }
    }

private:
    AudioEngine* engine_ = nullptr;
};

/**
 * @brief Play a Tool Suite–style SFX bank entry: layers at t=0 + timed sequence steps.
 *
 * @tparam Bank Struct with static API compatible with Tool Suite export:
 *   - `static uint8_t layerCount(SfxId)`
 *   - `static AudioEvent layerEvent(SfxId, uint8_t)`
 *   - `static uint8_t sequenceStepCount(SfxId)`
 *   - `static SequenceStep sequenceStep(SfxId, uint8_t)` where
 *     `SequenceStep` has `float delaySec` and `AudioEvent event`
 * @tparam SfxId Bank effect id type (typically the game's `enum class SfxId`)
 *
 * Layers are dispatched with `AudioEngine::playEvent` immediately. Sequence
 * steps with `delaySec <= 0` also play immediately; steps with `delaySec > 0`
 * are handed to @p scheduler.
 *
 * Opt-in: games that only iterate `layerCount` / `layerEvent` need not call this.
 *
 * @code
 * // Recommended game usage with an exported bank:
 * //   playSfxBank<MySfxBank>(audio, SfxId::Coin, mySceneTimerScheduler);
 * //
 * // Stop a looping voice later:
 * //   AudioCommand stop{};
 * //   stop.type = AudioCommandType::STOP_CHANNEL;
 * //   stop.channelIndex = voiceSlot;
 * //   audio.submitCommand(stop);
 * @endcode
 */
template <typename Bank, typename SfxId>
inline void playSfxBank(AudioEngine& engine, SfxId id, SfxDelayScheduler& scheduler) {
    const uint8_t layer_count = Bank::layerCount(id);
    for (uint8_t i = 0; i < layer_count; ++i) {
        engine.playEvent(Bank::layerEvent(id, i));
    }

    const uint8_t step_count = Bank::sequenceStepCount(id);
    for (uint8_t i = 0; i < step_count; ++i) {
        const auto step = Bank::sequenceStep(id, i);
        if (step.delaySec <= 0.0f) {
            engine.playEvent(step.event);
        } else {
            scheduler.schedule(step.delaySec, step.event);
        }
    }
}

}  // namespace pixelroot32::audio
