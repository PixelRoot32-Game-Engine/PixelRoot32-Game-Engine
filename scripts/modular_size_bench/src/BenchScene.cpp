#include "BenchScene.h"

#include <core/Engine.h>
#include <graphics/Color.h>
#include <graphics/Renderer.h>

#if PIXELROOT32_ENABLE_PARTICLES
#include <graphics/particles/ParticlePresets.h>
#endif

#if PIXELROOT32_ENABLE_AUDIO
#include <audio/AudioTypes.h>
#endif

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace modular_size_bench {

namespace gfx = pr32::graphics;

BenchScene::BenchScene()
#if PIXELROOT32_ENABLE_PARTICLES
    : emitter_(pr32::math::Vector2(64, 64), gfx::particles::ParticlePresets::Sparks)
#endif
{
}

void BenchScene::init() {
#if PIXELROOT32_ENABLE_UI_SYSTEM
    label_ = std::make_unique<gfx::ui::UILabel>(
        "SizeBench",
        pr32::math::Vector2(8, 8),
        gfx::Color::White,
        1
    );
    addEntity(label_.get());

    button_ = std::make_unique<gfx::ui::UIButton>(
        "Go",
        0,
        pr32::math::Vector2(8, 24),
        pr32::math::Vector2(40, 12),
        nullptr
    );
    addEntity(button_.get());
    (void)button_->getSelected();
#endif

#if PIXELROOT32_ENABLE_PARTICLES
    addEntity(&emitter_);
    emitter_.burst(pr32::math::Vector2(64, 64), 8);
#endif

#if PIXELROOT32_ENABLE_PHYSICS
    addEntity(&body_);
#endif

#if PIXELROOT32_ENABLE_AUDIO
    // Touch audio API so playEvent / MusicPlayer paths stay linked.
    pr32::audio::AudioEvent beep{};
    beep.type = pr32::audio::WaveType::PULSE;
    beep.frequency = 440.0f;
    beep.duration = 0.05f;
    beep.volume = 0.2f;
    beep.duty = 0.5f;
    engine.getAudioEngine().playEvent(beep);
    (void)engine.getMusicPlayer().isPlaying();
#endif

#if PIXELROOT32_ENABLE_NETWORKING
    // Registering the session links Engine::update()'s dispatch path, and the
    // pre-init calls below link the session and protocol entry points. Every
    // one of them is a documented no-op or NotInitialized answer before init(),
    // so no radio is started; the bench still pays the same flash and static
    // RAM the module costs.
    engine.setNetworkSession(&netSession_);
    netSession_.update(0);
    pr32::networking::NetEvent netEvent{};
    (void)netSession_.pollEvent(netEvent);
    (void)netSession_.peerCount();
    (void)netSession_.send(0, pr32::networking::NetDelivery::Unreliable, nullptr, 0);
#endif

#if PIXELROOT32_ENABLE_PERSISTENCE
    // Drive the service and the record codec once so both stay linked. With no
    // store bound, saveRecord answers Unavailable without touching a medium,
    // and encode/decode run over a stack buffer: the module is measured and no
    // NVS partition is opened.
    uint8_t record[pr32::persistence::RecordCodec::kHeaderSize + 4] = {};
    const uint8_t payload[4] = {0x50, 0x52, 0x33, 0x32};
    size_t written = 0;
    (void)pr32::persistence::RecordCodec::encode(
        1, payload, sizeof(payload), record, sizeof(record), written
    );
    const uint8_t* decoded = nullptr;
    pr32::persistence::RecordInfo info{};
    (void)pr32::persistence::RecordCodec::decode(record, written, decoded, info);
    (void)pr32::persistence::crc32(payload, sizeof(payload));
    (void)persistence_.saveRecord(0, 1, payload, sizeof(payload), record, sizeof(record));
    (void)persistence_.hasSettings();
#endif
}

void BenchScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime);
}

void BenchScene::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(
        0, 0,
        renderer.getLogicalWidth(),
        renderer.getLogicalHeight(),
        gfx::Color::Black
    );
    Scene::draw(renderer);
}

} // namespace modular_size_bench
