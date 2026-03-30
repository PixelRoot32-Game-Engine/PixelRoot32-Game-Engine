#pragma once

#include <unity.h>
#include "../../test_config.h"
#include "graphics/ui/UIVerticalLayout.h"
#include "graphics/ui/UIHorizontalLayout.h"
#include "graphics/ui/UIGridLayout.h"
#include "graphics/ui/UILabel.h"
#include "graphics/ui/UIButton.h"

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;

void test_vertical_layout_spacing() {
    UIVerticalLayout layout(0, 0, 100, 200);
    TEST_ASSERT_EQUAL(4, static_cast<int>(layout.getSpacing()));
    layout.setSpacing(toScalar(10));
    TEST_ASSERT_EQUAL(10, static_cast<int>(layout.getSpacing()));
}

void test_horizontal_layout_spacing() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    TEST_ASSERT_EQUAL(4, static_cast<int>(layout.getSpacing()));
    layout.setSpacing(toScalar(8));
    TEST_ASSERT_EQUAL(8, static_cast<int>(layout.getSpacing()));
}

void test_grid_layout_columns() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    TEST_ASSERT_EQUAL(2, layout.getColumns());
    TEST_ASSERT_EQUAL(-1, layout.getSelectedIndex());
}

void test_vertical_layout_elements() {
    UIVerticalLayout layout(0, 0, 100, 200);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    UILabel l2("B", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    TEST_ASSERT_EQUAL(2, static_cast<int>(layout.getElementCount()));
}

void test_vertical_layout_padding() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setPadding(toScalar(10));
    TEST_ASSERT_EQUAL(10, static_cast<int>(layout.getPadding()));
}

void test_horizontal_layout_navigation() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setNavigationButtons(0, 1);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL(0, layout.getSelectedIndex());
}

void test_horizontal_layout_scroll() {
    UIHorizontalLayout layout(0, 0, 100, 50);
    layout.setScrollEnabled(true);
    TEST_ASSERT_TRUE(layout.isScrollingEnabled());
}

void test_grid_layout_selection() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    UILabel l2("B", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL(0, layout.getSelectedIndex());
}

void test_grid_layout_navigation_buttons() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setNavigationButtons(0, 1, 2, 3);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    UILabel l3("3", Vector2::ZERO(), Color::White, 1);
    UILabel l4("4", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    layout.addElement(&l4);
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL(0, layout.getSelectedIndex());
    TEST_ASSERT_EQUAL(&l1, layout.getSelectedElement());
}

void test_grid_layout_rows() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    UILabel l2("B", Vector2::ZERO(), Color::White, 1);
    UILabel l3("C", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    TEST_ASSERT_EQUAL(2, layout.getRows());
}

void test_grid_layout_button_style() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setButtonStyle(Color::White, Color::Blue, Color::Gray, Color::Black);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
}

void test_grid_layout_change_columns() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(1);
    TEST_ASSERT_EQUAL(1, layout.getColumns());
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    UILabel l2("B", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.setColumns(2);
    TEST_ASSERT_EQUAL(2, layout.getColumns());
}

void test_grid_layout_three_columns() {
    UIGridLayout layout(0, 0, 120, 80);
    layout.setColumns(3);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    UILabel l3("3", Vector2::ZERO(), Color::White, 1);
    UILabel l4("4", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    layout.addElement(&l4);
    TEST_ASSERT_EQUAL(3, layout.getColumns());
    TEST_ASSERT_EQUAL(2, layout.getRows());
}

// =============================================================================
// Additional Layout Coverage Tests
// =============================================================================

// UIGridLayout additional tests
void test_grid_layout_constructor_vector2() {
    UIGridLayout layout(Vector2(10, 20), 100, 100);
    TEST_ASSERT_EQUAL(1, layout.getColumns());
}

void test_grid_layout_remove_element() {
    UIGridLayout layout(0, 0, 100, 100);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    UILabel l2("B", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    TEST_ASSERT_EQUAL(2, static_cast<int>(layout.getElementCount()));
    
    layout.removeElement(&l1);
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
}

void test_grid_layout_zero_columns() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(0);
    TEST_ASSERT_EQUAL(1, layout.getColumns()); // Should clamp to 1
}

void test_grid_layout_calculate_rows_zero_columns() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(0);
    TEST_ASSERT_EQUAL(0, layout.getRows());
}

void test_grid_layout_clear_elements() {
    UIGridLayout layout(0, 0, 100, 100);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.clearElements();
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

void test_grid_layout_get_element() {
    UIGridLayout layout(0, 0, 100, 100);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    TEST_ASSERT_NOT_NULL(layout.getElement(0));
    TEST_ASSERT_NULL(layout.getElement(5));
}

// UIHorizontalLayout additional tests
void test_horizontal_layout_constructor_scalars() {
    UIHorizontalLayout layout(toScalar(10), toScalar(20), 200, 50);
    TEST_ASSERT_EQUAL(4, static_cast<int>(layout.getSpacing()));
}

void test_horizontal_layout_add_remove_elements() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
    
    layout.removeElement(&l1);
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

void test_horizontal_layout_clear() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.clearElements();
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

void test_horizontal_layout_get_element() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    TEST_ASSERT_NOT_NULL(layout.getElement(0));
}

void test_horizontal_layout_set_scroll_disabled() {
    UIHorizontalLayout layout(0, 0, 100, 50);
    layout.setScrollEnabled(false);
    TEST_ASSERT_FALSE(layout.isScrollingEnabled());
}

// UIVerticalLayout additional tests
void test_vertical_layout_constructor_scalars() {
    UIVerticalLayout layout(toScalar(10), toScalar(20), 100, 200);
    TEST_ASSERT_EQUAL(4, static_cast<int>(layout.getSpacing()));
}

void test_vertical_layout_add_remove_elements() {
    UIVerticalLayout layout(0, 0, 100, 200);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
    
    layout.removeElement(&l1);
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

void test_vertical_layout_clear() {
    UIVerticalLayout layout(0, 0, 100, 200);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.clearElements();
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

void test_vertical_layout_get_element() {
    UIVerticalLayout layout(0, 0, 100, 200);
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    TEST_ASSERT_NOT_NULL(layout.getElement(0));
}
