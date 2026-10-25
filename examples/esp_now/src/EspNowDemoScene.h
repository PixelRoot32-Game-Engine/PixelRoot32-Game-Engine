#pragma once

#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <platforms/EngineConfig.h>

#include <cstdint>

// The demo has nothing to show without the module: fail the build instead of
// silently drawing two zeroes with no link behind them.
#if !PIXELROOT32_ENABLE_NETWORKING
#error "examples/esp_now requires PIXELROOT32_ENABLE_NETWORKING=1"
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

namespace esp_now_demo {

#ifdef PLATFORM_NATIVE
/** @brief Simulated link used by the native environment. */
using DemoTransport = pixelroot32::networking::LoopbackTransport;
#else
/** @brief Radio link used by the two ESP32-C3 environments. */
using DemoTransport = pixelroot32::networking::ESP32EspNowTransport;
#endif

/**
 * @struct DemoNode
 * @brief One board's complete networking state: link, session and counter.
 *
 * The transport, session and counter are direct members, so a node reserves its
 * whole footprint at construction and never allocates. A node never owns the
 * other side of the link: discovery finds the peer, and the counter replicates
 * only to peers that are already online.
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
     * @param delta Signed change one local press applies, +1 or -1.
     * @param bootId Fixed boot generation that makes the simulation repeatable.
     */
    DemoNode(pixelroot32::networking::LoopbackBus& bus,
             const pixelroot32::networking::NetAddress& address,
             std::uint8_t nodeId, std::int8_t delta, std::uint16_t bootId);
#else
    /**
     * @brief Creates a device node with an idle ESP-NOW transport.
     * @param nodeId Application label from ESP_NOW_NODE_ID.
     * @param delta Signed change one button press applies, from ESP_NOW_DELTA.
     * @param bootId Boot generation; 0 asks the session to generate one per boot.
     */
    DemoNode(std::uint8_t nodeId, std::int8_t delta, std::uint16_t bootId);
#endif

    /**
     * @brief Brings the transport up, starts the session and binds the counter.
     * @param channel Wi-Fi channel shared by every node, 1..13.
     * @return The first failing result, or NetResult::Ok when the node is ready.
     *
     * Idempotent: calling it again re-applies the same configuration and resets
     * the replicated state to the local zero. It never throws and never blocks.
     */
    pixelroot32::networking::NetResult start(std::uint8_t channel);

    /**
     * @brief Advances the counter clock.
     * @param nowMs Current time in milliseconds, from millis().
     *
     * Must run once per frame before local input is read, so a PeerJoined
     * handler compares the peer's advertised uptime against a current clock.
     */
    void update(std::uint32_t nowMs);

    /**
     * @brief Pumps the session and feeds every queued event to the counter.
     * @param nowMs Current time in milliseconds.
     *
     * Only the secondary native node calls this: the Engine pumps and dispatches
     * the primary session itself. A device node is always pumped by the Engine.
     */
    void pump(std::uint32_t nowMs);

    /**
     * @brief Applies one event to this node's counter.
     * @param event Event drained from the session queue.
     */
    void applyEvent(const pixelroot32::networking::NetEvent& event);

    /**
     * @brief Applies this node's role delta locally and replicates it.
     * @return The counter's result; NetResult::Busy means a snapshot is pending.
     */
    pixelroot32::networking::NetResult add();

    /** @brief Returns the session this node owns. */
    pixelroot32::networking::NetSession& getSession();

    /** @brief Returns the session this node owns. */
    const pixelroot32::networking::NetSession& getSession() const;

    /** @brief Returns the counter this node owns. */
    pixelroot32::networking::NetCounter& getCounter();

    /** @brief Returns the counter this node owns. */
    const pixelroot32::networking::NetCounter& getCounter() const;

    /** @brief Returns the application label from the build flag. */
    std::uint8_t nodeId() const;

    /** @brief Returns the signed change one local press applies. */
    std::int8_t delta() const;

    /** @brief Returns the converged value. */
    std::int32_t value() const;

    /** @brief Returns how many real value changes have been applied. */
    std::uint32_t revision() const;

    /** @brief Returns the number of online peers. */
    std::uint8_t peerCount() const;

    /** @brief Reports whether a snapshot import is in progress. */
    bool isResyncing() const;

    /** @brief Reports whether a resync gave up without a snapshot. */
    bool isOutOfSync() const;

    /** @brief Reports whether transport, session and counter all started. */
    bool isReady() const;

private:
    DemoTransport transport;                        ///< Link; never shared.
    pixelroot32::networking::NetSession session;    ///< Session on that link.
    pixelroot32::networking::NetCounter counter;    ///< Convergent shared value.
    std::uint8_t nodeIdValue = 0;                   ///< Application label.
    std::int8_t deltaValue = 0;                     ///< Role: +1 or -1.
    std::uint16_t bootIdValue = 0;                  ///< 0 = generate per boot.
    bool ready = false;                             ///< True after a clean start.
};

/**
 * @class EspNowDemoScene
 * @brief Two-board ESP-NOW counter demo, with both boards simulated on native.
 *
 * The scene is the composition root the engine examples usually avoid: the
 * Engine borrows a session and never owns a transport, so somebody has to bring
 * the link up, start the session and register the pointer. That is this class.
 *
 * Native runs one Engine and therefore one pumped session. The second simulated
 * board is driven from @ref update (session update, event drain, counter), which
 * is a simulation detail, not an engine limitation: two real Engines would each
 * pump their own session.
 *
 * The scene owns every node as a direct member and draws only Renderer text and
 * line primitives, because the target panel is 1-bit.
 */
class EspNowDemoScene : public pixelroot32::core::Scene {
public:
#ifdef PLATFORM_NATIVE
    /**
     * @brief Builds the simulated pair: panel A increments, panel B decrements.
     *
     * Both transports share one lossless loopback bus, so this mode proves the
     * protocol and the drawing, not the radio.
     */
    EspNowDemoScene();
#else
    /**
     * @brief Builds the one hardware node this build represents.
     * @param nodeId Application label from ESP_NOW_NODE_ID.
     * @param delta Signed change from ESP_NOW_DELTA.
     */
    EspNowDemoScene(std::uint8_t nodeId, std::int8_t delta);
#endif

    /**
     * @brief Starts the node(s) and publishes the primary session.
     *
     * On native the scene registers the primary session with the Engine; on
     * device setup() registers it after this method has brought the link up.
     */
    void init() override;

    /**
     * @brief Advances both counters, pumps the simulated second board and reads
     *        the local input.
     * @param deltaTime Milliseconds since the previous frame; unused because
     *        every timer in this demo comes from millis().
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws one 72x40 panel per node, side by side on native.
     * @param renderer Renderer to draw with; only text and a divider are used.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Skips the frame when nothing visible changed.
     * @return true when a counter revision, a peer count or a resync/out-of-sync
     *         flag differs from the last queried frame.
     *
     * A still U8G2 buffer is indistinguishable from a hung program, so the
     * scene redraws only on real state changes and leaves a static screen alone.
     */
    bool shouldRedrawFramebuffer() const override;

    /**
     * @brief Forwards one engine-drained event to the primary counter.
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
    /** @brief Visible state whose change justifies a redraw. */
    struct RedrawKey {
        std::uint32_t primaryRevision = 0;    ///< Primary counter changes.
        std::uint32_t secondaryRevision = 0;  ///< Secondary counter changes.
        std::uint8_t primaryPeers = 0;        ///< Primary online peer count.
        std::uint8_t secondaryPeers = 0;      ///< Secondary online peer count.
        bool primaryResyncing = false;        ///< Primary SYNC marker.
        bool primaryOutOfSync = false;        ///< Primary "!" marker.
        bool secondaryResyncing = false;      ///< Secondary SYNC marker.
        bool secondaryOutOfSync = false;      ///< Secondary "!" marker.
    };

    /** @brief Samples the state that affects the drawn pixels. */
    RedrawKey makeRedrawKey() const;

    /**
     * @brief Compares two samples field by field.
     * @param lhs First sample.
     * @param rhs Second sample.
     * @return true when every visible field matches.
     */
    static bool sameKey(const RedrawKey& lhs, const RedrawKey& rhs);

    /**
     * @brief Draws one node's panel.
     * @param renderer Renderer to draw with.
     * @param node Node whose value and link state are shown.
     * @param originX Left edge of the panel in logical coordinates.
     * @param label Header text, e.g. "A +" or "N2 -".
     */
    void drawNode(pixelroot32::graphics::Renderer& renderer,
                  const DemoNode& node, int originX, const char* label);

#ifdef PLATFORM_NATIVE
    pixelroot32::networking::LoopbackBus bus;  ///< Bus shared by both nodes.
    DemoNode primary;                          ///< Panel A, +1.
    DemoNode secondary;                        ///< Panel B, -1.
#else
    DemoNode primary;                          ///< The one hardware node.
#endif

    mutable RedrawKey lastKey{};    ///< Sample from the last redraw decision.
    mutable bool hasKey = false;    ///< false forces the first draw.
};

}  // namespace esp_now_demo
