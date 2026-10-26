/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the Scene::onNetworkEvent hook and the Engine drain/dispatch
 * path (slice N8).
 *
 * Engine only borrows a session: the application creates, initializes and
 * shuts it down, then registers it with setNetworkSession(). Every test here
 * generates events outside the Engine (peer sessions driven by an injected
 * clock), hand one session to the Engine and assert only what update()
 * delivered to the current scene.
 *
 * The networking module and the native loopback backend are compiled out
 * unless PIXELROOT32_ENABLE_NETWORKING is 1 on PLATFORM_NATIVE, so this file
 * follows the test_net_data_path pattern: the functional branch lives behind
 * both guards and the flag-off branch runs one stub that still proves the
 * inert contract under [env:native_test].
 *
 * The injected session clock starts at 0 while Engine::update() necessarily
 * reads the platform millis() (starting with N8 the Engine owns no clock).
 * The session config therefore uses a peer timeout far above any test runtime,
 * so the unrelated wall clock cannot emit a spurious PeerLeft mid-assertion.
 * No assertion depends on millis().
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"
#include "core/Engine.h"
#include "core/Scene.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

#include "../../mocks/MockDrawSurface.h"
#include "drivers/native/LoopbackTransport.h"
#include "graphics/DisplayConfig.h"
#include "networking/NetSession.h"
#include "networking/NetTransport.h"
#include "networking/NetTypes.h"

#include <cstdint>

namespace {

using pixelroot32::core::Engine;
using pixelroot32::core::Scene;
using pixelroot32::graphics::DisplayConfig;
using pixelroot32::graphics::MockDrawSurface;
using pixelroot32::networking::LoopbackBus;
using pixelroot32::networking::LoopbackTransport;
using pixelroot32::networking::NetAddress;
using pixelroot32::networking::NetDelivery;
using pixelroot32::networking::NetEvent;
using pixelroot32::networking::NetEventType;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetSession;
using pixelroot32::networking::NetSessionConfig;
using pixelroot32::networking::NetTransportConfig;
using pixelroot32::networking::kNetAddressSize;

// Room for the largest script below (two lifecycle events plus one update).
constexpr uint8_t kMaxRecordedCalls = 8;

// Well above any test runtime: keeps the millis() the Engine passes into
// NetSession::update() from expiring a peer whose injected last-seen is 0.
constexpr uint16_t kTestPeerTimeoutMs = 60000;

NetAddress makeAddress(uint8_t tag) {
    NetAddress address{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        address.bytes[i] = static_cast<uint8_t>(0x20u + tag + i);
    }
    return address;
}

NetTransportConfig makeTransportConfig() {
    NetTransportConfig transportConfig;
    transportConfig.channel = 1;
    transportConfig.nodeId = 0;
    transportConfig.manageWifi = false;
    return transportConfig;
}

NetSessionConfig makeSessionConfig(uint8_t nodeId, uint16_t bootId) {
    NetSessionConfig sessionConfig;
    sessionConfig.channel = 1;
    sessionConfig.nodeId = nodeId;
    sessionConfig.bootId = bootId;
    sessionConfig.discovery = true;
    sessionConfig.peerTimeoutMs = kTestPeerTimeoutMs;
    return sessionConfig;
}

// Two peers on one loopback bus. Declaration order matters: the addresses must
// exist before the transports that snapshot them, and the transports before
// the sessions that borrow them.
struct SessionPair {
    SessionPair(uint8_t firstTag, uint8_t secondTag)
        : firstAddress(makeAddress(firstTag)),
          secondAddress(makeAddress(secondTag)),
          firstTransport(bus, firstAddress),
          secondTransport(bus, secondAddress),
          first(firstTransport),
          second(secondTransport) {}

    LoopbackBus bus;
    NetAddress firstAddress;
    NetAddress secondAddress;
    LoopbackTransport firstTransport;
    LoopbackTransport secondTransport;
    NetSession first;
    NetSession second;
};

void assertNetResult(NetResult expected, NetResult actual) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(expected),
                                    static_cast<uint8_t>(actual),
                                    "unexpected NetResult");
}

void assertEventType(NetEventType expected, NetEventType actual) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(expected),
                                    static_cast<uint8_t>(actual),
                                    "unexpected NetEventType");
}

void initPair(SessionPair& pair, uint8_t firstNode, uint16_t firstBoot,
              uint8_t secondNode, uint16_t secondBoot) {
    assertNetResult(NetResult::Ok,
                    pair.firstTransport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    pair.secondTransport.init(makeTransportConfig()));
    assertNetResult(NetResult::Ok,
                    pair.first.init(makeSessionConfig(firstNode, firstBoot)));
    assertNetResult(NetResult::Ok,
                    pair.second.init(makeSessionConfig(secondNode, secondBoot)));
}

// The four interleaved updates two one-shot HELLOs need to be drained by both
// sides. The loopback bus clock is advanced first so delivery deadlines stay
// on the injected timeline.
void discoverPair(SessionPair& pair, uint32_t nowMs) {
    pair.bus.setNowMs(nowMs);
    pair.first.update(nowMs);
    pair.second.update(nowMs);
    pair.first.update(nowMs);
    pair.second.update(nowMs);
}

// One send tick on both peers, on the injected timeline.
void tickPair(SessionPair& pair, uint32_t nowMs) {
    pair.bus.setNowMs(nowMs);
    pair.first.update(nowMs);
    pair.second.update(nowMs);
}

void drainEvents(NetSession& session) {
    NetEvent event{};
    while (session.pollEvent(event)) {
    }
}

// Records both optional hooks in one shared sequence so a test can assert the
// dispatch-before-update ordering rather than inferring it.
class DispatchScene : public Scene {
public:
    enum class Call : uint8_t {
        NetworkEvent = 0,
        Update = 1
    };

    void update(unsigned long deltaTime) override {
        (void)deltaTime;
        ++updateCount;
        record(Call::Update);
    }

    void onNetworkEvent(const pixelroot32::networking::NetEvent& event) override {
        if (networkEventCount < kMaxRecordedCalls) {
            events[networkEventCount] = event;
        }
        lastEvent = event;
        ++networkEventCount;
        record(Call::NetworkEvent);
    }

    Call callAt(uint8_t index) const { return callOrder[index]; }

    uint8_t updateCount = 0;
    uint8_t networkEventCount = 0;
    uint8_t callCount = 0;
    Call callOrder[kMaxRecordedCalls]{};
    NetEvent events[kMaxRecordedCalls]{};
    NetEvent lastEvent{};

private:
    void record(Call call) {
        if (callCount < kMaxRecordedCalls) {
            callOrder[callCount] = call;
            ++callCount;
        }
    }
};

// Engine is constructed with a mock surface and initialized so the update path
// has a working input manager. Only the protected update() is exposed; the
// networking API under test stays public.
class TestEngine : public Engine {
public:
    explicit TestEngine(const DisplayConfig& displayConfig)
        : Engine(displayConfig) {}

    using Engine::update;
};

DisplayConfig makeDisplayConfig() {
    // DisplayConfig owns the surface, so no raw pointer survives the call.
    return PIXELROOT32_CUSTOM_DISPLAY(new MockDrawSurface(), 240, 240);
}

}  // namespace

// =============================================================================
// 1. One queued Message reaches the current scene exactly once, with the
//    sender's payload, peer slot and node label.
// =============================================================================

void test_net_engine_dispatch_message_once(void) {
    SessionPair pair(1, 2);
    initPair(pair, 11, 1, 22, 2);
    discoverPair(pair, 0);
    drainEvents(pair.first);
    drainEvents(pair.second);

    const uint8_t payload[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    assertNetResult(NetResult::Ok,
                    pair.first.send(0, NetDelivery::Unreliable, payload,
                                    static_cast<uint16_t>(sizeof(payload))));
    tickPair(pair, 0);

    DisplayConfig config = makeDisplayConfig();
    TestEngine engine(config);
    engine.init();

    DispatchScene scene;
    engine.setScene(&scene);
    engine.setNetworkSession(&pair.second);
    engine.update();

    TEST_ASSERT_EQUAL_UINT8(1, scene.networkEventCount);
    assertEventType(NetEventType::Message, scene.lastEvent.type);
    TEST_ASSERT_EQUAL_UINT8(0, scene.lastEvent.peerId);
    TEST_ASSERT_EQUAL_UINT8(11, scene.lastEvent.nodeId);
    TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(sizeof(payload)),
                             scene.lastEvent.length);
    TEST_ASSERT_EQUAL_MEMORY(payload, scene.lastEvent.payload,
                             sizeof(payload));
    TEST_ASSERT_EQUAL_UINT8(1, scene.updateCount);
}

// =============================================================================
// 2. Dispatch runs before Scene::update(): the scene reacts in the same frame.
// =============================================================================

void test_net_engine_dispatch_runs_before_scene_update(void) {
    SessionPair pair(1, 2);
    initPair(pair, 11, 1, 22, 2);
    discoverPair(pair, 0);
    drainEvents(pair.first);
    drainEvents(pair.second);

    const uint8_t payload[2] = {0x42, 0x43};
    assertNetResult(NetResult::Ok,
                    pair.first.send(0, NetDelivery::Unreliable, payload,
                                    static_cast<uint16_t>(sizeof(payload))));
    tickPair(pair, 0);

    DisplayConfig config = makeDisplayConfig();
    TestEngine engine(config);
    engine.init();

    DispatchScene scene;
    engine.setScene(&scene);
    engine.setNetworkSession(&pair.second);
    engine.update();

    TEST_ASSERT_EQUAL_UINT8(2, scene.callCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DispatchScene::Call::NetworkEvent),
                            static_cast<uint8_t>(scene.callAt(0)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DispatchScene::Call::Update),
                            static_cast<uint8_t>(scene.callAt(1)));
    TEST_ASSERT_EQUAL_UINT8(1, scene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(1, scene.updateCount);
}

// =============================================================================
// 3. Several queued events are all drained by a single Engine::update().
// =============================================================================

void test_net_engine_dispatch_drains_every_queued_event(void) {
    SessionPair pair(1, 2);
    initPair(pair, 11, 1, 22, 2);
    discoverPair(pair, 0);
    drainEvents(pair.first);
    drainEvents(pair.second);

    const uint8_t payloads[3][3] = {
        {0x01, 0x02, 0x03},
        {0x11, 0x12, 0x13},
        {0x21, 0x22, 0x23},
    };
    for (uint8_t i = 0; i < 3; ++i) {
        assertNetResult(NetResult::Ok,
                        pair.first.send(0, NetDelivery::Unreliable, payloads[i],
                                        static_cast<uint16_t>(sizeof(payloads[i]))));
    }
    // One tick per direction is enough: every unreliable send queued its frame
    // in the peer transport, and one receive tick drains them all.
    tickPair(pair, 0);

    DisplayConfig config = makeDisplayConfig();
    TestEngine engine(config);
    engine.init();

    DispatchScene scene;
    engine.setScene(&scene);
    engine.setNetworkSession(&pair.second);
    engine.update();

    TEST_ASSERT_EQUAL_UINT8(3, scene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(1, scene.updateCount);
    for (uint8_t i = 0; i < 3; ++i) {
        assertEventType(NetEventType::Message, scene.events[i].type);
        TEST_ASSERT_EQUAL_UINT8(11, scene.events[i].nodeId);
        TEST_ASSERT_EQUAL_UINT16(3, scene.events[i].length);
        TEST_ASSERT_EQUAL_MEMORY(payloads[i], scene.events[i].payload, 3);
    }
    // All three hooks ran first, then the single scene update.
    TEST_ASSERT_EQUAL_UINT8(4, scene.callCount);
    for (uint8_t i = 0; i < 3; ++i) {
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DispatchScene::Call::NetworkEvent),
                                static_cast<uint8_t>(scene.callAt(i)));
    }
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DispatchScene::Call::Update),
                            static_cast<uint8_t>(scene.callAt(3)));
}

// =============================================================================
// 4. No session (never set or explicitly null) means no pump and no hook.
// =============================================================================

void test_net_engine_dispatch_null_session_is_inert(void) {
    SessionPair pair(1, 2);
    initPair(pair, 11, 1, 22, 2);
    discoverPair(pair, 0);
    drainEvents(pair.first);
    drainEvents(pair.second);

    const uint8_t payload[1] = {0x7A};
    assertNetResult(NetResult::Ok,
                    pair.first.send(0, NetDelivery::Unreliable, payload,
                                    static_cast<uint16_t>(sizeof(payload))));
    tickPair(pair, 0);

    DisplayConfig config = makeDisplayConfig();
    TestEngine engine(config);
    engine.init();

    DispatchScene scene;
    engine.setScene(&scene);

    // Never set: update() still drives the scene.
    engine.update();
    TEST_ASSERT_EQUAL_UINT8(0, scene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(1, scene.updateCount);

    // Explicitly null: same contract.
    engine.setNetworkSession(nullptr);
    TEST_ASSERT_NULL(engine.getNetworkSession());
    engine.update();
    TEST_ASSERT_EQUAL_UINT8(0, scene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(2, scene.updateCount);

    // The pending event was never pumped, so it is still in the session queue.
    NetEvent stillQueued{};
    TEST_ASSERT_TRUE(pair.second.pollEvent(stillQueued));
    assertEventType(NetEventType::Message, stillQueued.type);
}

// =============================================================================
// 5. getNetworkSession() returns what was set, and a replacement session is
//    the one drained.
// =============================================================================

void test_net_engine_dispatch_session_replacement(void) {
    SessionPair firstPair(1, 2);
    initPair(firstPair, 11, 1, 22, 2);
    discoverPair(firstPair, 0);
    drainEvents(firstPair.first);
    drainEvents(firstPair.second);

    SessionPair secondPair(3, 4);
    initPair(secondPair, 33, 3, 44, 4);
    discoverPair(secondPair, 0);
    drainEvents(secondPair.first);
    drainEvents(secondPair.second);

    const uint8_t firstPayload[2] = {0xA1, 0xA2};
    const uint8_t secondPayload[2] = {0xB1, 0xB2};
    assertNetResult(NetResult::Ok,
                    firstPair.first.send(0, NetDelivery::Unreliable,
                                         firstPayload,
                                         static_cast<uint16_t>(sizeof(firstPayload))));
    tickPair(firstPair, 0);
    assertNetResult(NetResult::Ok,
                    secondPair.first.send(0, NetDelivery::Unreliable,
                                          secondPayload,
                                          static_cast<uint16_t>(sizeof(secondPayload))));
    tickPair(secondPair, 0);

    DisplayConfig config = makeDisplayConfig();
    TestEngine engine(config);
    engine.init();

    DispatchScene scene;
    engine.setScene(&scene);

    TEST_ASSERT_NULL(engine.getNetworkSession());

    engine.setNetworkSession(&firstPair.second);
    TEST_ASSERT_EQUAL_PTR(&firstPair.second, engine.getNetworkSession());
    engine.update();
    TEST_ASSERT_EQUAL_UINT8(1, scene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(11, scene.events[0].nodeId);
    TEST_ASSERT_EQUAL_MEMORY(firstPayload, scene.events[0].payload,
                             sizeof(firstPayload));

    engine.setNetworkSession(&secondPair.second);
    TEST_ASSERT_EQUAL_PTR(&secondPair.second, engine.getNetworkSession());
    engine.update();
    TEST_ASSERT_EQUAL_UINT8(2, scene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(33, scene.events[1].nodeId);
    TEST_ASSERT_EQUAL_MEMORY(secondPayload, scene.events[1].payload,
                             sizeof(secondPayload));
}

// =============================================================================
// 6. Drain-time semantics: a scene swap routes pending events to the new
//    scene and never replays them on the old one.
// =============================================================================

void test_net_engine_dispatch_scene_swap_uses_current_scene(void) {
    SessionPair pair(1, 2);
    initPair(pair, 11, 1, 22, 2);
    discoverPair(pair, 0);
    drainEvents(pair.first);
    drainEvents(pair.second);

    const uint8_t payload[3] = {0x55, 0x66, 0x77};
    assertNetResult(NetResult::Ok,
                    pair.first.send(0, NetDelivery::Unreliable, payload,
                                    static_cast<uint16_t>(sizeof(payload))));
    tickPair(pair, 0);

    DisplayConfig config = makeDisplayConfig();
    TestEngine engine(config);
    engine.init();

    DispatchScene oldScene;
    DispatchScene newScene;
    engine.setScene(&oldScene);
    engine.setScene(&newScene);
    engine.setNetworkSession(&pair.second);
    engine.update();

    TEST_ASSERT_EQUAL_UINT8(0, oldScene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(0, oldScene.updateCount);
    TEST_ASSERT_EQUAL_UINT8(1, newScene.networkEventCount);
    TEST_ASSERT_EQUAL_UINT8(1, newScene.updateCount);
    assertEventType(NetEventType::Message, newScene.events[0].type);
    TEST_ASSERT_EQUAL_UINT8(11, newScene.events[0].nodeId);
    TEST_ASSERT_EQUAL_MEMORY(payload, newScene.events[0].payload,
                             sizeof(payload));
}

// =============================================================================
// 7. Peer lifecycle events (not only Message) are dispatched in queue order.
// =============================================================================

void test_net_engine_dispatch_peer_lifecycle_events(void) {
    SessionPair pair(1, 2);
    initPair(pair, 11, 1, 22, 2);

    // Leave B's discovery event queued instead of draining it, then have A
    // shut down so its BYE appends a PeerLeft behind the PeerJoined.
    discoverPair(pair, 0);
    pair.first.shutdown();
    pair.second.update(0);

    DisplayConfig config = makeDisplayConfig();
    TestEngine engine(config);
    engine.init();

    DispatchScene scene;
    engine.setScene(&scene);
    engine.setNetworkSession(&pair.second);
    engine.update();

    TEST_ASSERT_EQUAL_UINT8(2, scene.networkEventCount);
    assertEventType(NetEventType::PeerJoined, scene.events[0].type);
    TEST_ASSERT_EQUAL_UINT8(0, scene.events[0].peerId);
    TEST_ASSERT_EQUAL_UINT8(11, scene.events[0].nodeId);
    assertEventType(NetEventType::PeerLeft, scene.events[1].type);
    TEST_ASSERT_EQUAL_UINT8(0, scene.events[1].peerId);
    TEST_ASSERT_EQUAL_UINT8(11, scene.events[1].nodeId);
    TEST_ASSERT_EQUAL_UINT8(1, scene.updateCount);

    // Both hooks ran before the scene update, in queue order.
    TEST_ASSERT_EQUAL_UINT8(3, scene.callCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DispatchScene::Call::NetworkEvent),
                            static_cast<uint8_t>(scene.callAt(0)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DispatchScene::Call::NetworkEvent),
                            static_cast<uint8_t>(scene.callAt(1)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DispatchScene::Call::Update),
                            static_cast<uint8_t>(scene.callAt(2)));
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_engine_dispatch_flag_off_stub_compiles(void) {
    // With the flag off there is no session pointer, no hook and no new
    // Engine symbol: this translation unit compiling and passing without
    // referencing any of them is the zero-cost flags-off contract.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_NETWORKING=0: no network session member, no "
        "onNetworkEvent hook, no Engine networking symbol.");
}

#endif  // PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)
    RUN_TEST(test_net_engine_dispatch_message_once);
    RUN_TEST(test_net_engine_dispatch_runs_before_scene_update);
    RUN_TEST(test_net_engine_dispatch_drains_every_queued_event);
    RUN_TEST(test_net_engine_dispatch_null_session_is_inert);
    RUN_TEST(test_net_engine_dispatch_session_replacement);
    RUN_TEST(test_net_engine_dispatch_scene_swap_uses_current_scene);
    RUN_TEST(test_net_engine_dispatch_peer_lifecycle_events);
#else
    RUN_TEST(test_net_engine_dispatch_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
