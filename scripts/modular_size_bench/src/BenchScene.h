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

#if PIXELROOT32_ENABLE_NETWORKING
#include <networking/NetSession.h>
#endif

#if PIXELROOT32_ENABLE_PERSISTENCE
#include <persistence/PersistenceService.h>
#include <persistence/RecordCodec.h>
#endif

namespace modular_size_bench {

#if PIXELROOT32_ENABLE_NETWORKING
/**
 * @class BenchNetTransport
 * @brief Inert link stand-in used only to keep the networking module linked.
 *
 * The flag gates the engine module (session, protocol, counters), not a
 * platform backend: constructing ESP32EspNowTransport would pull the whole
 * Wi-Fi/lwIP stack in and drown the module in a ~600 KB delta that measures
 * the radio stack, not the module. The stub gives NetSession a transport
 * reference without linking any backend, and no method is ever called with a
 * live session, so nothing here initializes a radio.
 */
class BenchNetTransport final : public pixelroot32::networking::NetTransport {
public:
    pixelroot32::networking::NetResult init(
        const pixelroot32::networking::NetTransportConfig&) override {
        return pixelroot32::networking::NetResult::Ok;
    }

    void shutdown() override {}

    pixelroot32::networking::NetResult addPeer(
        const pixelroot32::networking::NetPeerConfig&) override {
        return pixelroot32::networking::NetResult::Ok;
    }

    pixelroot32::networking::NetResult removePeer(
        const pixelroot32::networking::NetAddress&) override {
        return pixelroot32::networking::NetResult::Ok;
    }

    pixelroot32::networking::NetResult send(
        const pixelroot32::networking::NetAddress&, const uint8_t*,
        uint16_t) override {
        return pixelroot32::networking::NetResult::Ok;
    }

    bool pollRx(pixelroot32::networking::NetRxInfo&) override { return false; }

    bool pollTxResult(pixelroot32::networking::NetTxResult&) override { return false; }

    pixelroot32::networking::NetAddress localAddress() const override {
        return pixelroot32::networking::NetAddress{};
    }

    pixelroot32::networking::NetResult setChannel(uint8_t) override {
        return pixelroot32::networking::NetResult::Ok;
    }

    bool isReady() const override { return false; }
};
#endif

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

#if PIXELROOT32_ENABLE_NETWORKING
    // Value members, mirroring the physics/particles pattern: the session's
    // fixed peer table, event queue and pending-frame queue are the module's
    // real static cost. The transport is the inert stub above, so the delta
    // covers the engine module and not the ESP-NOW/Wi-Fi stack.
    BenchNetTransport netTransport_;
    pixelroot32::networking::NetSession netSession_{netTransport_};
#endif

#if PIXELROOT32_ENABLE_PERSISTENCE
    // The engine service holds the settings and saves roles; a game binds its
    // own store adapters. The flag gates the module, not a driver, so the
    // engine-side service and codec are what this row measures; the ESP32 NVS
    // adapters add their own driver cost when a game links them.
    pixelroot32::persistence::PersistenceService persistence_;
#endif
};

} // namespace modular_size_bench
