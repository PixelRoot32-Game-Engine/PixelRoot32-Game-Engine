#pragma once

#include <core/Scene.h>
#include <core/EngineModules.h>
#include <memory>

#if PIXELROOT32_ENABLE_UI_SYSTEM
#include <graphics/ui/UILabel.h>
#include <graphics/ui/UIButton.h>
#endif

#if PIXELROOT32_ENABLE_PARTICLES
#include <graphics/particles/ParticleEmitter.h>
#endif

#if PIXELROOT32_ENABLE_PHYSICS
#include <core/PhysicsActor.h>
#include <graphics/Renderer.h>
#endif

namespace modular_size_bench {

#if PIXELROOT32_ENABLE_PHYSICS
class BenchBody : public pixelroot32::core::PhysicsActor {
public:
    BenchBody()
        : pixelroot32::core::PhysicsActor(
              pixelroot32::math::toScalar(40),
              pixelroot32::math::toScalar(40),
              8,
              8
          ) {}

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawFilledRectangle(
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            width,
            height,
            pixelroot32::graphics::Color::White
        );
    }
};
#endif

class BenchScene : public pixelroot32::core::Scene {
public:
    BenchScene();
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
#if PIXELROOT32_ENABLE_UI_SYSTEM
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> label_;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> button_;
#endif

#if PIXELROOT32_ENABLE_PARTICLES
    // Value member so the emitter pool counts in static RAM (.bss).
    pixelroot32::graphics::particles::ParticleEmitter emitter_;
#endif

#if PIXELROOT32_ENABLE_PHYSICS
    // Value member so physics actor storage counts in static RAM.
    BenchBody body_;
#endif
};

} // namespace modular_size_bench
