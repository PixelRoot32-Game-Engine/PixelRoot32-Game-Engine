/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Compile-time contract for the optional networking module. The module is
 * default-off, so this suite must pass under both [env:native_test] (flag off,
 * the contract every existing game relies on) and [env:native_test_networking]
 * (flag on, where the same assertions prove the mirror actually tracks the
 * build flag instead of being hard-coded).
 */
#include <unity.h>
#include "platforms/EngineConfig.h"
#include "core/EngineModules.h"

namespace config = pixelroot32::platforms::config;
namespace modules = pixelroot32::modules;

void setUp(void) {}
void tearDown(void) {}

void test_networking_flag_matches_build(void) {
#if PIXELROOT32_ENABLE_NETWORKING
    TEST_ASSERT_TRUE(config::EnableNetworking);
    TEST_ASSERT_TRUE(modules::Networking);
#else
    TEST_ASSERT_FALSE(config::EnableNetworking);
    TEST_ASSERT_FALSE(modules::Networking);
#endif
}

void test_networking_limits(void) {
    TEST_ASSERT_EQUAL_UINT8(4, config::NetMaxPeers);
    TEST_ASSERT_EQUAL_UINT16(64, config::NetMaxPayload);
    TEST_ASSERT_EQUAL_UINT16(8, config::NetRxQueueCapacity);
    TEST_ASSERT_EQUAL_UINT16(8, config::NetTxQueueCapacity);
    TEST_ASSERT_EQUAL_UINT16(20, config::NetAckTimeoutMs);
    TEST_ASSERT_EQUAL_UINT8(3, config::NetMaxRetries);
    TEST_ASSERT_EQUAL_UINT16(3000, config::NetPeerTimeoutMs);
    TEST_ASSERT_EQUAL_UINT16(1000, config::NetHelloIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(1, config::NetProtocolVersion);
    TEST_ASSERT_EQUAL_UINT16(16, config::NetHeaderSize);

    // A single ESP-NOW frame is at most 250 bytes, header included.
    TEST_ASSERT_TRUE(config::NetMaxPayload + config::NetHeaderSize <= 250);
}

void test_networking_queue_capacities(void) {
    // Ring buffers use the modulo trick, so the capacities must stay powers of two.
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(2, config::NetRxQueueCapacity);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(2, config::NetTxQueueCapacity);

    TEST_ASSERT_TRUE((config::NetRxQueueCapacity &
                      (config::NetRxQueueCapacity - 1u)) == 0u);
    TEST_ASSERT_TRUE((config::NetTxQueueCapacity &
                      (config::NetTxQueueCapacity - 1u)) == 0u);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_networking_flag_matches_build);
    RUN_TEST(test_networking_limits);
    RUN_TEST(test_networking_queue_capacities);
    return UNITY_END();
}
