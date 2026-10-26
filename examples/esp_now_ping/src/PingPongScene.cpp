/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include <core/Engine.h>

#include "PingPongScene.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

extern pixelroot32::core::Engine engine;

namespace esp_now_ping {

namespace {

namespace cfg = pixelroot32::platforms::config;

using pixelroot32::graphics::Color;
using pixelroot32::graphics::Renderer;
using pixelroot32::networking::NetAddress;
using pixelroot32::networking::NetDelivery;
using pixelroot32::networking::NetEvent;
using pixelroot32::networking::NetEventType;
using pixelroot32::networking::NetPeerInfo;
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

/// Rows for the three text lines. The last one ends at 38, inside the panel.
constexpr int kRttY = 12;
constexpr int kFooterY = 31;

/// Counters and the failure number are clamped so a long soak cannot widen a
/// line past the 72-pixel panel. The counters themselves stay uint32_t.
constexpr std::uint32_t kMaxDisplayedCount = 999;

/// An RTT above 9.999 s would overflow a 10-character line; clamp it too.
constexpr std::uint32_t kMaxDisplayedRtt = 9999;

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

/**
 * @brief Clamps a counter to the digits that fit above the RTT line.
 * @param value Counter as accumulated.
 * @return value, or kMaxDisplayedCount when it is larger.
 */
std::uint32_t clampCount(std::uint32_t value) {
    return value > kMaxDisplayedCount ? kMaxDisplayedCount : value;
}

/**
 * @brief Clamps a measured RTT to the widest line that fits the panel.
 * @param value Measured interval in milliseconds.
 * @return value, or kMaxDisplayedRtt when it is larger.
 */
std::uint32_t clampRtt(std::uint32_t value) {
    return value > kMaxDisplayedRtt ? kMaxDisplayedRtt : value;
}

/**
 * @brief Measures a string in panel pixels.
 * @param text Null-terminated text.
 * @return Width at font size 1, in pixels.
 *
 * Used to right-align the RX counter: the renderer exposes no text
 * measurement, and at size 1 every glyph advances exactly kGlyphWidth pixels.
 */
int textWidth(const char* text) {
    return static_cast<int>(std::strlen(text)) * kGlyphWidth;
}

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
                   std::uint16_t bootId)
    : transport(bus, address),
      session(transport),
      nodeIdValue(nodeId),
      bootIdValue(bootId) {}
#else
DemoNode::DemoNode(std::uint8_t nodeId, std::uint16_t bootId)
    : transport(),
      session(transport),
      nodeIdValue(nodeId),
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
        return result;
    }

    NetSessionConfig sessionConfig;
    sessionConfig.channel = channel;
    sessionConfig.nodeId = nodeIdValue;
    sessionConfig.bootId = bootIdValue;
    sessionConfig.discovery = true;

    return session.init(sessionConfig);
}

void DemoNode::pump(std::uint32_t nowMs) {
    session.update(nowMs);

    // The session never owns the queue: the caller drains it, exactly as the
    // Engine does for the primary node.
    NetEvent event;
    while (session.pollEvent(event)) {
        onEvent(event);
    }
}

void DemoNode::applyEvent(const NetEvent& event) { onEvent(event); }

void DemoNode::sendPing(std::uint32_t nowMs) {
    int8_t peerId = -1;
    NetPeerInfo info;
    for (std::uint8_t id = 0; id < NetSession::kMaxPeers; ++id) {
        if (session.peerInfo(id, info) && info.online) {
            peerId = static_cast<int8_t>(id);
            break;
        }
    }

    if (peerId < 0) {
        // No discovery yet: pressing does nothing rather than queueing an
        // operation against a peer that does not exist.
        return;
    }

    // 0 is not a usable application id, so wrap skips it. The sequence is
    // uint16_t and the peer sees it inside the 7-byte payload, not the wire
    // sequence the session assigns for reliable delivery.
    ++nextSequence;
    if (nextSequence == 0) {
        ++nextSequence;
    }

    PingPongMessage message;
    message.sequence = nextSequence;
    message.stampMs = nowMs;

    // An exact-sized buffer makes a capacity mistake a compile-time constant
    // rather than a silent truncation.
    std::uint8_t payload[kPingPongPayloadSize] = {};
    const std::uint16_t length = encodePing(payload, sizeof(payload), message);
    if (length == 0) {
        ++failureCount;
        return;
    }

    const NetResult result = session.send(
        static_cast<std::uint8_t>(peerId), NetDelivery::Reliable, payload,
        length);
    if (result == NetResult::Ok) {
        ++pingsSentCount;
    } else {
        ++failureCount;
    }
}

void DemoNode::onEvent(const NetEvent& event) {
    switch (event.type) {
        case NetEventType::PeerJoined:
        case NetEventType::PeerLeft:
            // No protocol action: the UI reads the live peer count. A reply is
            // never addressed to a cached slot from a previous join.
            break;

        case NetEventType::Message: {
            if (event.length == 0) {
                break;
            }

            PingPongMessage message;

            if (event.payload[0] == kPingTag) {
                if (!decodePing(event.payload, event.length, message)) {
                    break;
                }

                // The reply is automatic: the receiving board does not need a
                // button press. It echoes the sequence and the stamp so the
                // sender can close its measurement.
                std::uint8_t reply[kPingPongPayloadSize] = {};
                const std::uint16_t length =
                    encodePong(reply, sizeof(reply), message);
                if (length == 0) {
                    ++failureCount;
                    break;
                }

                const NetResult result = session.send(
                    event.peerId, NetDelivery::Reliable, reply, length);
                if (result == NetResult::Ok) {
                    ++pongsSentCount;
                } else {
                    ++failureCount;
                }
            } else if (event.payload[0] == kPongTag) {
                if (!decodePong(event.payload, event.length, message)) {
                    break;
                }

                // Both stamps come from this sender's millisecond clock: the
                // value in the pong was echoed from the ping we sent. That is
                // what makes the subtraction an interval rather than a
                // cross-board clock comparison.
                lastRttValue = elapsedMs(
                    static_cast<std::uint32_t>(millis()), message.stampMs);
                hasRttValue = true;
                ++pongsReceivedCount;
            }
            break;
        }

        case NetEventType::SendFailed:
            // A reliable message exhausted its retries. The demo surfaces it as
            // a count instead of retrying with a new message.
            ++failureCount;
            break;
    }
}

NetSession& DemoNode::getSession() { return session; }

const NetSession& DemoNode::getSession() const { return session; }

std::uint8_t DemoNode::nodeId() const { return nodeIdValue; }

std::uint32_t DemoNode::pingsSent() const { return pingsSentCount; }

std::uint32_t DemoNode::pongsSent() const { return pongsSentCount; }

std::uint32_t DemoNode::pongsReceived() const { return pongsReceivedCount; }

std::uint32_t DemoNode::lastRttMs() const { return lastRttValue; }

bool DemoNode::hasRtt() const { return hasRttValue; }

std::uint32_t DemoNode::failures() const { return failureCount; }

std::uint8_t DemoNode::peerCount() const { return session.peerCount(); }

// =============================================================================
// PingPongScene
// =============================================================================

#ifdef PLATFORM_NATIVE
PingPongScene::PingPongScene()
    : bus(),
      primary(bus, makeLoopbackAddress(1), 1, 1),
      secondary(bus, makeLoopbackAddress(2), 2, 2) {}
#else
PingPongScene::PingPongScene(std::uint8_t nodeId) : primary(nodeId, 0) {}
#endif

void PingPongScene::init() {
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

void PingPongScene::update(unsigned long deltaTime) {
    (void)deltaTime;

    const std::uint32_t nowMs = static_cast<std::uint32_t>(millis());

#ifdef PLATFORM_NATIVE
    // The loopback bus has no clock of its own. The engine loop owns timing, so
    // the scene hands the bus the same millisecond value and then pumps the
    // second simulated board, which has no Engine to do it.
    bus.setNowMs(nowMs);
    secondary.pump(nowMs);
#endif

    auto& input = engine.getInputManager();

    if (input.isButtonPressed(kPrimaryButton)) {
        primary.sendPing(nowMs);
    }

#ifdef PLATFORM_NATIVE
    if (input.isButtonPressed(kSecondaryButton)) {
        secondary.sendPing(nowMs);
    }
#endif
}

void PingPongScene::draw(Renderer& renderer) {
#ifdef PLATFORM_NATIVE
    drawNode(renderer, primary, 0);
    drawNode(renderer, secondary, kPanelWidth);
    renderer.drawLine(kPanelWidth, 0, kPanelWidth, kPanelHeight - 1,
                      Color::White);
#else
    drawNode(renderer, primary, 0);
#endif
}

bool PingPongScene::shouldRedrawFramebuffer() const {
    const RedrawKey current = makeRedrawKey();
    if (hasKey && sameNodeKey(current.primary, lastKey.primary) &&
        sameNodeKey(current.secondary, lastKey.secondary)) {
        return false;
    }

    lastKey = current;
    hasKey = true;
    return true;
}

void PingPongScene::onNetworkEvent(const NetEvent& event) {
    // The Engine dispatches only the primary session; the secondary board is
    // drained by update().
    primary.applyEvent(event);
}

NetSession* PingPongScene::primarySession() { return &primary.getSession(); }

PingPongScene::NodeKey PingPongScene::makeNodeKey(const DemoNode& node) {
    NodeKey key;
    key.pingsSent = node.pingsSent();
    key.pongsReceived = node.pongsReceived();
    key.lastRttMs = node.lastRttMs();
    key.failures = node.failures();
    key.peers = node.peerCount();
    key.hasRtt = node.hasRtt();
    return key;
}

bool PingPongScene::sameNodeKey(const NodeKey& lhs, const NodeKey& rhs) {
    return lhs.pingsSent == rhs.pingsSent &&
           lhs.pongsReceived == rhs.pongsReceived &&
           lhs.lastRttMs == rhs.lastRttMs && lhs.failures == rhs.failures &&
           lhs.peers == rhs.peers && lhs.hasRtt == rhs.hasRtt;
}

PingPongScene::RedrawKey PingPongScene::makeRedrawKey() const {
    RedrawKey key;
    key.primary = makeNodeKey(primary);
#ifdef PLATFORM_NATIVE
    key.secondary = makeNodeKey(secondary);
#endif
    return key;
}

void PingPongScene::drawNode(Renderer& renderer, const DemoNode& node,
                             int originX) {
    // A 1-bit panel has ink and background only. U8G2_Drawer collapses
    // everything else, so the scene never asks for another colour.
    const Color ink = Color::White;

    char text[16];

    // Top line: the outgoing ping count on the left, the received pong count on
    // the right. Both are clamped to three digits, which is exactly the widest
    // pair that still fits 72 pixels (6 glyphs each side at 6 px).
    std::snprintf(text, sizeof(text), "TX %u",
                  static_cast<unsigned>(clampCount(node.pingsSent())));
    renderer.drawText(text, originX, 0, ink, 1);

    std::snprintf(text, sizeof(text), "RX %u",
                  static_cast<unsigned>(clampCount(node.pongsReceived())));
    renderer.drawText(text, originX + kPanelWidth - textWidth(text), 0, ink, 1);

    // Middle line: the application round-trip interval, or a placeholder until
    // the first pong arrives.
    if (node.hasRtt()) {
        std::snprintf(text, sizeof(text), "RTT %ums",
                      static_cast<unsigned>(clampRtt(node.lastRttMs())));
    } else {
        std::snprintf(text, sizeof(text), "RTT --");
    }
    renderer.drawText(text, originX + 1, kRttY, ink, 1);

    // Bottom line: link state, or the failure count alone when one exists.
    // "PEER 1 ERR 2" is 13 glyphs (78 px) and cannot fit, so a failure takes
    // the whole line instead of being appended to the peer text.
    if (node.failures() > 0) {
        std::snprintf(text, sizeof(text), "ERR %u",
                      static_cast<unsigned>(clampCount(node.failures())));
    } else if (node.peerCount() > 0) {
        std::snprintf(text, sizeof(text), "PEER %u",
                      static_cast<unsigned>(node.peerCount()));
    } else {
        std::snprintf(text, sizeof(text), "OFFLINE");
    }
    renderer.drawText(text, originX + 1, kFooterY, ink, 1);
}

}  // namespace esp_now_ping
