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

    // UI Elements Tests (33 tests)
    RUN_TEST(test_label_initialization);
    RUN_TEST(test_label_set_text);
    RUN_TEST(test_button_initialization);
    RUN_TEST(test_button_callback_invocation);
    RUN_TEST(test_checkbox_initialization);
    RUN_TEST(test_checkbox_toggle);
    RUN_TEST(test_checkbox_set_checked);
    RUN_TEST(test_checkbox_callback);
    RUN_TEST(test_panel_initialization);
    RUN_TEST(test_panel_set_child);
    RUN_TEST(test_panel_draw_basic);
    RUN_TEST(test_panel_draw_with_background);
    RUN_TEST(test_panel_draw_with_border);
    RUN_TEST(test_panel_draw_with_child);
    RUN_TEST(test_panel_update);
    RUN_TEST(test_panel_not_visible);

    // UIButton draw tests
    RUN_TEST(test_button_draw_basic);
    RUN_TEST(test_button_draw_with_background);
    RUN_TEST(test_button_draw_when_not_visible);
    RUN_TEST(test_button_is_focusable);
    RUN_TEST(test_button_press);
    RUN_TEST(test_button_press_disabled);
    RUN_TEST(test_button_press_no_callback);

    // UILabel draw tests
    RUN_TEST(test_label_draw_basic);
    RUN_TEST(test_label_draw_when_not_visible);
    RUN_TEST(test_label_draw_with_fixed_position);

    // UI Layouts Tests (13 tests + 21 new = 34 tests)
    RUN_TEST(test_vertical_layout_spacing);
    RUN_TEST(test_horizontal_layout_spacing);
    RUN_TEST(test_grid_layout_columns);
    RUN_TEST(test_vertical_layout_elements);
    RUN_TEST(test_vertical_layout_padding);
    RUN_TEST(test_horizontal_layout_navigation);
    RUN_TEST(test_horizontal_layout_scroll);
    RUN_TEST(test_grid_layout_selection);
    RUN_TEST(test_grid_layout_navigation_buttons);
    RUN_TEST(test_grid_layout_rows);
    RUN_TEST(test_grid_layout_button_style);
    RUN_TEST(test_grid_layout_change_columns);
    RUN_TEST(test_grid_layout_three_columns);
    // New layout coverage tests
    RUN_TEST(test_grid_layout_constructor_vector2);
    RUN_TEST(test_grid_layout_remove_element);
    RUN_TEST(test_grid_layout_zero_columns);
    RUN_TEST(test_grid_layout_calculate_rows_zero_columns);
    RUN_TEST(test_grid_layout_clear_elements);
    RUN_TEST(test_grid_layout_get_element);
    RUN_TEST(test_horizontal_layout_constructor_scalars);
    RUN_TEST(test_horizontal_layout_add_remove_elements);
    RUN_TEST(test_horizontal_layout_clear);
    RUN_TEST(test_horizontal_layout_get_element);
    RUN_TEST(test_horizontal_layout_set_scroll_disabled);
    RUN_TEST(test_vertical_layout_constructor_scalars);
    RUN_TEST(test_vertical_layout_add_remove_elements);
    RUN_TEST(test_vertical_layout_clear);
    RUN_TEST(test_vertical_layout_get_element);

    // UI Layout Advanced Tests (18 tests)
    RUN_TEST(test_horizontal_layout_spacing_calculation);
    RUN_TEST(test_horizontal_layout_zero_spacing);
    RUN_TEST(test_vertical_layout_spacing_calculation);
    RUN_TEST(test_vertical_layout_zero_spacing);
    RUN_TEST(test_button_state_transitions_advanced);
    RUN_TEST(test_checkbox_state_transitions_advanced);
    RUN_TEST(test_checkbox_toggle_advanced);
    RUN_TEST(test_checkbox_toggle_disabled);
    RUN_TEST(test_parent_child_relationship);
    RUN_TEST(test_panel_position_update);
    RUN_TEST(test_nested_container_hierarchy);
    RUN_TEST(test_sibling_order_in_horizontal_layout);
    RUN_TEST(test_sibling_order_in_vertical_layout);
    RUN_TEST(test_element_index_tracking);
    RUN_TEST(test_element_removal_from_hierarchy);
    RUN_TEST(test_selected_index_navigation);
    RUN_TEST(test_visibility_affects_rendering);
    RUN_TEST(test_layout_element_count);
    RUN_TEST(test_layout_clear_elements);
    // New draw/update/handleInput coverage tests
    RUN_TEST(test_grid_layout_draw);
    RUN_TEST(test_grid_layout_update);
    RUN_TEST(test_horizontal_layout_draw);
    RUN_TEST(test_horizontal_layout_update);
    RUN_TEST(test_vertical_layout_draw);
    RUN_TEST(test_vertical_layout_update);
    RUN_TEST(test_grid_layout_handle_input);
    RUN_TEST(test_horizontal_layout_handle_input);
    RUN_TEST(test_vertical_layout_handle_input);

    // Additional UIGridLayout Coverage Tests
    RUN_TEST(test_grid_layout_calculate_rows_basic);
    RUN_TEST(test_grid_layout_calculate_rows_single_element);
    RUN_TEST(test_grid_layout_calculate_rows_empty);
    RUN_TEST(test_grid_layout_set_selected_index);
    RUN_TEST(test_grid_layout_set_selected_index_invalid);
    RUN_TEST(test_grid_layout_get_selected_element);
    RUN_TEST(test_grid_layout_get_selected_element_invalid);
    RUN_TEST(test_grid_layout_add_duplicate_element);
    RUN_TEST(test_grid_layout_set_button_style);
    RUN_TEST(test_grid_layout_set_selected_index_negative);
    RUN_TEST(test_grid_layout_remove_selected_element);

    // Additional UIHorizontalLayout Coverage Tests
    RUN_TEST(test_horizontal_layout_set_selected_index);
    RUN_TEST(test_horizontal_layout_set_selected_index_negative);
    RUN_TEST(test_horizontal_layout_get_selected_element);
    RUN_TEST(test_horizontal_layout_remove_selected);
    RUN_TEST(test_horizontal_layout_scroll_offset);
    RUN_TEST(test_horizontal_layout_scroll_to_selected);

    // Additional UIVerticalLayout Coverage Tests
    RUN_TEST(test_vertical_layout_set_selected_index);
    RUN_TEST(test_vertical_layout_set_selected_index_negative);
    RUN_TEST(test_vertical_layout_get_selected_element);
    RUN_TEST(test_vertical_layout_remove_selected);

    // More UI layout handleInput/ensureVisible tests
    RUN_TEST(test_horizontal_layout_handle_input_with_selection);
    RUN_TEST(test_vertical_layout_handle_input_with_selection);
    RUN_TEST(test_grid_layout_handle_input_with_selection);
    RUN_TEST(test_horizontal_layout_ensure_visible);
    RUN_TEST(test_vertical_layout_ensure_visible);

    // Protected methods tests (via test subclasses)
    RUN_TEST(test_vertical_layout_protected_calculate_content_height);
    RUN_TEST(test_vertical_layout_protected_update_element_visibility);
    RUN_TEST(test_vertical_layout_protected_clamp_scroll);
    RUN_TEST(test_horizontal_layout_protected_calculate_content_width);
    RUN_TEST(test_horizontal_layout_protected_update_element_visibility);
    RUN_TEST(test_horizontal_layout_protected_clamp_scroll);
    RUN_TEST(test_grid_layout_protected_calculate_rows);
    RUN_TEST(test_grid_layout_protected_calculate_cell_dimensions);

    // UI Padding Tests (13 tests)
    RUN_TEST(test_padding_container_initialization);
    RUN_TEST(test_padding_container_vector2_constructor);
    RUN_TEST(test_padding_container_set_child);
    RUN_TEST(test_padding_container_no_child);
    RUN_TEST(test_padding_container_uniform_padding);
    RUN_TEST(test_padding_container_asymmetric_padding);
    RUN_TEST(test_padding_container_zero_padding);
    RUN_TEST(test_padding_container_change_padding);
    RUN_TEST(test_padding_container_with_panel_child);
    RUN_TEST(test_padding_container_position_update);
    RUN_TEST(test_padding_container_different_padding_values);
    RUN_TEST(test_padding_container_large_padding);
    RUN_TEST(test_padding_container_replace_child);
    RUN_TEST(test_padding_container_child_null_after_clear);
    RUN_TEST(test_padding_container_update_with_child);
    RUN_TEST(test_padding_container_update_disabled);
    RUN_TEST(test_padding_container_draw_with_child);
    RUN_TEST(test_padding_container_draw_not_visible);

    return UNITY_END();
}