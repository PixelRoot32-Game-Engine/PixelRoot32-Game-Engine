/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unified test runner for test_ui - includes all UI tests
 */

#include <unity.h>
#include "../../test_config.h"

// Include test headers
#include "test_ui_anchor_layout.h"
#include "test_ui_elements.h"
#include "test_ui_layouts.h"
#include "test_ui_layout_advanced.h"
#include "test_ui_padding.h"

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main() {
    UNITY_BEGIN();

    // UI Anchor Layout Tests (22 tests)
    RUN_TEST(test_anchor_layout_initialization);
    RUN_TEST(test_anchor_layout_set_screen_size);
    RUN_TEST(test_anchor_top_left);
    RUN_TEST(test_anchor_top_right);
    RUN_TEST(test_anchor_bottom_left);
    RUN_TEST(test_anchor_bottom_right);
    RUN_TEST(test_anchor_center);
    RUN_TEST(test_anchor_top_center);
    RUN_TEST(test_anchor_bottom_center);
    RUN_TEST(test_anchor_left_center);
    RUN_TEST(test_anchor_right_center);
    RUN_TEST(test_anchor_layout_multiple_elements);
    RUN_TEST(test_anchor_layout_remove_element);
    RUN_TEST(test_anchor_layout_update_layout);
    RUN_TEST(test_anchor_layout_default_anchor);
    RUN_TEST(test_anchor_layout_different_sizes);
    RUN_TEST(test_anchor_layout_vector2_constructor);
    RUN_TEST(test_anchor_layout_empty);
    RUN_TEST(test_anchor_layout_clear_elements);
    RUN_TEST(test_anchor_with_label);
    RUN_TEST(test_anchor_all_nine_positions);

    // UI Elements Tests (33 tests + new = 41 tests)
    RUN_TEST(test_label_initialization);
    RUN_TEST(test_label_set_text);
    RUN_TEST(test_button_initialization);
    RUN_TEST(test_button_callback_invocation);
    RUN_TEST(test_checkbox_initialization);
    RUN_TEST(test_checkbox_toggle);
    RUN_TEST(test_checkbox_set_checked);
    RUN_TEST(test_checkbox_callback);
    // UICheckBox full coverage tests
    RUN_TEST(test_checkbox_handle_input_not_visible);
    RUN_TEST(test_checkbox_handle_input_not_enabled);
    RUN_TEST(test_checkbox_handle_input_not_selected);
    RUN_TEST(test_checkbox_handle_input_selected_pressed);
    RUN_TEST(test_checkbox_draw_selected_no_background);
    RUN_TEST(test_checkbox_draw_checked_selected);
    RUN_TEST(test_checkbox_draw_fixed_position);
    RUN_TEST(test_checkbox_draw_different_font_sizes);
    RUN_TEST(test_checkbox_empty_label);
RUN_TEST(test_checkbox_long_label);
    
    // UIElement base class value tests (for line coverage)
    RUN_TEST(test_uielement_is_focusable_default_returns_false);
    RUN_TEST(test_uielement_get_preferred_size_returns_dimensions);
    RUN_TEST(test_uielement_get_preferred_size_different_dimensions);
    
    return UNITY_END();
}