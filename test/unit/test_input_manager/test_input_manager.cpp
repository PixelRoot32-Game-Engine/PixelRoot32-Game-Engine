#include <unity.h>
#include "../../test_config.h"
#include "input/InputManager.h"
#include "input/InputConfig.h"
#include "input/TouchEvent.h"
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

void test_input_manager_multiple_buttons(void) {
    InputConfig config(3, 10, 11, 12);
    InputManager manager(config);
    manager.init();
    
    mockKeyboardState[10] = 1;
    mockKeyboardState[12] = 1;
    manager.update(1, mockKeyboardState);
    
    TEST_ASSERT_TRUE(manager.isButtonDown(0));
    TEST_ASSERT_FALSE(manager.isButtonDown(1));
    TEST_ASSERT_TRUE(manager.isButtonDown(2));
}

void test_input_manager_button_state_persistence(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    mockKeyboardState[10] = 1;
    manager.update(1, mockKeyboardState);
    TEST_ASSERT_TRUE(manager.isButtonDown(0));
    
    manager.update(50, mockKeyboardState);
    TEST_ASSERT_TRUE(manager.isButtonDown(0));
    
    mockKeyboardState[10] = 0;
    manager.update(200, mockKeyboardState);
    TEST_ASSERT_FALSE(manager.isButtonDown(0));
}

void test_input_manager_is_button_pressed_clears_after_frame(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    mockKeyboardState[10] = 1;
    manager.update(1, mockKeyboardState);
    TEST_ASSERT_TRUE(manager.isButtonPressed(0));
    
    manager.update(50, mockKeyboardState);
    TEST_ASSERT_FALSE(manager.isButtonPressed(0));
}

void test_input_manager_is_button_released_clears_after_frame(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    mockKeyboardState[10] = 1;
    manager.update(1, mockKeyboardState);
    
    mockKeyboardState[10] = 0;
    manager.update(101, mockKeyboardState);
    TEST_ASSERT_TRUE(manager.isButtonReleased(0));
    
    manager.update(50, mockKeyboardState);
    TEST_ASSERT_FALSE(manager.isButtonReleased(0));
}

void test_input_manager_touch_events_empty(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    TouchEvent events[5];
    uint8_t count = manager.getTouchEvents(events, 5);
    TEST_ASSERT_EQUAL(0, count);
    
    TEST_ASSERT_FALSE(manager.hasTouchEvents());
}

void test_input_manager_get_touch_state_idle(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
}

void test_input_manager_invalid_button_index_all_methods(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    TEST_ASSERT_FALSE(manager.isButtonDown(99));
    TEST_ASSERT_FALSE(manager.isButtonPressed(99));
    TEST_ASSERT_FALSE(manager.isButtonReleased(99));
    TEST_ASSERT_FALSE(manager.isButtonClicked(99));
}

void test_input_manager_zero_button_count(void) {
    InputConfig config(0);
    InputManager manager(config);
    manager.init();
    
    TEST_ASSERT_FALSE(manager.isButtonDown(0));
    TEST_ASSERT_FALSE(manager.isButtonPressed(0));
}

void test_input_manager_config_max_buttons(void) {
    InputConfig config(16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    InputManager manager(config);
    manager.init();
    
    mockKeyboardState[0] = 1;
    manager.update(1, mockKeyboardState);
    TEST_ASSERT_TRUE(manager.isButtonDown(0));
    
    mockKeyboardState[15] = 1;
    manager.update(1, mockKeyboardState);
    TEST_ASSERT_TRUE(manager.isButtonDown(15));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_input_manager_initialization);
    RUN_TEST(test_input_manager_button_press);
    RUN_TEST(test_input_manager_button_release);
    RUN_TEST(test_input_manager_debouncing);
    RUN_TEST(test_input_manager_click);
    RUN_TEST(test_input_manager_out_of_bounds);
    RUN_TEST(test_input_manager_multiple_buttons);
    RUN_TEST(test_input_manager_button_state_persistence);
    RUN_TEST(test_input_manager_is_button_pressed_clears_after_frame);
    RUN_TEST(test_input_manager_is_button_released_clears_after_frame);
    RUN_TEST(test_input_manager_touch_events_empty);
    RUN_TEST(test_input_manager_get_touch_state_idle);
    RUN_TEST(test_input_manager_invalid_button_index_all_methods);
    RUN_TEST(test_input_manager_zero_button_count);
    RUN_TEST(test_input_manager_config_max_buttons);
    return UNITY_END();
}
