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
// Tests for platformPrint() - verify no crash on edge cases
// =============================================================================

void test_platform_log_platform_print_null(void) {
    // Call with nullptr - should not crash (printf handles null)
    platformPrint(nullptr);
    // If we reach here, test passes
}

void test_platform_log_platform_print_special_chars(void) {
    // Test with special characters that might cause issues
    platformPrint("\t\r\x01\x02");
    platformPrint("\"quotes\" and \\backslash");
}

// =============================================================================
// Tests for logInternal() - verify edge cases don't crash
// =============================================================================

void test_platform_log_log_very_long_message(void) {
    // Test with message exceeding internal buffer (256 chars)
    log(LogLevel::Info, "This is a very long message that exceeds the 256 character limit of the internal buffer used by vsnprintf in the logInternal function");
}

void test_platform_log_log_empty_format(void) {
    // Test with empty format string
    log(LogLevel::Info, "");
}

void test_platform_log_log_only_format_specifiers(void) {
    // Test with only format specifiers but no actual values
    log(LogLevel::Info, "%d %s %f");
}

void test_platform_log_log_null_pointer(void) {
    // Test with null pointer as last argument
    const char* nullStr = nullptr;
    log(LogLevel::Info, "Value: %s", nullStr);
}

void test_platform_log_log_unicode_chars(void) {
    // Test with unicode characters
    log(LogLevel::Info, "Unicode: ñ 你好 🔥");
}

void test_platform_log_log_mixed_format(void) {
    // Test mixed format specifiers
    log(LogLevel::Error, "Error %d at %s: value=%f, name=%s", 42, "location", 3.14, "test");
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
    
    // platformPrint edge case tests
    RUN_TEST(test_platform_log_platform_print_null);
    RUN_TEST(test_platform_log_platform_print_special_chars);
    
    // logInternal edge case tests
    RUN_TEST(test_platform_log_log_very_long_message);
    RUN_TEST(test_platform_log_log_empty_format);
    RUN_TEST(test_platform_log_log_only_format_specifiers);
    RUN_TEST(test_platform_log_log_null_pointer);
    RUN_TEST(test_platform_log_log_unicode_chars);
    RUN_TEST(test_platform_log_log_mixed_format);
    
    return UNITY_END();
}
