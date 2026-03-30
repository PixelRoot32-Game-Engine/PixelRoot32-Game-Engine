/**
 * @file test_platform_capabilities.cpp
 * @brief Unit tests for platforms/PlatformCapabilities module
 * @version 1.0
 * @date 2026-03-29
 * 
 * Tests for PlatformCapabilities including:
 * - detect() function
 * - Struct members (hasDualCore, coreCount, etc.)
 * - Platform-specific behavior
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformCapabilities.h"

using namespace pixelroot32::platforms;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for PlatformCapabilities detect()
// =============================================================================

void test_platform_capabilities_detect_returns_valid_struct(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE(caps.coreCount >= 1);
}

void test_platform_capabilities_detect_has_valid_core_ids(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE(caps.audioCoreId >= 0);
    TEST_ASSERT_TRUE(caps.mainCoreId >= 0);
}

void test_platform_capabilities_detect_audio_priority_valid(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE(caps.audioPriority >= 1);
}

// =============================================================================
// Tests for PlatformCapabilities default values
// =============================================================================

void test_platform_capabilities_has_dual_core_is_valid(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE((caps.hasDualCore == true) || (caps.hasDualCore == false));
}

void test_platform_capabilities_core_count_matches_dual_core(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    if (caps.hasDualCore) {
        TEST_ASSERT_TRUE(caps.coreCount >= 2);
    } else {
        TEST_ASSERT_EQUAL_INT(1, caps.coreCount);
    }
}

void test_platform_capabilities_has_wifi_is_valid(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE((caps.hasWifi == true) || (caps.hasWifi == false));
}

void test_platform_capabilities_has_bluetooth_is_valid(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE((caps.hasBluetooth == true) || (caps.hasBluetooth == false));
}

// =============================================================================
// Tests for NATIVE platform specific behavior
// =============================================================================

#ifdef PLATFORM_NATIVE

void test_platform_capabilities_native_has_dual_core(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE(caps.hasDualCore);
}

void test_platform_capabilities_native_core_count_greater_than_one(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE(caps.coreCount > 1);
}

void test_platform_capabilities_native_audio_priority_valid(void) {
    PlatformCapabilities caps = PlatformCapabilities::detect();
    
    TEST_ASSERT_TRUE(caps.audioPriority >= 1);
}

#endif

// =============================================================================
// Tests for struct members exist and are accessible
// =============================================================================

void test_platform_capabilities_members_are_accessible(void) {
    PlatformCapabilities caps;
    
    caps.hasDualCore = true;
    caps.hasWifi = false;
    caps.hasBluetooth = false;
    caps.coreCount = 2;
    caps.audioCoreId = 0;
    caps.mainCoreId = 1;
    caps.audioPriority = 5;
    
    TEST_ASSERT_TRUE(caps.hasDualCore);
    TEST_ASSERT_FALSE(caps.hasWifi);
    TEST_ASSERT_FALSE(caps.hasBluetooth);
    TEST_ASSERT_EQUAL_INT(2, caps.coreCount);
    TEST_ASSERT_EQUAL_INT(0, caps.audioCoreId);
    TEST_ASSERT_EQUAL_INT(1, caps.mainCoreId);
    TEST_ASSERT_EQUAL_INT(5, caps.audioPriority);
}

void test_platform_capabilities_default_values(void) {
    PlatformCapabilities caps;
    
    TEST_ASSERT_FALSE(caps.hasDualCore);
    TEST_ASSERT_FALSE(caps.hasWifi);
    TEST_ASSERT_FALSE(caps.hasBluetooth);
    TEST_ASSERT_EQUAL_INT(1, caps.coreCount);
    TEST_ASSERT_EQUAL_INT(0, caps.audioCoreId);
    TEST_ASSERT_EQUAL_INT(0, caps.mainCoreId);
    TEST_ASSERT_EQUAL_INT(5, caps.audioPriority);
}

// =============================================================================
// Unity test runner
// =============================================================================

void setUpSuite(void) {
}

void tearDownSuite(void) {
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_platform_capabilities_detect_returns_valid_struct);
    RUN_TEST(test_platform_capabilities_detect_has_valid_core_ids);
    RUN_TEST(test_platform_capabilities_detect_audio_priority_valid);
    
    RUN_TEST(test_platform_capabilities_has_dual_core_is_valid);
    RUN_TEST(test_platform_capabilities_core_count_matches_dual_core);
    RUN_TEST(test_platform_capabilities_has_wifi_is_valid);
    RUN_TEST(test_platform_capabilities_has_bluetooth_is_valid);
    
    #ifdef PLATFORM_NATIVE
    RUN_TEST(test_platform_capabilities_native_has_dual_core);
    RUN_TEST(test_platform_capabilities_native_core_count_greater_than_one);
    RUN_TEST(test_platform_capabilities_native_audio_priority_valid);
    #endif
    
    RUN_TEST(test_platform_capabilities_members_are_accessible);
    RUN_TEST(test_platform_capabilities_default_values);
    
    return UNITY_END();
}
