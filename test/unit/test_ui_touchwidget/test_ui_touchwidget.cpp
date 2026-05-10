/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unified test runner for UITouchWidget tests
 */

#include <unity.h>
#include "../../test_config.h"

// Include test headers
#include "test_ui_touchwidget.h"

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main() {
    UNITY_BEGIN();
    
    // UITouchElement tests (5 + 6 new = 11 tests)
    RUN_TEST(test_uitouch_element_initialization);
    RUN_TEST(test_uitouch_element_is_enabled);
    RUN_TEST(test_uitouch_element_is_disabled);
    RUN_TEST(test_uitouch_element_is_visible);
    RUN_TEST(test_uitouch_element_is_not_visible);
    // Additional coverage tests
    RUN_TEST(test_uitouch_element_set_position_syncs_widget_data);
    RUN_TEST(test_uitouch_element_set_position_different_values);
    RUN_TEST(test_uitouch_element_update_called);
    RUN_TEST(test_uitouch_element_update_with_zero_delta);
    RUN_TEST(test_uitouch_element_get_widget_state_default);
    RUN_TEST(test_uitouch_element_get_widget_state_after_press);
    
    // UITouchButton tests (15 + 3 new = 18 tests)
    RUN_TEST(test_uitouch_button_initialization);
    RUN_TEST(test_uitouch_button_set_label);
    RUN_TEST(test_uitouch_button_set_colors);
    RUN_TEST(test_uitouch_button_callbacks);
    RUN_TEST(test_uitouch_button_process_event_disabled);
    RUN_TEST(test_uitouch_button_process_event_outside_bounds);
    RUN_TEST(test_uitouch_button_process_event_inside_bounds);
    RUN_TEST(test_uitouch_button_process_event_touch_up);
    RUN_TEST(test_uitouch_button_process_event_click);
    RUN_TEST(test_uitouch_button_press_outside_bounds_resets_state);
    RUN_TEST(test_uitouch_button_reset);
    RUN_TEST(test_uitouch_button_get_callbacks);
    RUN_TEST(test_uitouch_button_get_font_size);
    RUN_TEST(test_uitouch_button_get_text_alignment);
    RUN_TEST(test_uitouch_button_get_border_colors);
    // Additional coverage tests
    RUN_TEST(test_uitouch_button_handle_touch_down_sets_state);
    RUN_TEST(test_uitouch_button_auto_size_no_label);
    RUN_TEST(test_uitouch_button_auto_size_with_label);
    
    // UITouchSlider tests (19 + 3 new = 22 tests)
    RUN_TEST(test_uitouch_slider_initialization);
    RUN_TEST(test_uitouch_slider_get_value);
    RUN_TEST(test_uitouch_slider_set_value);
    RUN_TEST(test_uitouch_slider_set_value_clamped);
    RUN_TEST(test_uitouch_slider_set_colors);
    RUN_TEST(test_uitouch_slider_process_event_disabled);
    RUN_TEST(test_uitouch_slider_process_event_outside_bounds);
    RUN_TEST(test_uitouch_slider_process_event_touch_down);
    RUN_TEST(test_uitouch_slider_process_event_drag_move);
    RUN_TEST(test_uitouch_slider_process_event_touch_up);
    RUN_TEST(test_uitouch_slider_drag_move_not_dragging);
    RUN_TEST(test_uitouch_slider_value_clamping_min);
    RUN_TEST(test_uitouch_slider_has_value_changed);
    RUN_TEST(test_uitouch_slider_previous_value);
    RUN_TEST(test_uitouch_slider_reset);
    RUN_TEST(test_uitouch_slider_get_callbacks);
    // Additional coverage tests
    RUN_TEST(test_uitouch_slider_get_on_drag_start);
    RUN_TEST(test_uitouch_slider_get_on_drag_end);
    RUN_TEST(test_uitouch_slider_handle_touch_up_exceeds_drag);
    
    // UITouchCheckbox tests (12 + 3 new = 15 tests)
    RUN_TEST(test_uitouch_checkbox_initialization);
    RUN_TEST(test_uitouch_checkbox_initialization_unchecked);
    RUN_TEST(test_uitouch_checkbox_set_label);
    RUN_TEST(test_uitouch_checkbox_set_checked);
    RUN_TEST(test_uitouch_checkbox_toggle);
    RUN_TEST(test_uitouch_checkbox_toggle_disabled);
    RUN_TEST(test_uitouch_checkbox_set_colors);
    RUN_TEST(test_uitouch_checkbox_callback);
    RUN_TEST(test_uitouch_checkbox_process_event_disabled);
    RUN_TEST(test_uitouch_checkbox_process_event_outside_bounds);
    RUN_TEST(test_uitouch_checkbox_process_event_inside_bounds);
    RUN_TEST(test_uitouch_checkbox_toggle_on_touch_up);
    // Additional coverage tests
    RUN_TEST(test_uitouch_checkbox_get_on_changed);
    RUN_TEST(test_uitouch_checkbox_font_size_get_set);
    RUN_TEST(test_uitouch_checkbox_handle_touch_up_exceeds_drag);
    
    // UIHitTest tests (6 tests)
    RUN_TEST(test_uitouch_element_hit_test_enabled_visible);
    RUN_TEST(test_uitouch_element_hit_test_disabled);
    RUN_TEST(test_uitouch_element_hit_test_not_visible);
    RUN_TEST(test_uitouch_element_hit_test_outside_bounds);
    RUN_TEST(test_uitouch_hit_test_find_hit_array);
    RUN_TEST(test_uitouch_hit_test_find_hit_no_match);
    
    return UNITY_END();
}
