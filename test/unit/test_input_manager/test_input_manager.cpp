#include <unity.h>
#include "input/InputManager.h"
#include <vector>

using namespace pixelroot32::input;

// Mock keyboard state
uint8_t mockKeyboardState[256];

void setUp(void) {
    for (int i = 0; i < 256; i++) {
        mockKeyboardState[i] = 0;
    }
}

void tearDown(void) {}

void test_input_manager_initialization(void) {
    InputConfig config(2, 10, 20);
    InputManager manager(config);
    manager.init();
    
    // Initial states should be false
    TEST_ASSERT_FALSE(manager.isButtonDown(0));
    TEST_ASSERT_FALSE(manager.isButtonDown(1));
}

void test_input_manager_button_press(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Press button 10
    mockKeyboardState[10] = 1;
    manager.update(1, mockKeyboardState);
    
    TEST_ASSERT_TRUE(manager.isButtonDown(0));
    TEST_ASSERT_TRUE(manager.isButtonPressed(0));
    TEST_ASSERT_FALSE(manager.isButtonReleased(0));
}

void test_input_manager_button_release(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Press button
    mockKeyboardState[10] = 1;
    manager.update(1, mockKeyboardState);
    
    // Release button after debounce time (100ms)
    mockKeyboardState[10] = 0;
    manager.update(101, mockKeyboardState);
    
    TEST_ASSERT_FALSE(manager.isButtonDown(0));
    TEST_ASSERT_FALSE(manager.isButtonPressed(0));
    TEST_ASSERT_TRUE(manager.isButtonReleased(0));
}

void test_input_manager_debouncing(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Press button
    mockKeyboardState[10] = 1;
    manager.update(1, mockKeyboardState);
    TEST_ASSERT_TRUE(manager.isButtonDown(0));
    
    // Try to release button immediately (within 100ms debounce)
    mockKeyboardState[10] = 0;
    manager.update(50, mockKeyboardState);
    
    // Should still be DOWN because of debouncing
    TEST_ASSERT_TRUE(manager.isButtonDown(0));
    TEST_ASSERT_FALSE(manager.isButtonReleased(0));
    
    // After debounce time
    manager.update(51, mockKeyboardState); // Total 101ms
    TEST_ASSERT_FALSE(manager.isButtonDown(0));
    TEST_ASSERT_TRUE(manager.isButtonReleased(0));
}

void test_input_manager_click(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Press
    mockKeyboardState[10] = 1;
    manager.update(1, mockKeyboardState);
    TEST_ASSERT_FALSE(manager.isButtonClicked(0)); // Just pressed, not clicked yet
    
    // Release after debounce
    mockKeyboardState[10] = 0;
    manager.update(101, mockKeyboardState);
    
    TEST_ASSERT_TRUE(manager.isButtonClicked(0));
    TEST_ASSERT_FALSE(manager.isButtonClicked(0)); // Should be false after one check
}

void test_input_manager_out_of_bounds(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    TEST_ASSERT_FALSE(manager.isButtonDown(5));
    TEST_ASSERT_FALSE(manager.isButtonPressed(5));
    TEST_ASSERT_FALSE(manager.isButtonReleased(5));
    TEST_ASSERT_FALSE(manager.isButtonClicked(5));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_input_manager_initialization);
    RUN_TEST(test_input_manager_button_press);
    RUN_TEST(test_input_manager_button_release);
    RUN_TEST(test_input_manager_debouncing);
    RUN_TEST(test_input_manager_click);
    RUN_TEST(test_input_manager_out_of_bounds);
    return UNITY_END();
}
