#pragma once

#include <cstdint>

#if PIXELROOT32_ENABLE_AUDIO
#include <audio/AudioEngine.h>
#include <audio/AudioTypes.h>
#endif

namespace bomberbot {

/// Game-owned SFX ids — must match exported SfxBank.cppSymbol cases.
enum class SfxId : uint8_t {
    PlaceBomb,
    Death,
    CoinBlip,
    MenuBlip,
    StageClear,
    PickupPowerSoft,
    BombExplosionTiny,
    EnemyDeath,
    Footstep,
    FootstepSoft,
    Count
};

class AudioDirector {
public:
    static AudioDirector& instance();

#if PIXELROOT32_ENABLE_AUDIO
    void bind(pixelroot32::audio::AudioEngine* engine);
#endif

    void playSfx(SfxId id);
    void update(unsigned long dtMs);
    bool canPlay(SfxId id) const;
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    void resetCooldowns();
    void setSfxVolume(float volume);

    /// Approximate audible length of an effect (layers + sequence steps) in ms.
    [[nodiscard]] static unsigned long estimateSfxDurationMs(SfxId id);

    unsigned long getCooldownRemainingMs(SfxId id) const;

private:
    AudioDirector() = default;

    bool enabled_ = true;
    unsigned long cooldownMs_[static_cast<size_t>(SfxId::Count)] = {};

#if PIXELROOT32_ENABLE_AUDIO
    void clearPendingSfx();
    void enqueueDelayedSfx(float delaySec, const pixelroot32::audio::AudioEvent& event);
    void updatePendingSfx(unsigned long dtMs);

    static constexpr uint8_t kMaxPendingSfx = 16;

    struct PendingSfx {
        unsigned long remainingMs = 0;
        pixelroot32::audio::AudioEvent event{};
        bool active = false;
    };

    pixelroot32::audio::AudioEngine* engine_ = nullptr;
    float sfxVolume_ = 0.70f;
    PendingSfx pendingSfx_[kMaxPendingSfx] = {};
#endif
};

} // namespace bomberbot
