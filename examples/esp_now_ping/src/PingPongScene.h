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
// drawing zeros with no link behind them.
#if !PIXELROOT32_ENABLE_NETWORKING
#error "examples/esp_now_ping requires PIXELROOT32_ENABLE_NETWORKING=1"
#endif

#include <networking/NetSession.h>
#include <networking/NetTransport.h>
#include <networking/NetTypes.h>

#ifdef PLATFORM_NATIVE
#include <drivers/native/LoopbackTransport.h>
#else
#include <drivers/esp32/ESP32EspNowTransport.h>
#endif

#include "PingPongProtocol.h"

namespace esp_now_ping {

#ifdef PLATFORM_NATIVE
/** @brief Simulated link used by the native environment. */
using PingTransport = pixelroot32::networking::LoopbackTransport;
#else
/** @brief Radio link used by the two ESP32-C3 environments. */
using PingTransport = pixelroot32::networking::ESP32EspNowTransport;
#endif

/**
 * @struct DemoNode
 * @brief One board's complete networking state: link, session and ping counters.
 *
 * The transport and session are direct members in that construction order, so a
 * node reserves its whole footprint at construction and never allocates. A node
 * never owns the other side of the link: discovery finds the peer, and a ping
 * is only sent to a peer that is already online.
 *
 * The node is deliberately platform-shaped rather than polymorphic. On device it
 * default-constructs an @ref ESP32EspNowTransport; on native the scene passes
 * the shared bus and an address because a loopback transport is born attached.
 */
struct DemoNode {
#ifdef PLATFORM_NATIVE
    /**
     * @brief Creates a native node on a loopback bus.
     * @param bus Bus that carries this node's frames; it must outlive the node.
     * @param address Local address the transport advertises.
     * @param nodeId Application label, 1 or 2 in this demo.
     * @param bootId Fixed boot generation that makes the simulation repeatable.
     */
    DemoNode(pixelroot32::networking::LoopbackBus& bus,
             const pixelroot32::networking::NetAddress& address,
             std::uint8_t nodeId, std::uint16_t bootId);
#else
    /**
     * @brief Creates a device node with an idle ESP-NOW transport.
     * @param nodeId Application label from ESP_NOW_NODE_ID.
     * @param bootId Boot generation; 0 asks the session to generate one per boot.
     */
    DemoNode(std::uint8_t nodeId, std::uint16_t bootId);
#endif

    /**
     * @brief Brings the transport up and starts the session.
     * @param channel Wi-Fi channel shared by every node, 1..13.
     * @return The first failing result, or NetResult::Ok when the node is ready.
     *
     * The order matters: the session refuses to start on a transport that is not
     * ready. Idempotent: calling it again re-applies the same configuration and
     * clears pending events. It never throws and never blocks.
     */
    pixelroot32::networking::NetResult start(std::uint8_t channel);

    /**
     * @brief Pumps the session and feeds every queued event to @ref onEvent.
     * @param nowMs Current time in milliseconds.
     *
     * Only the secondary native node calls this: the Engine pumps and dispatches
     * the primary session itself. A device node is always pumped by the Engine.
     */
    void pump(std::uint32_t nowMs);

    /**
     * @brief Applies one event to this node.
     * @param event Event drained from the session queue.
     */
    void applyEvent(const pixelroot32::networking::NetEvent& event);

    /**
     * @brief Sends one reliable ping to the first online peer.
     * @param nowMs Sender timestamp stored in the payload and later echoed by
     *        the pong, so the application round-trip interval can be measured.
     *
     * Does nothing when no peer is online. The sequence advances on every
     * attempt and skips zero on wrap, so it stays a valid application id for
     * the life of the session. A send that the session does not accept counts
     * as a failure instead of as a sent ping.
     */
    void sendPing(std::uint32_t nowMs);

    /**
     * @brief Routes one event: reply to a ping, measure a pong, count failures.
     * @param event Event drained from the session queue.
     *
     * Peer lifecycle events need no protocol action: the UI reads the peer
     * count live. Unknown tags and malformed payloads are ignored.
     */
    void onEvent(const pixelroot32::networking::NetEvent& event);

    /** @brief Returns the session this node owns. */
    pixelroot32::networking::NetSession& getSession();

    /** @brief Returns the session this node owns. */
    const pixelroot32::networking::NetSession& getSession() const;

    /** @brief Returns the application label from the build flag. */
    std::uint8_t nodeId() const;

    /** @brief Returns how many pings this node accepted for sending. */
    std::uint32_t pingsSent() const;

    /** @brief Returns how many pongs this node accepted for sending. */
    std::uint32_t pongsSent() const;

    /** @brief Returns how many valid pongs this node received. */
    std::uint32_t pongsReceived() const;

    /** @brief Returns the last measured application round-trip interval in ms. */
    std::uint32_t lastRttMs() const;

    /** @brief Reports whether @ref lastRttMs holds a measurement. */
    bool hasRtt() const;

    /** @brief Returns how many sends were rejected, or failed after retries. */
    std::uint32_t failures() const;

    /** @brief Returns the number of online peers. */
    std::uint8_t peerCount() const;

private:
    PingTransport transport;                        ///< Link; never shared.
    pixelroot32::networking::NetSession session;    ///< Session on that link.
    std::uint8_t nodeIdValue = 0;                   ///< Application label.
    std::uint16_t bootIdValue = 0;                  ///< 0 = generate per boot.
    std::uint16_t nextSequence = 0;                 ///< Next ping id; 0 is skipped.
    std::uint32_t pingsSentCount = 0;               ///< Accepted ping sends.
    std::uint32_t pongsSentCount = 0;               ///< Accepted pong replies.
    std::uint32_t pongsReceivedCount = 0;           ///< Valid pongs received.
    std::uint32_t lastRttValue = 0;                 ///< Last measured interval, ms.
    std::uint32_t failureCount = 0;                 ///< Rejected or failed sends.
    bool hasRttValue = false;                       ///< True after one measurement.
};

/**
 * @class PingPongScene
 * @brief Two-board ping/pong demo, with both boards simulated on native.
 *
 * This is the minimal custom-message example: a tagged payload, a pure codec,
 * one reliable send, one automatic reply in @ref onNetworkEvent, and an
 * application-level round-trip measurement.
 *
 * The scene is the composition root the engine examples usually avoid: the
 * Engine borrows a session and never owns a transport, so somebody has to bring
 * the link up, start the session and register the pointer. That is this class.
 *
 * Native runs one Engine and therefore one pumped session. The second simulated
 * board is driven from @ref update (session update, event drain), which is a
 * simulation detail, not an engine limitation: two real Engines would each pump
 * their own session.
 *
 * The scene owns every node as a direct member and draws only Renderer text,
 * because the target panel is 1-bit.
 */
class PingPongScene : public pixelroot32::core::Scene {
public:
#ifdef PLATFORM_NATIVE
    /**
     * @brief Builds the simulated pair: node 1 and node 2 on one bus.
     *
     * Both transports share one lossless loopback bus, so this mode proves the
     * protocol and the drawing, not the radio.
     */
    PingPongScene();
#else
    /**
     * @brief Builds the one hardware node this build represents.
     * @param nodeId Application label from ESP_NOW_NODE_ID.
     */
    explicit PingPongScene(std::uint8_t nodeId);
#endif

    /**
     * @brief Starts the node(s) and publishes the primary session.
     *
     * On native the scene registers the primary session with the Engine; on
     * device setup() registers it after this method has brought the link up.
     */
    void init() override;

    /**
     * @brief Pumps the simulated second board and reads the local input.
     * @param deltaTime Milliseconds since the previous frame; unused because
     *        every timestamp in this demo comes from millis().
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws one 72x40 panel per node, side by side on native.
     * @param renderer Renderer to draw with; only White text and a divider.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Skips the frame when nothing visible changed.
     * @return true when a drawn counter, RTT, failure count or peer count
     *         differs from the last queried frame.
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
        std::uint32_t pingsSent = 0;      ///< TX counter.
        std::uint32_t pongsReceived = 0;  ///< RX counter.
        std::uint32_t lastRttMs = 0;      ///< RTT value.
        std::uint32_t failures = 0;       ///< ERR marker and its number.
        std::uint8_t peers = 0;           ///< PEER / OFFLINE choice.
        bool hasRtt = false;              ///< RTT -- versus a measurement.
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
     * @param node Node whose counters and link state are shown.
     * @param originX Left edge of the panel in logical coordinates.
     */
    void drawNode(pixelroot32::graphics::Renderer& renderer,
                  const DemoNode& node, int originX);

#ifdef PLATFORM_NATIVE
    pixelroot32::networking::LoopbackBus bus;  ///< Bus shared by both nodes.
    DemoNode primary;                          ///< Panel A, node 1.
    DemoNode secondary;                        ///< Panel B, node 2.
#else
    DemoNode primary;                          ///< The one hardware node.
#endif

    mutable RedrawKey lastKey{};    ///< Sample from the last redraw decision.
    mutable bool hasKey = false;    ///< false forces the first draw.
};

}  // namespace esp_now_ping
