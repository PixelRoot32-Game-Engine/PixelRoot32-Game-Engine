/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <platforms/EngineConfig.h>

#include <cstdint>

// The demo has nothing to show without the module: fail the build instead of
// drawing a static board with no link behind it.
#if !PIXELROOT32_ENABLE_NETWORKING
#error "examples/esp_now_dice requires PIXELROOT32_ENABLE_NETWORKING=1"
#endif

#include <networking/NetCounter.h>
#include <networking/NetSession.h>
#include <networking/NetTransport.h>
#include <networking/NetTypes.h>

#ifdef PLATFORM_NATIVE
#include <drivers/native/LoopbackTransport.h>
#else
#include <drivers/esp32/ESP32EspNowTransport.h>
#endif

#include "DiceDuel.h"

namespace esp_now_dice {

#ifdef PLATFORM_NATIVE
/** @brief Simulated link used by the native environment. */
using DuelTransport = pixelroot32::networking::LoopbackTransport;
#else
/** @brief Radio link used by the two ESP32-C3 environments. */
using DuelTransport = pixelroot32::networking::ESP32EspNowTransport;
#endif

/**
 * @struct DemoNode
 * @brief One board's complete duel state: link, session, score and duel.
 *
 * The transport, session, counter and duel are direct members in that
 * construction order, so a node reserves its whole footprint at construction
 * and never allocates. The composition is the same on both targets; only the
 * transport type differs, because a loopback transport is born attached to a
 * bus while the radio one comes up empty.
 *
 * The node is deliberately platform-shaped rather than polymorphic: the
 * preprocessor picks the transport, and the members above it are written once.
 */
struct DemoNode {
#ifdef PLATFORM_NATIVE
    /**
     * @brief Creates a native node on a loopback bus.
     * @param bus Bus that carries this node's frames; it must outlive the node.
     * @param address Local address the transport advertises.
     * @param nodeId Application label, 1 or 2 in this demo.
     * @param seedBase Fixed per-node seed base; different for A and B so the
     *        two simulated boards never derive the same commitment.
     */
    DemoNode(pixelroot32::networking::LoopbackBus& bus,
             const pixelroot32::networking::NetAddress& address,
             std::uint8_t nodeId, std::uint32_t seedBase);
#else
    /**
     * @brief Creates a device node with an idle ESP-NOW transport.
     * @param nodeId Application label from ESP_NOW_NODE_ID.
     * @param seedBase Per-boot seed base from esp_random().
     */
    DemoNode(std::uint8_t nodeId, std::uint32_t seedBase);
#endif

    /**
     * @brief Brings the transport up, starts the session and binds the counter.
     * @param channel Wi-Fi channel shared by every node, 1..13.
     * @return The first failing result, or NetResult::Ok when the node is ready.
     *
     * The order matters: the session refuses to start on a transport that is not
     * ready, and the counter needs the session's local identity. Idempotent:
     * calling it again re-applies the same configuration and resets the
     * replicated score to the local zero. It never throws and never blocks.
     */
    pixelroot32::networking::NetResult start(std::uint8_t channel);

    /**
     * @brief Advances the counter and the duel clocks.
     * @param nowMs Current time in milliseconds, from millis().
     *
     * Must run once per frame before local input is read: press() stamps a new
     * round with the timestamp of the last update(), so a round started before
     * update() would measure its own timeout from a stale frame.
     */
    void update(std::uint32_t nowMs);

    /**
     * @brief Pumps the session and feeds every queued event to both consumers.
     * @param nowMs Current time in milliseconds.
     *
     * Only the secondary native node calls this: the Engine pumps and dispatches
     * the primary session itself. A device node is always pumped by the Engine.
     * The counter sees every event, because peer lifecycle events decide who
     * exports the score snapshot; the duel ignores the ones it does not use.
     */
    void pump(std::uint32_t nowMs);

    /**
     * @brief Applies one event to this node's counter and duel.
     * @param event Event drained from the session queue.
     */
    void applyEvent(const pixelroot32::networking::NetEvent& event);

    /** @brief Applies one button press according to the duel's current phase. */
    void press();

    /** @brief Returns the session this node owns. */
    pixelroot32::networking::NetSession& getSession();

    /** @brief Returns the current round phase. */
    DuelPhase phase() const;

    /** @brief Returns the round number, 0 before the first round. */
    std::uint16_t round() const;

    /** @brief Returns this board's die, 0 before a round resolves. */
    std::uint8_t localDie() const;

    /** @brief Returns the peer's die, 0 before a round resolves. */
    std::uint8_t peerDie() const;

    /** @brief Returns the shared score as A - B. */
    std::int32_t netScore() const;

    /** @brief Reports whether this board won the last resolved round. */
    bool localWonRound() const;

    /** @brief Reports whether the last resolved round was a tie. */
    bool tieRound() const;

    /** @brief Reports whether a commitment or reveal failed verification. */
    bool tampered() const;

    /** @brief Reports whether the last round was abandoned by the timeout. */
    bool timedOut() const;

    /** @brief Reports whether the shared score reached the target magnitude. */
    bool matchOver() const;

    /** @brief Reports whether this board owns the A (lower node id) role. */
    bool isNodeA() const;

    /** @brief Returns how many real score changes have been applied. */
    std::uint32_t revision() const;

    /** @brief Returns the number of online peers. */
    std::uint8_t peerCount() const;

    /** @brief Reports whether a score snapshot import is in progress. */
    bool isResyncing() const;

    /** @brief Reports whether a score resync gave up without a snapshot. */
    bool isOutOfSync() const;

    /** @brief Reports whether transport, session and counter all started. */
    bool isReady() const;

private:
    DuelTransport transport;                        ///< Link; never shared.
    pixelroot32::networking::NetSession session;    ///< Session on that link.
    pixelroot32::networking::NetCounter counter;    ///< Convergent shared score.
    DiceDuel duel;                                  ///< Round state machine.
    std::uint8_t nodeIdValue = 0;                   ///< Application label.
    bool readyValue = false;                        ///< True after a clean start.
};

/**
 * @class DiceDuelScene
 * @brief Two-board commit/reveal dice duel, with both boards simulated on native.
 *
 * The scene is the composition root: the Engine borrows a NetSession and never
 * owns a transport, so somebody has to bring the link up, start the session and
 * register the pointer. That is this class.
 *
 * Native runs one Engine and therefore one pumped session. The second simulated
 * board is driven from @ref update (session update, event drain, duel), which is
 * a simulation detail, not an engine limitation: two real Engines would each
 * pump their own session.
 *
 * The scene owns every node as a direct member and draws only Renderer
 * primitives, because the target panel is 1-bit.
 */
class DiceDuelScene : public pixelroot32::core::Scene {
public:
#ifdef PLATFORM_NATIVE
    /**
     * @brief Builds the simulated pair: node 1 is A, node 2 is B.
     *
     * Both transports share one lossless loopback bus, so this mode proves the
     * protocol, the scoring and the drawing, not the radio.
     */
    DiceDuelScene();
#else
    /**
     * @brief Builds the one hardware node this build represents.
     * @param nodeId Application label from ESP_NOW_NODE_ID.
     * @param seedBase Per-boot seed base from esp_random().
     */
    DiceDuelScene(std::uint8_t nodeId, std::uint32_t seedBase);
#endif

    /**
     * @brief Starts the node(s) and publishes the primary session.
     *
     * On native the scene registers the primary session with the Engine; on
     * device setup() registers it after this method has brought the link up.
     */
    void init() override;

    /**
     * @brief Advances both boards, pumps the simulated second board and reads
     *        the local input.
     * @param deltaTime Milliseconds since the previous frame; unused because
     *        every timer in this demo comes from millis().
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws one 72x40 panel per node, side by side on native.
     * @param renderer Renderer to draw with; only text, lines and rectangles.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Skips the frame when nothing visible changed.
     * @return true when a phase, a die, a round, the score, the peer count or a
     *         status flag differs from the last queried frame.
     *
     * A still U8G2 buffer is indistinguishable from a hung program, so the
     * scene redraws only on real state changes and leaves a static screen alone.
     */
    bool shouldRedrawFramebuffer() const override;

    /**
     * @brief Forwards one engine-drained event to the primary node.
     * @param event Event from the primary session, already popped by the Engine.
     */
    void onNetworkEvent(
        const pixelroot32::networking::NetEvent& event) override;

    /**
     * @brief Returns the session the Engine should pump.
     * @return Pointer to the primary node's session; valid for the scene's life.
     */
    pixelroot32::networking::NetSession* primarySession();

private:
    /** @brief Visible state of one node, sampled for the redraw decision. */
    struct NodeKey {
        std::uint32_t revision = 0;          ///< Counter changes.
        std::int32_t netScore = 0;           ///< Shared score.
        std::uint16_t round = 0;             ///< Round number.
        std::uint8_t localDie = 0;           ///< Local face.
        std::uint8_t peerDie = 0;            ///< Peer face.
        std::uint8_t peers = 0;              ///< Online peer count.
        DuelPhase phase = DuelPhase::Idle;   ///< Round phase.
        bool localWon = false;               ///< Result marker.
        bool tie = false;                    ///< Tie marker.
        bool tampered = false;               ///< CHEAT marker.
        bool timedOut = false;               ///< TIMEOUT marker.
        bool matchOver = false;              ///< MATCH marker.
        bool resyncing = false;              ///< SYNC marker.
        bool outOfSync = false;              ///< OUT marker.
        bool ready = false;                  ///< NO LINK marker.
    };

    /** @brief Visible state of both nodes. */
    struct RedrawKey {
        NodeKey primary{};    ///< Primary node sample.
        NodeKey secondary{};  ///< Secondary node sample (default on device).
    };

    /**
     * @brief Samples the state that affects one node's drawn pixels.
     * @param node Node to sample.
     * @return Sample of every field the panel renders.
     */
    static NodeKey makeNodeKey(const DemoNode& node);

    /**
     * @brief Compares two node samples field by field.
     * @param lhs First sample.
     * @param rhs Second sample.
     * @return true when every visible field matches.
     *
     * Field-by-field rather than memcmp: the struct carries padding bytes that
     * are not initialised, and comparing them would force spurious redraws.
     */
    static bool sameNodeKey(const NodeKey& lhs, const NodeKey& rhs);

    /** @brief Samples the state that affects the whole framebuffer. */
    RedrawKey makeRedrawKey() const;

    /**
     * @brief Draws one node's panel.
     * @param renderer Renderer to draw with.
     * @param node Node whose duel state is shown.
     * @param originX Left edge of the panel in logical coordinates.
     */
    void drawNode(pixelroot32::graphics::Renderer& renderer,
                  const DemoNode& node, int originX);

    /**
     * @brief Draws one die as a bordered 5x5 pip grid.
     * @param renderer Renderer to draw with.
     * @param x Left edge of the die box.
     * @param y Top edge of the die box.
     * @param value Face to draw, 1..kDiceFaces; 0 draws the empty box.
     * @param scale Pixels per pip cell; the box is 5 * scale wide and tall.
     */
    void drawDie(pixelroot32::graphics::Renderer& renderer, int x, int y,
                 std::uint8_t value, int scale);

    /**
     * @brief Draws the status word for the panel's bottom line.
     * @param node Node to describe.
     * @param out Destination buffer, at least @ref kStatusCapacity bytes.
     *
     * Priority is CHEAT > TIMEOUT > SYNC > phase text: a failed verification is
     * the most important thing the board can say, and a round that is aborting
     * outranks the phase it is technically still in.
     */
    static void statusText(const DemoNode& node, char* out);

    /** @brief Capacity of a status word, terminator included. */
    static constexpr int kStatusCapacity = 8;

#ifdef PLATFORM_NATIVE
    pixelroot32::networking::LoopbackBus bus;  ///< Bus shared by both nodes.
    DemoNode primary;                          ///< Panel A, lower node id.
    DemoNode secondary;                        ///< Panel B, higher node id.
#else
    DemoNode primary;                          ///< The one hardware node.
#endif

    mutable RedrawKey lastKey{};    ///< Sample from the last redraw decision.
    mutable bool hasKey = false;    ///< false forces the first draw.
};

}  // namespace esp_now_dice
