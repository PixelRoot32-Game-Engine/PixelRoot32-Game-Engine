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
