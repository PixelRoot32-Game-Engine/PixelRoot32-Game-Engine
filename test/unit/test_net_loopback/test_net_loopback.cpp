/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the transport contract (networking/NetTransport.h) and the
 * native in-process backend (drivers/native/LoopbackTransport.h).
 *
 * The whole networking module is compiled out unless
 * PIXELROOT32_ENABLE_NETWORKING is 1, and the loopback backend only exists on
 * PLATFORM_NATIVE. This file therefore mirrors the dialog/net_protocol pattern:
 * the functional branch lives behind both guards, and the flag-off branch runs
 * one stub so the suite still proves the inert flags-off contract under
 * [env:native_test].
 */
#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_NETWORKING && defined(PLATFORM_NATIVE)

#include "drivers/native/LoopbackTransport.h"
#include "networking/NetTransport.h"
#include "networking/NetTypes.h"

#include <cstdint>

namespace config = pixelroot32::platforms::config;

namespace {

using pixelroot32::networking::LoopbackBus;
using pixelroot32::networking::LoopbackFaults;
using pixelroot32::networking::LoopbackTransport;
using pixelroot32::networking::NetAddress;
using pixelroot32::networking::NetPeerConfig;
using pixelroot32::networking::NetResult;
using pixelroot32::networking::NetRxInfo;
using pixelroot32::networking::NetTransportConfig;
using pixelroot32::networking::NetTxResult;
using pixelroot32::networking::kNetAddressSize;
using pixelroot32::networking::kNetTransportFrameSize;

// A frame is the transport unit, not an application payload: the complete
// protocol frame (header plus payload), 80 bytes at the default limits.
static_assert(kNetTransportFrameSize ==
                  config::NetHeaderSize + config::NetMaxPayload,
              "The transport frame size is exactly header plus payload.");
static_assert(kNetTransportFrameSize <= 250,
              "One transport frame must fit a single ESP-NOW frame.");
static_assert(sizeof(NetRxInfo::data) == kNetTransportFrameSize,
              "NetRxInfo must carry a complete frame inline.");

#define ASSERT_NET_RESULT(expected, actual)                                \
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected),                \
                            static_cast<uint8_t>(actual))

// Deterministic, readable addresses. Tag 1 and tag 2 are the usual peers; the
// bytes differ in every position so a partial copy cannot pass unnoticed.
NetAddress makeAddress(uint8_t tag) {
    NetAddress address{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        address.bytes[i] = static_cast<uint8_t>(0x10u + tag + i);
    }
    return address;
}

NetAddress makeBroadcastAddress() {
    NetAddress address{};
    for (uint8_t i = 0; i < kNetAddressSize; ++i) {
        address.bytes[i] = 0xFFu;
    }
    return address;
}

NetTransportConfig configFor(uint8_t channel) {
    NetTransportConfig config;
    config.channel = channel;
    config.nodeId = 1;
    config.manageWifi = false;
    return config;
}

void fillFrame(uint8_t* frame, uint16_t length, uint8_t seed) {
    for (uint16_t i = 0; i < length; ++i) {
        frame[i] = static_cast<uint8_t>(seed + i * 7u);
    }
}

}  // namespace

// =============================================================================
// 1. Two transports on one bus exchange a frame.
// =============================================================================

void test_net_loopback_exchanges_complete_frame(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));

    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));
    TEST_ASSERT_TRUE(sender.isReady());
    TEST_ASSERT_TRUE(receiver.localAddress() == makeAddress(2));

    uint8_t frame[7];
    fillFrame(frame, 7, 0x40);

    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 7));

    NetRxInfo rx{};
    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_UINT16(7, rx.length);
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 7);
    TEST_ASSERT_TRUE(rx.from == makeAddress(1));

    NetTxResult tx{};
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_TRUE(tx.success);
    TEST_ASSERT_TRUE(tx.to == makeAddress(2));

    // The sender never receives its own unicast frame.
    TEST_ASSERT_FALSE(sender.pollRx(rx));
}

// =============================================================================
// 2. Rejected sends: unknown peer, oversized frame, null frame.
// =============================================================================

void test_net_loopback_rejects_unknown_peer_oversized_and_null(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    uint8_t frame[3];
    fillFrame(frame, 3, 0x20);

    ASSERT_NET_RESULT(NetResult::PeerNotFound,
                      sender.send(makeAddress(9), frame, 3));

    uint8_t oversized[kNetTransportFrameSize + 1]{};
    ASSERT_NET_RESULT(NetResult::PayloadTooLarge,
                      sender.send(makeAddress(2), oversized,
                                  sizeof(oversized)));

    ASSERT_NET_RESULT(NetResult::InvalidArgument,
                      sender.send(makeAddress(2), nullptr, 3));

    NetRxInfo rx{};
    TEST_ASSERT_FALSE(receiver.pollRx(rx));
    NetTxResult tx{};
    TEST_ASSERT_FALSE(sender.pollTxResult(tx));

    // A zero-length frame is legal and still carries an address.
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), nullptr, 0));
    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_UINT16(0, rx.length);
    TEST_ASSERT_TRUE(rx.from == makeAddress(1));
}

// =============================================================================
// 3. sendBusy fault: Busy and no delivery.
// =============================================================================

void test_net_loopback_send_busy_fault_blocks_delivery(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    LoopbackFaults faults;
    faults.sendBusy = true;
    bus.setFaults(faults);

    uint8_t frame[4];
    fillFrame(frame, 4, 0x30);
    ASSERT_NET_RESULT(NetResult::Busy, sender.send(makeAddress(2), frame, 4));

    NetRxInfo rx{};
    TEST_ASSERT_FALSE(receiver.pollRx(rx));
    NetTxResult tx{};
    TEST_ASSERT_FALSE(sender.pollTxResult(tx));
}

// =============================================================================
// 4. Channel mismatch: no delivery, tx result reports failure.
// =============================================================================

void test_net_loopback_channel_mismatch_is_not_delivered(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(2)));

    uint8_t frame[3];
    fillFrame(frame, 3, 0x50);
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));

    NetRxInfo rx{};
    TEST_ASSERT_FALSE(receiver.pollRx(rx));
    NetTxResult tx{};
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_FALSE(tx.success);
    TEST_ASSERT_TRUE(tx.to == makeAddress(2));
}

// =============================================================================
// 5. Loss fault at 100 percent and 0 percent.
// =============================================================================

void test_net_loopback_loss_fault_drops_or_delivers(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    uint8_t frame[3];
    fillFrame(frame, 3, 0x60);

    LoopbackFaults faults;
    faults.lossPercent = 100;
    bus.setFaults(faults);
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));

    NetRxInfo rx{};
    TEST_ASSERT_FALSE(receiver.pollRx(rx));
    NetTxResult tx{};
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_FALSE(tx.success);

    faults.lossPercent = 0;
    bus.setFaults(faults);
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));

    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 3);
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_TRUE(tx.success);
}

// =============================================================================
// 6. Duplicate fault: two deliveries, one tx result.
// =============================================================================

void test_net_loopback_duplicate_fault_enqueues_twice_one_result(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    LoopbackFaults faults;
    faults.duplicatePercent = 100;
    bus.setFaults(faults);

    uint8_t frame[5];
    fillFrame(frame, 5, 0x70);
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 5));

    NetRxInfo first{};
    NetRxInfo second{};
    TEST_ASSERT_TRUE(receiver.pollRx(first));
    TEST_ASSERT_TRUE(receiver.pollRx(second));
    TEST_ASSERT_EQUAL_UINT16(5, first.length);
    TEST_ASSERT_EQUAL_MEMORY(frame, first.data, 5);
    TEST_ASSERT_EQUAL_MEMORY(frame, second.data, 5);
    TEST_ASSERT_TRUE(first.from == makeAddress(1));
    TEST_ASSERT_TRUE(second.from == makeAddress(1));

    NetRxInfo third{};
    TEST_ASSERT_FALSE(receiver.pollRx(third));

    NetTxResult tx{};
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_TRUE(tx.success);
    TEST_ASSERT_FALSE(sender.pollTxResult(tx));
}

// =============================================================================
// 7. Delay fault: nothing until the clock reaches dueMs.
// =============================================================================

void test_net_loopback_delay_fault_holds_frame_until_due(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    bus.setNowMs(100);
    LoopbackFaults faults;
    faults.delayMs = 50;
    bus.setFaults(faults);

    uint8_t frame[3];
    fillFrame(frame, 3, 0x80);
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));

    NetRxInfo rx{};
    TEST_ASSERT_FALSE(receiver.pollRx(rx));

    bus.setNowMs(149);
    TEST_ASSERT_FALSE(receiver.pollRx(rx));

    bus.setNowMs(150);
    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 3);
}

// =============================================================================
// 8. Broadcast reaches every other transport on the channel, never the sender.
// =============================================================================

void test_net_loopback_broadcast_reaches_other_peers(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport first(bus, makeAddress(2));
    LoopbackTransport second(bus, makeAddress(3));
    LoopbackTransport otherChannel(bus, makeAddress(4));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, first.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, second.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, otherChannel.init(configFor(2)));

    uint8_t frame[4];
    fillFrame(frame, 4, 0x90);
    ASSERT_NET_RESULT(NetResult::Ok,
                      sender.send(makeBroadcastAddress(), frame, 4));

    NetRxInfo rx{};
    TEST_ASSERT_TRUE(first.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 4);
    TEST_ASSERT_TRUE(second.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 4);
    TEST_ASSERT_FALSE(sender.pollRx(rx));
    TEST_ASSERT_FALSE(otherChannel.pollRx(rx));

    // One result per reached destination, in attach order.
    NetTxResult tx{};
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_TRUE(tx.success);
    TEST_ASSERT_TRUE(tx.to == makeAddress(2));
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_TRUE(tx.success);
    TEST_ASSERT_TRUE(tx.to == makeAddress(3));
    TEST_ASSERT_FALSE(sender.pollTxResult(tx));
}

// =============================================================================
// 9. Receive-queue overflow: Busy and the first frames survive.
// =============================================================================

void test_net_loopback_receive_queue_overflow_returns_busy(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    for (uint8_t i = 0; i < 8; ++i) {
        uint8_t frame[1] = {i};
        ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 1));
        NetTxResult tx{};
        TEST_ASSERT_TRUE(sender.pollTxResult(tx));
        TEST_ASSERT_TRUE(tx.success);
    }

    uint8_t overflowFrame[1] = {99};
    ASSERT_NET_RESULT(NetResult::Busy,
                      sender.send(makeAddress(2), overflowFrame, 1));

    NetRxInfo rx{};
    for (uint8_t i = 0; i < 8; ++i) {
        TEST_ASSERT_TRUE(receiver.pollRx(rx));
        TEST_ASSERT_EQUAL_UINT8(i, rx.data[0]);
    }
    TEST_ASSERT_FALSE(receiver.pollRx(rx));

    NetTxResult tx{};
    TEST_ASSERT_FALSE(sender.pollTxResult(tx));
}

// =============================================================================
// 10. FIFO order with no delay.
// =============================================================================

void test_net_loopback_receive_is_fifo_without_delay(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    for (uint8_t i = 0; i < 3; ++i) {
        uint8_t frame[2] = {i, static_cast<uint8_t>(i + 1)};
        ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 2));
    }

    NetRxInfo rx{};
    for (uint8_t i = 0; i < 3; ++i) {
        TEST_ASSERT_TRUE(receiver.pollRx(rx));
        TEST_ASSERT_EQUAL_UINT8(i, rx.data[0]);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(i + 1), rx.data[1]);
    }
    TEST_ASSERT_FALSE(receiver.pollRx(rx));
}

// =============================================================================
// 11. shutdown: NotInitialized, queues cleared, init works again.
// =============================================================================

void test_net_loopback_shutdown_and_reinit(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    uint8_t frame[3];
    fillFrame(frame, 3, 0xA0);
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));

    NetRxInfo rx{};
    TEST_ASSERT_TRUE(receiver.pollRx(rx));

    sender.shutdown();
    TEST_ASSERT_FALSE(sender.isReady());
    ASSERT_NET_RESULT(NetResult::NotInitialized,
                      sender.send(makeAddress(2), frame, 3));
    NetTxResult tx{};
    TEST_ASSERT_FALSE(sender.pollTxResult(tx));

    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    TEST_ASSERT_TRUE(sender.isReady());
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));
    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 3);
}

// =============================================================================
// 12. Determinism: two identical scripts on fresh buses produce the same trace.
// =============================================================================

struct ScriptTrace {
    uint8_t deliveryCount = 0;
    uint8_t delivered[64]{};
    uint8_t resultCount = 0;
    uint8_t resultSuccess[64]{};
    bool allSendsAccepted = true;
};

ScriptTrace runDeterministicScript() {
    ScriptTrace trace;
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    sender.init(configFor(1));
    receiver.init(configFor(1));

    LoopbackFaults faults;
    faults.lossPercent = 40;
    faults.duplicatePercent = 40;
    faults.reorderPercent = 40;
    faults.delayMs = 0;
    faults.seed = 0x0BADF00Du;
    bus.setFaults(faults);

    for (uint8_t i = 0; i < 16; ++i) {
        uint8_t frame[2] = {i, static_cast<uint8_t>(0xF0u + i)};
        if (sender.send(makeAddress(2), frame, 2) != NetResult::Ok) {
            trace.allSendsAccepted = false;
        }

        // Drain both queues every iteration so neither fixed array overflows
        // and the trace records the exact per-send outcome.
        NetTxResult tx{};
        while (sender.pollTxResult(tx)) {
            if (trace.resultCount < sizeof(trace.resultSuccess)) {
                trace.resultSuccess[trace.resultCount++] =
                    tx.success ? 1u : 0u;
            }
        }

        NetRxInfo rx{};
        while (receiver.pollRx(rx)) {
            if (rx.length == 2 &&
                static_cast<uint16_t>(trace.deliveryCount) + 2 <=
                    sizeof(trace.delivered)) {
                trace.delivered[trace.deliveryCount++] = rx.data[0];
                trace.delivered[trace.deliveryCount++] = rx.data[1];
            }
        }
    }

    return trace;
}

void test_net_loopback_bus_is_deterministic_across_fresh_buses(void) {
    const ScriptTrace first = runDeterministicScript();
    const ScriptTrace second = runDeterministicScript();

    TEST_ASSERT_TRUE(first.allSendsAccepted);
    TEST_ASSERT_TRUE(second.allSendsAccepted);
    TEST_ASSERT_EQUAL_UINT8(16, first.resultCount);
    TEST_ASSERT_EQUAL_UINT8(first.resultCount, second.resultCount);
    TEST_ASSERT_EQUAL_UINT8(first.deliveryCount, second.deliveryCount);
    TEST_ASSERT_TRUE(first.deliveryCount > 0);
    TEST_ASSERT_EQUAL_MEMORY(first.delivered, second.delivered,
                             first.deliveryCount);
    TEST_ASSERT_EQUAL_MEMORY(first.resultSuccess, second.resultSuccess,
                             first.resultCount);
}

// =============================================================================
// 13. setChannel accepts exactly 1..13 and leaves the channel unchanged on
//     rejection.
// =============================================================================

void test_net_loopback_set_channel_accepts_only_1_to_13(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    ASSERT_NET_RESULT(NetResult::InvalidArgument, sender.setChannel(0));
    ASSERT_NET_RESULT(NetResult::InvalidArgument, sender.setChannel(14));
    for (uint8_t channel = 1; channel <= 13; ++channel) {
        ASSERT_NET_RESULT(NetResult::Ok, sender.setChannel(channel));
    }

    uint8_t frame[3];
    fillFrame(frame, 3, 0xB0);

    // Rejected calls must not disturb the channel that was last accepted.
    ASSERT_NET_RESULT(NetResult::Ok, sender.setChannel(5));
    ASSERT_NET_RESULT(NetResult::InvalidArgument, sender.setChannel(0));
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));

    NetRxInfo rx{};
    TEST_ASSERT_FALSE(receiver.pollRx(rx));
    NetTxResult tx{};
    TEST_ASSERT_TRUE(sender.pollTxResult(tx));
    TEST_ASSERT_FALSE(tx.success);

    ASSERT_NET_RESULT(NetResult::Ok, receiver.setChannel(5));
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 3));
    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 3);
}

// =============================================================================
// Peer registration is a documented no-op for the loopback backend.
// =============================================================================

void test_net_loopback_peer_registration_is_a_no_op(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    NetPeerConfig peer;
    peer.address = makeAddress(2);
    peer.nodeId = 7;
    peer.encrypted = false;
    ASSERT_NET_RESULT(NetResult::Ok, sender.addPeer(peer));
    ASSERT_NET_RESULT(NetResult::Ok, sender.removePeer(makeAddress(2)));

    // Delivery is resolved against attached transports, so it still works.
    uint8_t frame[2] = {0xDE, 0xAD};
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 2));
    NetRxInfo rx{};
    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 2);
}

// =============================================================================
// reset() detaches every transport and clears its queues.
// =============================================================================

void test_net_loopback_bus_reset_detaches_transports(void) {
    LoopbackBus bus;
    LoopbackTransport sender(bus, makeAddress(1));
    LoopbackTransport receiver(bus, makeAddress(2));
    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));

    uint8_t frame[2] = {0x11, 0x22};
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 2));

    // Leave the frame queued: reset() must detach the transports and clear
    // their queues, not just drop the bus pointers.
    bus.reset();
    NetRxInfo queued{};
    TEST_ASSERT_FALSE(receiver.pollRx(queued));
    TEST_ASSERT_FALSE(sender.isReady());
    TEST_ASSERT_FALSE(receiver.isReady());
    ASSERT_NET_RESULT(NetResult::NotInitialized,
                      sender.send(makeAddress(2), frame, 2));

    ASSERT_NET_RESULT(NetResult::Ok, sender.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, receiver.init(configFor(1)));
    ASSERT_NET_RESULT(NetResult::Ok, sender.send(makeAddress(2), frame, 2));
    NetRxInfo rx{};
    TEST_ASSERT_TRUE(receiver.pollRx(rx));
    TEST_ASSERT_EQUAL_MEMORY(frame, rx.data, 2);
}

#else  // !PIXELROOT32_ENABLE_NETWORKING || !PLATFORM_NATIVE

void test_net_loopback_flag_off_stub_compiles(void) {
    // With the flag off (or off the native target) neither the transport
    // contract nor the loopback backend is compiled. This translation unit
    // compiling and passing without referencing any networking symbol is the
    // zero-cost flags-off property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_NETWORKING=0: no transport or loopback symbols are "
        "compiled, zero bytes reserved.");
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
    RUN_TEST(test_net_loopback_exchanges_complete_frame);
    RUN_TEST(test_net_loopback_rejects_unknown_peer_oversized_and_null);
    RUN_TEST(test_net_loopback_send_busy_fault_blocks_delivery);
    RUN_TEST(test_net_loopback_channel_mismatch_is_not_delivered);
    RUN_TEST(test_net_loopback_loss_fault_drops_or_delivers);
    RUN_TEST(test_net_loopback_duplicate_fault_enqueues_twice_one_result);
    RUN_TEST(test_net_loopback_delay_fault_holds_frame_until_due);
    RUN_TEST(test_net_loopback_broadcast_reaches_other_peers);
    RUN_TEST(test_net_loopback_receive_queue_overflow_returns_busy);
    RUN_TEST(test_net_loopback_receive_is_fifo_without_delay);
    RUN_TEST(test_net_loopback_shutdown_and_reinit);
    RUN_TEST(test_net_loopback_bus_is_deterministic_across_fresh_buses);
    RUN_TEST(test_net_loopback_set_channel_accepts_only_1_to_13);
    RUN_TEST(test_net_loopback_peer_registration_is_a_no_op);
    RUN_TEST(test_net_loopback_bus_reset_detaches_transports);
#else
    RUN_TEST(test_net_loopback_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
