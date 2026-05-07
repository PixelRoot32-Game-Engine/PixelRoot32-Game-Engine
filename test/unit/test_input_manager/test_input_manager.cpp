#include <unity.h>
#include "../../test_config.h"
#include "input/InputManager.h"
#include "input/InputConfig.h"
#include "input/TouchEvent.h"
#include <vector>

#ifdef PLATFORM_NATIVE
#include <SDL2/SDL.h>
#endif

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

// =============================================================================
// FASE 2 coverage expansion tests
// =============================================================================

void test_input_manager_touch_event_press_release(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Inject a touch press event
    #ifndef PLATFORM_NATIVE
    manager.injectTouchPoint(100, 200, true, 0, 100);
    #endif
    
    TouchState state = manager.getTouchState(0);
    #ifndef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Pressed), static_cast<uint8_t>(state));
    #endif
    
    // Release touch
    #ifndef PLATFORM_NATIVE
    manager.injectTouchPoint(100, 200, false, 0, 200);
    state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
#endif
}

void test_input_manager_touch_event_move(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Press
    #ifndef PLATFORM_NATIVE
    manager.injectTouchPoint(100, 200, true, 0, 100);
    #endif
    
#ifndef PLATFORM_NATIVE
    manager.injectTouchPoint(120, 220, true, 0, 150);
    #endif
}

void test_input_manager_touch_event_multiple_touches(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Multiple touch IDs
    #ifndef PLATFORM_NATIVE
    manager.injectTouchPoint(100, 100, true, 0, 100);
    manager.injectTouchPoint(200, 200, true, 1, 100);
    manager.injectTouchPoint(300, 300, true, 2, 100);
    #endif
    
    // Verify multiple touches are tracked
    #ifndef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Pressed), static_cast<uint8_t>(manager.getTouchState(0)));
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Pressed), static_cast<uint8_t>(manager.getTouchState(1)));
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Pressed), static_cast<uint8_t>(manager.getTouchState(2)));
    #endif
}

void test_input_manager_touch_event_queue_full(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Try to add many touch events to fill the queue
    #ifndef PLATFORM_NATIVE
    for (int i = 0; i < 20; i++) {
        manager.injectTouchPoint(i * 10, i * 10, true, 0, i * 10);
    }
    #endif
    
    // Should not crash - verify manager is still functional
    #ifndef PLATFORM_NATIVE
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_TRUE(state == TouchState::Pressed || state == TouchState::Idle);
    #endif
}

void test_input_manager_has_touch_events_after_inject(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    #ifndef PLATFORM_NATIVE
    TEST_ASSERT_FALSE(manager.hasTouchEvents());
    
    manager.injectTouchPoint(100, 200, true, 0, 100);
    
    TEST_ASSERT_TRUE(manager.hasTouchEvents());
    #endif
}

void test_input_manager_get_touch_state_pressed(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    #ifndef PLATFORM_NATIVE
    manager.injectTouchPoint(100, 200, true, 0, 100);
    
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Pressed), static_cast<uint8_t>(state));
    #endif
}

void test_input_manager_invalid_touch_id(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Invalid touch ID should return Idle
    TouchState state = manager.getTouchState(99);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
}

// Platform-native specific tests (SDL event processing)
#ifdef PLATFORM_NATIVE
void test_input_manager_process_sdl_event_mouse_button_down(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Create a mock SDL event for mouse button down
    SDL_Event sdlEvent;
    sdlEvent.type = SDL_MOUSEBUTTONDOWN;
    sdlEvent.button.button = SDL_BUTTON_LEFT;
    sdlEvent.button.x = 100;
    sdlEvent.button.y = 200;
    sdlEvent.button.timestamp = 100;
    
    manager.processSDLEvent(&sdlEvent);
    
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Pressed), static_cast<uint8_t>(state));
}

void test_input_manager_process_sdl_event_mouse_button_up(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Press first
    SDL_Event downEvent;
    downEvent.type = SDL_MOUSEBUTTONDOWN;
    downEvent.button.button = SDL_BUTTON_LEFT;
    downEvent.button.x = 100;
    downEvent.button.y = 200;
    downEvent.button.timestamp = 100;
    manager.processSDLEvent(&downEvent);
    
    // Release
    SDL_Event upEvent;
    upEvent.type = SDL_MOUSEBUTTONUP;
    upEvent.button.button = SDL_BUTTON_LEFT;
    upEvent.button.x = 100;
    upEvent.button.y = 200;
    upEvent.button.timestamp = 200;
    manager.processSDLEvent(&upEvent);
    
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
}

void test_input_manager_process_sdl_event_mouse_motion_drag(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Press first
    SDL_Event downEvent;
    downEvent.type = SDL_MOUSEBUTTONDOWN;
    downEvent.button.button = SDL_BUTTON_LEFT;
    downEvent.button.x = 100;
    downEvent.button.y = 200;
    downEvent.button.timestamp = 100;
    manager.processSDLEvent(&downEvent);
    
    // Clear the touch events after press
    TouchEvent events[5];
    manager.getTouchEvents(events, 5);
    
    // Drag motion - far enough to trigger drag (10+ pixels)
    SDL_Event motionEvent;
    motionEvent.type = SDL_MOUSEMOTION;
    motionEvent.motion.x = 120;
    motionEvent.motion.y = 220;
    motionEvent.motion.timestamp = 150;
    manager.processSDLEvent(&motionEvent);
    
    // Check that touch events are generated
    TEST_ASSERT_TRUE(manager.hasTouchEvents());
    
    TouchEvent dragEvents[5];
    uint8_t count = manager.getTouchEvents(dragEvents, 5);
    
    // Should have at least one drag event
    TEST_ASSERT_TRUE(count > 0);
}

void test_input_manager_process_sdl_event_mouse_motion_no_drag(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Motion without pressing - should be ignored
    SDL_Event motionEvent;
    motionEvent.type = SDL_MOUSEMOTION;
    motionEvent.motion.x = 120;
    motionEvent.motion.y = 220;
    motionEvent.motion.timestamp = 150;
    manager.processSDLEvent(&motionEvent);
    
    // Should remain idle
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
}

void test_input_manager_process_sdl_event_non_mouse_ignored(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Non-mouse event - should be ignored
    SDL_Event keyEvent;
    keyEvent.type = SDL_KEYDOWN;
    keyEvent.key.keysym.sym = SDLK_SPACE;
    keyEvent.key.timestamp = 100;
    manager.processSDLEvent(&keyEvent);
    
    // Should remain idle
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
}

void test_input_manager_process_sdl_event_right_button_ignored(void) {
    InputConfig config(1, 10);
    InputManager manager(config);
    manager.init();
    
    // Right button - should be ignored
    SDL_Event sdlEvent;
    sdlEvent.type = SDL_MOUSEBUTTONDOWN;
    sdlEvent.button.button = SDL_BUTTON_RIGHT;
    sdlEvent.button.x = 100;
    sdlEvent.button.y = 200;
    sdlEvent.button.timestamp = 100;
    manager.processSDLEvent(&sdlEvent);
    
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
}
#endif // PLATFORM_NATIVE

void test_input_manager_init_invalid_count(void) {
    // Test initialization with invalid count (0)
    InputConfig config(0);
    InputManager manager(config);
    manager.init();
    
    // Verify manager is still functional - should not crash
    // Invalid touch ID should return Idle
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
}

void test_input_manager_init_count_above_max(void) {
    // Test initialization with count above MAX_BUTTONS
    InputConfig config(20);  // Above MAX_BUTTONS (16)
    InputManager manager(config);
    manager.init();
    
    // Verify manager is still functional
    // Invalid touch ID should return Idle
    TouchState state = manager.getTouchState(0);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(TouchState::Idle), static_cast<uint8_t>(state));
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
    
    // FASE 2 coverage expansion tests
    RUN_TEST(test_input_manager_touch_event_press_release);
    RUN_TEST(test_input_manager_touch_event_move);
    RUN_TEST(test_input_manager_touch_event_multiple_touches);
    RUN_TEST(test_input_manager_touch_event_queue_full);
    RUN_TEST(test_input_manager_has_touch_events_after_inject);
    RUN_TEST(test_input_manager_get_touch_state_pressed);
    RUN_TEST(test_input_manager_invalid_touch_id);
    
    #ifdef PLATFORM_NATIVE
    RUN_TEST(test_input_manager_process_sdl_event_mouse_button_down);
    RUN_TEST(test_input_manager_process_sdl_event_mouse_button_up);
    RUN_TEST(test_input_manager_process_sdl_event_mouse_motion_drag);
    RUN_TEST(test_input_manager_process_sdl_event_mouse_motion_no_drag);
    RUN_TEST(test_input_manager_process_sdl_event_non_mouse_ignored);
    RUN_TEST(test_input_manager_process_sdl_event_right_button_ignored);
    #endif
    
    RUN_TEST(test_input_manager_init_invalid_count);
    RUN_TEST(test_input_manager_init_count_above_max);
    
    return UNITY_END();
}
