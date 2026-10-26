#include <core/Engine.h>

#include "EspNowDemoScene.h"

#include <cstdio>
#include <cstring>

extern pixelroot32::core::Engine engine;

namespace esp_now_demo {

namespace {

namespace cfg = pixelroot32::platforms::config;

using pixelroot32::graphics::Color;
using pixelroot32::graphics::Renderer;
using pixelroot32::networking::NetAddress;
using pixelroot32::networking::NetCounter;
using pixelroot32::networking::NetEvent;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetSession;
using pixelroot32::networking::NetSessionConfig;
using pixelroot32::networking::NetTransportConfig;

/// Every node in this demo shares one channel. Joining an access point would
/// move the radio off it, which is why the README says "no router".
constexpr std::uint8_t kChannel = 1;

/// One panel, in the same 72x40 coordinates the Beetle exposes. The native
/// window is exactly two of these, so the panel is the shared unit on both
/// targets; the static asserts below tie it back to platformio.ini.
constexpr int kPanelWidth = 72;
constexpr int kPanelHeight = 40;

/// The built-in font at size 1 is 6x8 per glyph.
constexpr int kGlyphWidth = 6;
constexpr int kGlyphHeight = 8;

/// Display clamp from the demo spec; the accumulator itself stays int32_t.
constexpr std::int32_t kMaxDisplayedValue = 999;

static_assert(cfg::LogicalHeight == kPanelHeight,
              "platformio.ini must give the panel 40 logical rows");
#ifdef PLATFORM_NATIVE
static_assert(cfg::LogicalWidth == 2 * kPanelWidth,
              "the native window must be two 72-pixel panels wide");
#else
static_assert(cfg::LogicalWidth == kPanelWidth,
              "the device panel must be 72 logical pixels wide");
#endif

/// Index of the one configured button. Both platform headers declare it first.
constexpr std::uint8_t kPrimaryButton = 0;
/// Index of the second native button (key 2); the device has no such button.
constexpr std::uint8_t kSecondaryButton = 1;

#ifdef PLATFORM_NATIVE
/**
 * @brief Builds a deterministic loopback MAC.
 * @param tag Node tag; every byte differs between tags.
 * @return Address with no byte left at zero.
 *
 * The loopback resolves peers by address, and two attached transports with the
 * same address would shadow each other. Deriving the bytes from the tag keeps
 * the two simulated boards distinct without a random source.
 */
NetAddress makeLoopbackAddress(std::uint8_t tag) {
    NetAddress address{};
    for (std::uint8_t i = 0; i < pixelroot32::networking::kNetAddressSize; ++i) {
        address.bytes[i] = static_cast<std::uint8_t>(0x10u + tag + i);
    }
    return address;
}
#endif

}  // namespace

// =============================================================================
// DemoNode
// =============================================================================

#ifdef PLATFORM_NATIVE
DemoNode::DemoNode(pixelroot32::networking::LoopbackBus& bus,
                   const NetAddress& address, std::uint8_t nodeId,
                   std::int8_t delta, std::uint16_t bootId)
    : transport(bus, address),
      session(transport),
      nodeIdValue(nodeId),
      deltaValue(delta),
      bootIdValue(bootId) {}
#else
DemoNode::DemoNode(std::uint8_t nodeId, std::int8_t delta,
                   std::uint16_t bootId)
    : transport(),
      session(transport),
      nodeIdValue(nodeId),
      deltaValue(delta),
      bootIdValue(bootId) {}
#endif

NetResult DemoNode::start(std::uint8_t channel) {
    NetTransportConfig transportConfig;
    transportConfig.channel = channel;
    transportConfig.nodeId = nodeIdValue;
    // On device the transport brings the station up without joining an AP,
    // because an association would move the channel and drop the link.
    transportConfig.manageWifi = true;

    NetResult result = transport.init(transportConfig);
    if (result != NetResult::Ok) {
        ready = false;
        return result;
    }

    NetSessionConfig sessionConfig;
    sessionConfig.channel = channel;
    sessionConfig.nodeId = nodeIdValue;
    sessionConfig.bootId = bootIdValue;
    sessionConfig.discovery = true;

    result = session.init(sessionConfig);
    if (result != NetResult::Ok) {
        ready = false;
        return result;
    }

    result = counter.init(session);
    ready = (result == NetResult::Ok);
    return result;
}

void DemoNode::update(std::uint32_t nowMs) { counter.update(nowMs); }

void DemoNode::pump(std::uint32_t nowMs) {
    session.update(nowMs);

    // The counter never owns the queue: the caller drains it, exactly as the
    // Engine does for the primary node. One node cannot see the other's events
    // in this simulation because each session has its own transport.
    NetEvent event;
    while (session.pollEvent(event)) {
        counter.onEvent(event);
    }
}

void DemoNode::applyEvent(const NetEvent& event) { counter.onEvent(event); }

NetResult DemoNode::add() { return counter.add(deltaValue); }

NetSession& DemoNode::getSession() { return session; }

const NetSession& DemoNode::getSession() const { return session; }

NetCounter& DemoNode::getCounter() { return counter; }

const NetCounter& DemoNode::getCounter() const { return counter; }

std::uint8_t DemoNode::nodeId() const { return nodeIdValue; }

std::int8_t DemoNode::delta() const { return deltaValue; }

std::int32_t DemoNode::value() const { return counter.value(); }

std::uint32_t DemoNode::revision() const { return counter.revision(); }

std::uint8_t DemoNode::peerCount() const { return session.peerCount(); }

bool DemoNode::isResyncing() const { return counter.isResyncing(); }

bool DemoNode::isOutOfSync() const { return counter.isOutOfSync(); }

bool DemoNode::isReady() const { return ready; }

// =============================================================================
// EspNowDemoScene
// =============================================================================

#ifdef PLATFORM_NATIVE
EspNowDemoScene::EspNowDemoScene()
    : bus(),
      primary(bus, makeLoopbackAddress(1), 1, 1, 1),
      secondary(bus, makeLoopbackAddress(2), 2, -1, 2) {}
#else
EspNowDemoScene::EspNowDemoScene(std::uint8_t nodeId, std::int8_t delta)
    : primary(nodeId, delta, 0) {}
#endif

void EspNowDemoScene::init() {
    // A fresh init must repaint even when every visible counter still reads 0.
    hasKey = false;

    (void)primary.start(kChannel);

#ifdef PLATFORM_NATIVE
    (void)secondary.start(kChannel);

    // One Engine dispatches one session. Registering the primary here, rather
    // than in main(), keeps the composition in the scene on both targets: the
    // device setup() only has to register the pointer after this init() ran.
    engine.setNetworkSession(&primary.getSession());
#endif
}

void EspNowDemoScene::update(unsigned long deltaTime) {
    (void)deltaTime;

    const std::uint32_t nowMs = static_cast<std::uint32_t>(millis());

    // Counter clocks first. The Engine already drained the primary session
    // this frame, so its counter sees a clock at most one frame old; that is
    // far inside the one-second uptime tie window the resync rule uses.
    primary.update(nowMs);

#ifdef PLATFORM_NATIVE
    secondary.update(nowMs);

    // The loopback bus has no clock of its own. The engine loop owns timing, so
    // the scene hands the bus the same millisecond value and then pumps the
    // second simulated board, which has no Engine to do it.
    bus.setNowMs(nowMs);
    secondary.pump(nowMs);
#endif

    auto& input = engine.getInputManager();

    if (input.isButtonPressed(kPrimaryButton)) {
        // Busy means a snapshot is being imported; the press is dropped on
        // purpose so it cannot be counted twice by the import and the replay.
        (void)primary.add();
    }

#ifdef PLATFORM_NATIVE
    if (input.isButtonPressed(kSecondaryButton)) {
        (void)secondary.add();
    }
#endif
}

void EspNowDemoScene::draw(Renderer& renderer) {
#ifdef PLATFORM_NATIVE
    drawNode(renderer, primary, 0, "A +");
    drawNode(renderer, secondary, kPanelWidth, "B -");
    renderer.drawLine(kPanelWidth, 0, kPanelWidth, kPanelHeight - 1,
                      Color::White);
#else
    char label[12];
    std::snprintf(label, sizeof(label), "N%u %c",
                  static_cast<unsigned>(primary.nodeId()),
                  primary.delta() > 0 ? '+' : '-');
    drawNode(renderer, primary, 0, label);
#endif
}

bool EspNowDemoScene::shouldRedrawFramebuffer() const {
    const RedrawKey current = makeRedrawKey();
    if (hasKey && sameKey(current, lastKey)) {
        return false;
    }

    lastKey = current;
    hasKey = true;
    return true;
}

void EspNowDemoScene::onNetworkEvent(const NetEvent& event) {
    // The Engine dispatches only the primary session; the secondary board is
    // drained by update(). Every primary event goes to the primary counter,
    // including lifecycle events, because joins decide who sends a snapshot.
    primary.applyEvent(event);
}

NetSession* EspNowDemoScene::primarySession() { return &primary.getSession(); }

EspNowDemoScene::RedrawKey EspNowDemoScene::makeRedrawKey() const {
    RedrawKey key;

    key.primaryRevision = primary.revision();
    key.primaryPeers = primary.peerCount();
    key.primaryResyncing = primary.isResyncing();
    key.primaryOutOfSync = primary.isOutOfSync();

#ifdef PLATFORM_NATIVE
    key.secondaryRevision = secondary.revision();
    key.secondaryPeers = secondary.peerCount();
    key.secondaryResyncing = secondary.isResyncing();
    key.secondaryOutOfSync = secondary.isOutOfSync();
#endif

    return key;
}

bool EspNowDemoScene::sameKey(const RedrawKey& lhs, const RedrawKey& rhs) {
    return lhs.primaryRevision == rhs.primaryRevision &&
           lhs.secondaryRevision == rhs.secondaryRevision &&
           lhs.primaryPeers == rhs.primaryPeers &&
           lhs.secondaryPeers == rhs.secondaryPeers &&
           lhs.primaryResyncing == rhs.primaryResyncing &&
           lhs.primaryOutOfSync == rhs.primaryOutOfSync &&
           lhs.secondaryResyncing == rhs.secondaryResyncing &&
           lhs.secondaryOutOfSync == rhs.secondaryOutOfSync;
}

void EspNowDemoScene::drawNode(Renderer& renderer, const DemoNode& node,
                               int originX, const char* label) {
    // A 1-bit panel has ink and background only. U8G2_Drawer collapses
    // everything else, so the scene never asks for another colour.
    const Color ink = Color::White;

    renderer.drawText(label, originX + 1, 0, ink, 1);

    if (!node.isReady()) {
        renderer.drawText("NO LINK", originX + 1, 16, ink, 1);
        return;
    }

    // Marker: resync wins over out-of-sync, which wins over the peer count.
    char marker[8];
    if (node.isResyncing()) {
        std::snprintf(marker, sizeof(marker), "SYNC");
    } else if (node.isOutOfSync()) {
        std::snprintf(marker, sizeof(marker), "!");
    } else {
        std::snprintf(marker, sizeof(marker), "%u",
                      static_cast<unsigned>(node.peerCount()));
    }

    const int markerX = originX + kPanelWidth - 2 -
                        static_cast<int>(std::strlen(marker)) * kGlyphWidth;
    renderer.drawText(marker, markerX, 0, ink, 1);

    // The accumulator is int32_t; only the drawing is clamped, so a saturated
    // counter never displays a wrapped sign.
    std::int32_t displayValue = node.value();
    if (displayValue > kMaxDisplayedValue) {
        displayValue = kMaxDisplayedValue;
    } else if (displayValue < -kMaxDisplayedValue) {
        displayValue = -kMaxDisplayedValue;
    }

    char valueText[8];
    std::snprintf(valueText, sizeof(valueText), "%ld",
                  static_cast<long>(displayValue));
    renderer.drawText(valueText, originX + 2, 12, ink, 2);

    char footer[12];
    std::snprintf(footer, sizeof(footer), "PEER %u",
                  static_cast<unsigned>(node.peerCount()));
    renderer.drawText(footer, originX + 1, kPanelHeight - kGlyphHeight, ink, 1);
}

}  // namespace esp_now_demo
