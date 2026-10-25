/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include <core/Engine.h>

#include "DiceDuelScene.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

extern pixelroot32::core::Engine engine;

namespace esp_now_dice {

namespace {

namespace cfg = pixelroot32::platforms::config;

using pixelroot32::graphics::Color;
using pixelroot32::graphics::Renderer;
using pixelroot32::networking::NetAddress;
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

/// Die box geometry. A 20x20 box with a 5x5 pip grid at scale 3 leaves a 2-pixel
/// margin between the pips and the border: without that margin the corner pips
/// merge with the border and faces 2/4/6 read as an empty box.
constexpr int kDieBox = 20;
constexpr int kDieScale = 3;
constexpr int kDiePipMargin = 2;
constexpr int kDieY = 9;
constexpr int kDieOriginA = 6;
constexpr int kDieOriginB = 46;

/// Bottom status line. The dice end at row 28, leaving rows 31..38 for the
/// status and the score.
constexpr int kStatusY = 31;

/// Right margin of the score in the status row.
constexpr int kScoreRightMargin = 2;

/// Display clamp for the score. The shared score only reaches +/-3 before the
/// match resets, but a peer is free to publish anything into the counter: the
/// clamp keeps a hostile or corrupt value from widening the header past the
/// round label, and keeps the negation below inside int32_t.
constexpr std::int32_t kMaxDisplayedScore = 99;

/// Pip layout of each face, read as a 5x5 bit grid: bit (row * 5 + col) fills
/// that cell. Keeping the six faces side by side in one table makes a typo
/// visible; reading them as a switch would hide the geometry in control flow.
constexpr std::uint32_t kPipMasks[kDiceFaces] = {
    1u << 12,                                             // 1: centre
    (1u << 0) | (1u << 24),                               // 2: diagonal
    (1u << 0) | (1u << 12) | (1u << 24),                  // 3: diagonal
    (1u << 0) | (1u << 4) | (1u << 20) | (1u << 24),      // 4: corners
    (1u << 0) | (1u << 4) | (1u << 12) | (1u << 20) |
        (1u << 24),                                       // 5: corners + centre
    (1u << 0) | (1u << 4) | (1u << 10) | (1u << 14) |
        (1u << 20) | (1u << 24),                          // 6: two columns
};

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

#ifdef PLATFORM_NATIVE
/// Index of the second native button (key 2); the device has no such button.
constexpr std::uint8_t kSecondaryButton = 1;

/// Native seed bases. Fixed so the same key sequence always produces the same
/// dice, which makes a bug reproducible. They differ between the two simulated
/// boards so the two commitments in a round are never derived from one shared
/// input by accident. The device takes its base from esp_random() instead.
constexpr std::uint32_t kSeedBaseA = 0xA1B2C3D4u;
constexpr std::uint32_t kSeedBaseB = 0x10203040u;

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
#endif  // PLATFORM_NATIVE

}  // namespace

// =============================================================================
// DemoNode
// =============================================================================

#ifdef PLATFORM_NATIVE
DemoNode::DemoNode(pixelroot32::networking::LoopbackBus& bus,
                   const NetAddress& address, std::uint8_t nodeId,
                   std::uint32_t seedBase)
    : transport(bus, address),
      session(transport),
      counter(),
      duel(session, counter, seedBase),
      nodeIdValue(nodeId) {}
#else
DemoNode::DemoNode(std::uint8_t nodeId, std::uint32_t seedBase)
    : transport(),
      session(transport),
      counter(),
      duel(session, counter, seedBase),
      nodeIdValue(nodeId) {}
#endif

NetResult DemoNode::start(std::uint8_t channel) {
    readyValue = false;

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
    // 0 asks the session for a per-boot generation, which is what makes a
    // rebooted board look new to its peer instead of like a stale duplicate.
    sessionConfig.bootId = 0;
    sessionConfig.discovery = true;

    result = session.init(sessionConfig);
    if (result != NetResult::Ok) {
        return result;
    }

    // The duel borrowed this counter at construction, so the counter must be
    // bound to a started session before the first press can score a round.
    const NetResult counterResult = counter.init(session);
    readyValue = (counterResult == NetResult::Ok);
    return counterResult;
}

void DemoNode::update(std::uint32_t nowMs) {
    counter.update(nowMs);
    duel.update(nowMs);
}

void DemoNode::pump(std::uint32_t nowMs) {
    session.update(nowMs);

    // Neither consumer owns the queue: the caller drains it, exactly as the
    // Engine does for the primary node. Both see every event, because the duel
    // reads commit/reveal/reset payloads and the counter reads peer lifecycle
    // and its own operations from the same stream.
    NetEvent event;
    while (session.pollEvent(event)) {
        counter.onEvent(event);
        duel.onEvent(event);
    }
}

void DemoNode::applyEvent(const NetEvent& event) {
    counter.onEvent(event);
    duel.onEvent(event);
}

void DemoNode::press() { duel.press(); }

NetSession& DemoNode::getSession() { return session; }

DuelPhase DemoNode::phase() const { return duel.phase(); }

std::uint16_t DemoNode::round() const { return duel.round(); }

std::uint8_t DemoNode::localDie() const { return duel.localDie(); }

std::uint8_t DemoNode::peerDie() const { return duel.peerDie(); }

std::int32_t DemoNode::netScore() const { return duel.netScore(); }

bool DemoNode::localWonRound() const { return duel.localWonRound(); }

bool DemoNode::tieRound() const { return duel.tieRound(); }

bool DemoNode::tampered() const { return duel.tampered(); }

bool DemoNode::timedOut() const { return duel.timedOut(); }

bool DemoNode::matchOver() const { return duel.matchOver(); }

bool DemoNode::isNodeA() const {
    // Derive the perspective from the live table rather than from the last
    // round: after a reboot the score can be restored before the first round,
    // and the display must already show the right side of it.
    for (std::uint8_t i = 0; i < NetSession::kMaxPeers; ++i) {
        pixelroot32::networking::NetPeerInfo info{};
        if (session.peerInfo(i, info) && info.online) {
            return session.localNodeId() < info.nodeId;
        }
    }
    return true;
}

std::uint32_t DemoNode::revision() const { return counter.revision(); }

std::uint8_t DemoNode::peerCount() const { return session.peerCount(); }

bool DemoNode::isResyncing() const { return counter.isResyncing(); }

bool DemoNode::isOutOfSync() const { return counter.isOutOfSync(); }

bool DemoNode::isReady() const { return readyValue; }

// =============================================================================
// DiceDuelScene
// =============================================================================

#ifdef PLATFORM_NATIVE
DiceDuelScene::DiceDuelScene()
    : bus(),
      primary(bus, makeLoopbackAddress(1), 1, kSeedBaseA),
      secondary(bus, makeLoopbackAddress(2), 2, kSeedBaseB) {}
#else
DiceDuelScene::DiceDuelScene(std::uint8_t nodeId, std::uint32_t seedBase)
    : primary(nodeId, seedBase) {}
#endif

void DiceDuelScene::init() {
    // A fresh init must repaint even when every visible field still reads its
    // initial value.
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

void DiceDuelScene::update(unsigned long deltaTime) {
    (void)deltaTime;

    const std::uint32_t nowMs = static_cast<std::uint32_t>(millis());

    // Clocks first: press() stamps a new round with the timestamp of the last
    // update(), so the timeout baseline is this frame and not the one before.
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

    // A press is the whole control scheme: it starts a round, advances past a
    // result, or resets the match. The duel decides which, from its phase.
    if (input.isButtonPressed(kPrimaryButton)) {
        primary.press();
    }

#ifdef PLATFORM_NATIVE
    if (input.isButtonPressed(kSecondaryButton)) {
        secondary.press();
    }
#endif
}

void DiceDuelScene::draw(Renderer& renderer) {
#ifdef PLATFORM_NATIVE
    drawNode(renderer, primary, 0);
    drawNode(renderer, secondary, kPanelWidth);
    renderer.drawLine(kPanelWidth, 0, kPanelWidth, kPanelHeight - 1,
                      Color::White);
#else
    drawNode(renderer, primary, 0);
#endif
}

bool DiceDuelScene::shouldRedrawFramebuffer() const {
    const RedrawKey current = makeRedrawKey();
    if (hasKey && sameNodeKey(current.primary, lastKey.primary) &&
        sameNodeKey(current.secondary, lastKey.secondary)) {
        return false;
    }

    lastKey = current;
    hasKey = true;
    return true;
}

void DiceDuelScene::onNetworkEvent(const NetEvent& event) {
    // The Engine dispatches only the primary session; the secondary board is
    // drained by update(). Every primary event reaches both the counter and the
    // duel, including lifecycle events, because joins decide who sends a score
    // snapshot and a departure abandons an in-flight round.
    primary.applyEvent(event);
}

NetSession* DiceDuelScene::primarySession() { return &primary.getSession(); }

DiceDuelScene::NodeKey DiceDuelScene::makeNodeKey(const DemoNode& node) {
    NodeKey key;

    key.revision = node.revision();
    key.netScore = node.netScore();
    key.round = node.round();
    key.localDie = node.localDie();
    key.peerDie = node.peerDie();
    key.peers = node.peerCount();
    key.phase = node.phase();
    key.localWon = node.localWonRound();
    key.tie = node.tieRound();
    key.tampered = node.tampered();
    key.timedOut = node.timedOut();
    key.matchOver = node.matchOver();
    key.resyncing = node.isResyncing();
    key.outOfSync = node.isOutOfSync();
    key.ready = node.isReady();

    return key;
}

bool DiceDuelScene::sameNodeKey(const NodeKey& lhs, const NodeKey& rhs) {
    return lhs.revision == rhs.revision &&
           lhs.netScore == rhs.netScore &&
           lhs.round == rhs.round &&
           lhs.localDie == rhs.localDie &&
           lhs.peerDie == rhs.peerDie &&
           lhs.peers == rhs.peers &&
           lhs.phase == rhs.phase &&
           lhs.localWon == rhs.localWon &&
           lhs.tie == rhs.tie &&
           lhs.tampered == rhs.tampered &&
           lhs.timedOut == rhs.timedOut &&
           lhs.matchOver == rhs.matchOver &&
           lhs.resyncing == rhs.resyncing &&
           lhs.outOfSync == rhs.outOfSync &&
           lhs.ready == rhs.ready;
}

DiceDuelScene::RedrawKey DiceDuelScene::makeRedrawKey() const {
    RedrawKey key;
    key.primary = makeNodeKey(primary);

#ifdef PLATFORM_NATIVE
    key.secondary = makeNodeKey(secondary);
#endif

    return key;
}

void DiceDuelScene::drawNode(Renderer& renderer, const DemoNode& node,
                             int originX) {
    // A 1-bit panel has ink and background only. U8G2_Drawer collapses
    // everything else, so the scene never asks for another colour.
    const Color ink = Color::White;

    // The shared score is A - B, which is only meaningful from a fixed
    // viewpoint. Each board flips it into "am I ahead", so both players read
    // the same words for opposite signs. The clamp is display-only: the counter
    // keeps the real value, and it keeps the negation inside int32_t.
    std::int32_t shared = node.netScore();
    if (shared > kMaxDisplayedScore) {
        shared = kMaxDisplayedScore;
    } else if (shared < -kMaxDisplayedScore) {
        shared = -kMaxDisplayedScore;
    }
    const std::int32_t advantage = node.isNodeA() ? shared : -shared;

    // Explicit ownership: each die sits under the player it belongs to. The
    // label is centred over the 20-pixel box (an 18-pixel word leaves 1px a
    // side) because a bare box gave no clue which face was whose.
    constexpr int kLabelOffset =
        (kDieBox - 3 * kGlyphWidth) / 2;
    renderer.drawText("YOU", originX + kDieOriginA + kLabelOffset, 0, ink, 1);
    renderer.drawText("RIV", originX + kDieOriginB + kLabelOffset, 0, ink, 1);

    drawDie(renderer, originX + kDieOriginA, kDieY, node.localDie(), kDieScale);
    drawDie(renderer, originX + kDieOriginB, kDieY, node.peerDie(), kDieScale);

    char status[kStatusCapacity];
    statusText(node, status);
    renderer.drawText(status, originX + 1, kStatusY, ink, 1);

    // The score stays visible in every phase, including Idle, so dismissing a
    // result does not hide the match state. It is written from this board's
    // perspective: +N means you lead, -N means the peer does.
    char scoreText[8];
    if (advantage > 0) {
        std::snprintf(scoreText, sizeof(scoreText), "+%ld",
                      static_cast<long>(advantage));
    } else if (advantage < 0) {
        std::snprintf(scoreText, sizeof(scoreText), "-%ld",
                      static_cast<long>(-advantage));
    } else {
        std::snprintf(scoreText, sizeof(scoreText), "0");
    }
    const int scoreX = originX + kPanelWidth - kScoreRightMargin -
                       static_cast<int>(std::strlen(scoreText)) * kGlyphWidth;
    renderer.drawText(scoreText, scoreX, kStatusY, ink, 1);
}

void DiceDuelScene::drawDie(Renderer& renderer, int x, int y,
                            std::uint8_t value, int scale) {
    const Color ink = Color::White;

    // The border is drawn first in every case, including the empty box, so a
    // board that has not resolved a round yet still shows where its die will
    // appear instead of a blank panel that looks like a hung program.
    renderer.drawRectangle(x, y, kDieBox, kDieBox, ink);

    if (value == 0 || value > kDiceFaces) {
        return;
    }

    // The pip grid is inset from the border. Drawing it flush made corner pips
    // merge with the border, so 2, 4 and 6 looked like an empty box on the
    // 1-bit panel. Two pixels of margin keep every pip distinct at 20x20.
    const int pipX = x + kDiePipMargin;
    const int pipY = y + kDiePipMargin;
    const std::uint32_t mask = kPipMasks[value - 1];
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            if ((mask & (1u << (row * 5 + col))) == 0) {
                continue;
            }
            renderer.drawFilledRectangle(pipX + col * scale, pipY + row * scale,
                                         scale, scale, ink);
        }
    }
}

void DiceDuelScene::statusText(const DemoNode& node, char* out) {
    // A failed verification outranks everything: it is the one state the player
    // must act on. A timeout and a resync both explain why nothing is happening,
    // so they outrank the phase word they are technically still in.
    // A link that never came up explains every other symptom; it outranks the
    // rest because no protocol state exists yet.
    if (!node.isReady()) {
        std::snprintf(out, kStatusCapacity, "NO LINK");
        return;
    }
    if (node.tampered()) {
        std::snprintf(out, kStatusCapacity, "CHEAT");
        return;
    }
    if (node.timedOut()) {
        std::snprintf(out, kStatusCapacity, "TIMEOUT");
        return;
    }
    if (node.isResyncing()) {
        std::snprintf(out, kStatusCapacity, "SYNC");
        return;
    }
    if (node.isOutOfSync()) {
        std::snprintf(out, kStatusCapacity, "OUT");
        return;
    }

    switch (node.phase()) {
        case DuelPhase::Idle:
            // A lone board cannot start a round it can never finish, so Idle
            // with no peer asks for nothing and says why.
            if (node.peerCount() > 0) {
                std::snprintf(out, kStatusCapacity, "PRESS");
            } else {
                std::snprintf(out, kStatusCapacity, "OFFLINE");
            }
            break;
        case DuelPhase::Committing:
        case DuelPhase::Revealing:
            std::snprintf(out, kStatusCapacity, "WAIT");
            break;
        case DuelPhase::Result:
            if (node.tieRound()) {
                std::snprintf(out, kStatusCapacity, "TIE");
            } else if (node.localWonRound()) {
                std::snprintf(out, kStatusCapacity, "WIN");
            } else {
                std::snprintf(out, kStatusCapacity, "LOSE");
            }
            break;
        case DuelPhase::MatchOver:
            std::snprintf(out, kStatusCapacity, "MATCH");
            break;
    }
}

}  // namespace esp_now_dice
