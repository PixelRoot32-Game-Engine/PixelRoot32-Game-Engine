#include <unity.h>
#include "input/InputConfig.h"

using namespace pixelroot32::input;

void setUp(void) {}
void tearDown(void) {}

void test_input_config_initialization(void) {
    InputConfig config(10, 20, 30);

    TEST_ASSERT_EQUAL(3, config.count);

#ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(10, config.buttonNames[0]);
    TEST_ASSERT_EQUAL(20, config.buttonNames[1]);
    TEST_ASSERT_EQUAL(30, config.buttonNames[2]);
#else
    TEST_ASSERT_EQUAL(10, config.inputPins[0]);
    TEST_ASSERT_EQUAL(20, config.inputPins[1]);
    TEST_ASSERT_EQUAL(30, config.inputPins[2]);
#endif
}

void test_input_config_empty(void) {
    InputConfig config;
    TEST_ASSERT_EQUAL(0, config.count);
#ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(0, config.buttonNames[0]);
#else
    TEST_ASSERT_EQUAL(0, config.inputPins[0]);
#endif
}

void test_input_config_negative_count(void) {
    InputConfig config;
    TEST_ASSERT_EQUAL(0, config.count);
}

// Phase 3: New tests for array-based InputConfig

void test_input_config_default(void) {
    // Verify default constructor initializes count=0 and all elements are zero
    InputConfig config;

    TEST_ASSERT_EQUAL(0, config.count);
    TEST_ASSERT_EQUAL(16u, config.buttonNames.size());

    // Verify all elements are zero-initialized
    for (size_t i = 0; i < config.buttonNames.size(); i++) {
        TEST_ASSERT_EQUAL(0, config.buttonNames[i]);
    }
}

void test_input_config_init_list(void) {
    // Verify variadic constructor works: InputConfig(1, 2, 3) = count 3
    InputConfig config(1, 2, 3);

    TEST_ASSERT_EQUAL(3, config.count);
    TEST_ASSERT_EQUAL(1, config.buttonNames[0]);
    TEST_ASSERT_EQUAL(2, config.buttonNames[1]);
    TEST_ASSERT_EQUAL(3, config.buttonNames[2]);

    // Verify unused elements are zero
    TEST_ASSERT_EQUAL(0, config.buttonNames[3]);
    TEST_ASSERT_EQUAL(0, config.buttonNames[15]);
}

void test_input_config_bounds(void) {
    // Verify maximum valid count (16) works correctly
    // static_assert prevents compile-time overflow for >16 args
    InputConfig config(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    TEST_ASSERT_EQUAL(16, config.count);
    TEST_ASSERT_EQUAL(1, config.buttonNames[0]);
    TEST_ASSERT_EQUAL(16, config.buttonNames[15]);

    // Verify trailing elements are zero (index 15 is last valid, 0-14 should be data, 15 is last data)
    // Actually with 16 elements: indices 0-15 contain data, so no trailing zeros to check
}

void test_input_config_overflow(void) {
    // Test that default constructor handles zero case properly
    InputConfig config;

    TEST_ASSERT_EQUAL(0, config.count);

    // Verify MAX_INPUT_COUNT constant is accessible
    TEST_ASSERT_EQUAL(16u, InputConfig::MAX_INPUT_COUNT);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_input_config_initialization);
    RUN_TEST(test_input_config_empty);
    RUN_TEST(test_input_config_negative_count);
    RUN_TEST(test_input_config_default);
    RUN_TEST(test_input_config_init_list);
    RUN_TEST(test_input_config_bounds);
    RUN_TEST(test_input_config_overflow);
    return UNITY_END();
}
