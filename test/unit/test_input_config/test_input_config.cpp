#include <unity.h>
#include "input/InputConfig.h"

using namespace pixelroot32::input;

void setUp(void) {}
void tearDown(void) {}

void test_input_config_initialization(void) {
    InputConfig config(3, 10, 20, 30);
    
    TEST_ASSERT_EQUAL(3, config.count);
    
#ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(3, config.buttonNames.size());
    TEST_ASSERT_EQUAL(10, config.buttonNames[0]);
    TEST_ASSERT_EQUAL(20, config.buttonNames[1]);
    TEST_ASSERT_EQUAL(30, config.buttonNames[2]);
#else
    TEST_ASSERT_EQUAL(3, config.inputPins.size());
    TEST_ASSERT_EQUAL(10, config.inputPins[0]);
    TEST_ASSERT_EQUAL(20, config.inputPins[1]);
    TEST_ASSERT_EQUAL(30, config.inputPins[2]);
#endif
}

void test_input_config_empty(void) {
    InputConfig config(0);
    TEST_ASSERT_EQUAL(0, config.count);
#ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(0, config.buttonNames.size());
#else
    TEST_ASSERT_EQUAL(0, config.inputPins.size());
#endif
}

void test_input_config_negative_count(void) {
    InputConfig config(-1);
    TEST_ASSERT_EQUAL(0, config.count);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_input_config_initialization);
    RUN_TEST(test_input_config_empty);
    RUN_TEST(test_input_config_negative_count);
    return UNITY_END();
}
