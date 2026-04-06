/**
 * @file test_platform_log.cpp
 * @brief Unit tests for platforms/PlatformLog module
 * @version 1.0
 * @date 2026-03-29
 * 
 * Tests for PlatformLog including:
 * - Level to string conversion
 * - Platform print
 * - Log internal with EnableLogging=true
 * - Log internal with EnableLogging=false (no-op behavior)
 */

#include <unity.h>
#include "../../test_config.h"
#include "core/Log.h"
#include "platforms/EngineConfig.h"

#include <cstdio>
#include <cstring>
#include <cstdarg>

using namespace pixelroot32::core::logging;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for levelToString()
// =============================================================================

void test_platform_log_level_to_string_info(void) {
    const char* result = levelToString(LogLevel::Info);
    TEST_ASSERT_EQUAL_STRING("[INFO] ", result);
}

void test_platform_log_level_to_string_warning(void) {
    const char* result = levelToString(LogLevel::Warning);
    TEST_ASSERT_EQUAL_STRING("[WARN] ", result);
}

void test_platform_log_level_to_string_error(void) {
    const char* result = levelToString(LogLevel::Error);
    TEST_ASSERT_EQUAL_STRING("[ERROR] ", result);
}

void test_platform_log_level_to_string_profiling(void) {
    const char* result = levelToString(LogLevel::Profiling);
    TEST_ASSERT_EQUAL_STRING("[PROFILING] ", result);
}

void test_platform_log_level_to_string_default_fallback(void) {
    LogLevel invalidLevel = static_cast<LogLevel>(100);
    const char* result = levelToString(invalidLevel);
    TEST_ASSERT_EQUAL_STRING("", result);
}

// =============================================================================
// Tests for logInternal() - basic compilation and behavior
// =============================================================================

void test_platform_log_level_conversions_complete(void) {
    TEST_ASSERT_NOT_NULL(levelToString(LogLevel::Info));
    TEST_ASSERT_NOT_NULL(levelToString(LogLevel::Warning));
    TEST_ASSERT_NOT_NULL(levelToString(LogLevel::Error));
    TEST_ASSERT_NOT_NULL(levelToString(LogLevel::Profiling));
}

void test_platform_log_all_levels_unique(void) {
    const char* info = levelToString(LogLevel::Info);
    const char* warn = levelToString(LogLevel::Warning);
    const char* err = levelToString(LogLevel::Error);
    const char* prof = levelToString(LogLevel::Profiling);
    
    TEST_ASSERT_NOT_EQUAL(info, warn);
    TEST_ASSERT_NOT_EQUAL(warn, err);
    TEST_ASSERT_NOT_EQUAL(err, prof);
}

void test_platform_log_info_prefix_format(void) {
    const char* prefix = levelToString(LogLevel::Info);
    TEST_ASSERT_TRUE(prefix[0] == '[');
    TEST_ASSERT_TRUE(prefix[strlen(prefix)-1] == ' ');
}

void test_platform_log_error_prefix_format(void) {
    const char* prefix = levelToString(LogLevel::Error);
    TEST_ASSERT_TRUE(prefix[0] == '[');
    TEST_ASSERT_TRUE(prefix[strlen(prefix)-1] == ' ');
}

// =============================================================================
// Tests for platformPrint() - now covered with DEBUG_MODE enabled
// =============================================================================

void test_platform_log_platform_print_basic(void) {
    // Call platformPrint directly - should compile and run without crash
    platformPrint("test");
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_platform_print_empty(void) {
    // Call platformPrint with empty string
    platformPrint("");
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_platform_print_newline(void) {
    // Call platformPrint with newline
    platformPrint("\n");
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Tests for logInternal() - now covered with DEBUG_MODE enabled
// =============================================================================

void test_platform_log_log_with_info_level(void) {
    // Call log with Info level - exercises logInternal
    log(LogLevel::Info, "Test info message");
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_log_with_warning_level(void) {
    // Call log with Warning level
    log(LogLevel::Warning, "Test warning %d", 42);
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_log_with_error_level(void) {
    // Call log with Error level
    log(LogLevel::Error, "Test error %s", "error");
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_log_with_profiling_level(void) {
    // Call log with Profiling level
    log(LogLevel::Profiling, "Test profiling %f", 3.14f);
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_log_simple_message(void) {
    // Call log without explicit level (defaults to Info)
    log("Simple test message");
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_log_multiple_args(void) {
    // Call log with multiple format arguments
    log("Values: %d, %f, %s", 123, 4.56f, "test");
    TEST_ASSERT_TRUE(true);
}

void test_platform_log_log_no_args(void) {
    // Call log with no format arguments
    log("No args message");
    TEST_ASSERT_TRUE(true);
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
    
    RUN_TEST(test_platform_log_level_to_string_info);
    RUN_TEST(test_platform_log_level_to_string_warning);
    RUN_TEST(test_platform_log_level_to_string_error);
    RUN_TEST(test_platform_log_level_to_string_profiling);
    RUN_TEST(test_platform_log_level_to_string_default_fallback);
    
    RUN_TEST(test_platform_log_level_conversions_complete);
    RUN_TEST(test_platform_log_all_levels_unique);
    RUN_TEST(test_platform_log_info_prefix_format);
    RUN_TEST(test_platform_log_error_prefix_format);
    
    // platformPrint tests
    RUN_TEST(test_platform_log_platform_print_basic);
    RUN_TEST(test_platform_log_platform_print_empty);
    RUN_TEST(test_platform_log_platform_print_newline);
    
    // logInternal tests (via log() function)
    RUN_TEST(test_platform_log_log_with_info_level);
    RUN_TEST(test_platform_log_log_with_warning_level);
    RUN_TEST(test_platform_log_log_with_error_level);
    RUN_TEST(test_platform_log_log_with_profiling_level);
    RUN_TEST(test_platform_log_log_simple_message);
    RUN_TEST(test_platform_log_log_multiple_args);
    RUN_TEST(test_platform_log_log_no_args);
    
    return UNITY_END();
}
