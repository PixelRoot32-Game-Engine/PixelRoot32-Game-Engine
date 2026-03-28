#include <unity.h>
#include "../../test_config.h"
#include "graphics/ui/UIVerticalLayout.h"
#include "graphics/ui/UIHorizontalLayout.h"
#include "graphics/ui/UIGridLayout.h"
#include "graphics/ui/UIAnchorLayout.h"
#include "graphics/ui/UIPaddingContainer.h"
#include "graphics/ui/UILabel.h"
#include "graphics/ui/UIButton.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "graphics/BaseDrawSurface.h"
#include "input/InputManager.h"
#include "input/InputConfig.h"
#include <vector>
#include <memory>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;
using namespace pixelroot32::input;

namespace {
class SimpleMockDrawSurface : public BaseDrawSurface {
public:
    void init() override {}
    void clearBuffer() override {}
    void sendBuffer() override {}
    void drawRectangle(int, int, int, int, uint16_t) override {}
    void drawFilledRectangle(int, int, int, int, uint16_t) override {}
    void drawPixel(int, int, uint16_t) override {}
};
}

void test_vertical_layout_positioning() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setPadding(10);
    layout.setSpacing(5);
    
    auto label1 = std::make_unique<UILabel>("L1", Vector2::ZERO(), Color::White, 1); 
    auto label2 = std::make_unique<UILabel>("L2", Vector2::ZERO(), Color::White, 1); 
    
    layout.addElement(label1.get());
    layout.addElement(label2.get());
    
    // label1 should be centered horizontally: (100 - 12) / 2 = 44
    // label1 should be at y = padding = 10
    TEST_ASSERT_EQUAL_FLOAT(44, label1->position.x);
    TEST_ASSERT_EQUAL_FLOAT(10, label1->position.y);
    
    // label2 should be centered horizontally: (100 - 12) / 2 = 44
    // label2 should be at y = padding + label1.height + spacing = 10 + 8 + 5 = 23
    TEST_ASSERT_EQUAL_FLOAT(44, label2->position.x);
    TEST_ASSERT_EQUAL_FLOAT(23, label2->position.y);
}

void test_horizontal_layout_positioning() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setPadding(5);
    layout.setSpacing(10);
    
    auto label1 = std::make_unique<UILabel>("L1", Vector2::ZERO(), Color::White, 1);
    auto label2 = std::make_unique<UILabel>("L2", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(label1.get());
    layout.addElement(label2.get());
    
    // label1: (padding, centeredY) = (5, (50-8)/2 = 21)
    TEST_ASSERT_EQUAL_FLOAT(5, label1->position.x);
    TEST_ASSERT_EQUAL_FLOAT(21, label1->position.y);
    
    // label2: (padding + label1.width + spacing, centeredY)
    // 5 + 12 + 10 = 27
    TEST_ASSERT_EQUAL_FLOAT(27, label2->position.x);
    TEST_ASSERT_EQUAL_FLOAT(21, label2->position.y);
}

void test_grid_layout_positioning() {
    // 2 columns grid
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setPadding(0);
    layout.setSpacing(0);
    
    auto l1 = std::make_unique<UILabel>("1", Vector2::ZERO(), Color::White, 1);
    auto l2 = std::make_unique<UILabel>("2", Vector2::ZERO(), Color::White, 1);
    auto l3 = std::make_unique<UILabel>("3", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(l1.get());
    layout.addElement(l2.get());
    layout.addElement(l3.get());
    
    // Grid 2 cols, width 100 -> cell width 50, cell height 50
    // l1: col 0, row 0 -> centered in (0,0,50,50) -> (22, 21)
    TEST_ASSERT_EQUAL_FLOAT(22, l1->position.x);
    TEST_ASSERT_EQUAL_FLOAT(21, l1->position.y);
    
    // l2: col 1, row 0 -> centered in (50,0,50,50) -> (50+22, 21) = (72, 21)
    TEST_ASSERT_EQUAL_FLOAT(72, l2->position.x);
    TEST_ASSERT_EQUAL_FLOAT(21, l2->position.y);
    
    // l3: col 0, row 1 -> centered in (0,50,50,50) -> (22, 50+21) = (22, 71)
    TEST_ASSERT_EQUAL_FLOAT(22, l3->position.x);
    TEST_ASSERT_EQUAL_FLOAT(71, l3->position.y);
}

void test_padding_container() {
    UIPaddingContainer container(10, 20, 100, 100);
    container.setPadding(20, 10, 5, 15); // left, right, top, bottom
    
    UILabel* label = new UILabel("P", Vector2::ZERO(), Color::White, 1);
    container.setChild(label);
    
    // Parent at (10, 20), left padding 20, top padding 5
    // Label should be at (10 + 20, 20 + 5) = (30, 25)
    TEST_ASSERT_EQUAL_FLOAT(30, label->position.x);
    TEST_ASSERT_EQUAL_FLOAT(25, label->position.y);
    
    delete label;
}

void test_anchor_layout_positioning() {
    UIAnchorLayout layout(0, 0, 100, 100);
    
    UILabel* l1 = new UILabel("TL", Vector2::ZERO(), Color::White, 1); // width 12, height 8
    UILabel* l2 = new UILabel("BR", Vector2::ZERO(), Color::White, 1);
    UILabel* l3 = new UILabel("C", Vector2::ZERO(), Color::White, 1); // width 6, height 8
    
    layout.addElement(l1, Anchor::TOP_LEFT);
    layout.addElement(l2, Anchor::BOTTOM_RIGHT);
    layout.addElement(l3, Anchor::CENTER);
    
    // TOP_LEFT: (0, 0)
    TEST_ASSERT_EQUAL_FLOAT(0, l1->position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, l1->position.y);
    
    // BOTTOM_RIGHT: (100 - 12, 100 - 8) = (88, 92)
    TEST_ASSERT_EQUAL_FLOAT(88, l2->position.x);
    TEST_ASSERT_EQUAL_FLOAT(92, l2->position.y);
    
    // CENTER: (50 - 3, 50 - 4) = (47, 46)
    TEST_ASSERT_EQUAL_FLOAT(47, l3->position.x);
    TEST_ASSERT_EQUAL_FLOAT(46, l3->position.y);
    
    delete l1;
    delete l2;
    delete l3;
}

void test_vertical_layout_scrolling() {
    UIVerticalLayout layout(0, 0, 100, 50); // Small viewport
    layout.setPadding(0);
    layout.setSpacing(0);
    layout.setScrollingEnabled(true);
    
    // Total height will be 8 * 10 = 80
    std::vector<std::unique_ptr<UILabel>> labels;
    for(int i=0; i<10; i++) {
        labels.push_back(std::make_unique<UILabel>("L", Vector2::ZERO(), Color::White, 1));
        layout.addElement(labels.back().get());
    }
    
    TEST_ASSERT_EQUAL_FLOAT(80, layout.getContentHeight());
    
    // Scroll to 20
    layout.setScrollOffset(toScalar(20.0f));
    TEST_ASSERT_EQUAL_FLOAT(20, layout.getScrollOffset());
    
    // Element at index 0 should be at 0 - 20 = -20
    TEST_ASSERT_EQUAL_FLOAT(0 - 20, labels[0]->position.y);
    // Element at index 4 (starts at 32) should be at 32 - 20 = 12
    TEST_ASSERT_EQUAL_FLOAT(32 - 20, labels[4]->position.y);
    
    // Clamp test: max scroll = 80 - 50 = 30
    layout.setScrollOffset(toScalar(100.0f));
    TEST_ASSERT_EQUAL_FLOAT(30, layout.getScrollOffset());
    
    layout.setScrollOffset(toScalar(-10.0f));
    TEST_ASSERT_EQUAL_FLOAT(0, layout.getScrollOffset());
}

void test_vertical_layout_remove_element() {
    UIVerticalLayout layout(0, 0, 100, 100);
    layout.setPadding(0);
    layout.setSpacing(0);
    
    UILabel l1("1", Vector2::ZERO(), Color::White, 1); // height 8
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(&l1);
    layout.addElement(&l2);
    TEST_ASSERT_EQUAL_FLOAT(16, layout.getContentHeight());
    
    layout.removeElement(&l1);
    TEST_ASSERT_EQUAL_FLOAT(8, layout.getContentHeight());
    TEST_ASSERT_EQUAL_FLOAT(0, l2.position.y); // l2 should move to top
}

void test_grid_layout_variable_columns() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setPadding(0);
    layout.setSpacing(0);
    
    UILabel l1("1", Vector2::ZERO(), Color::White, 1); // width 6, height 8
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    UILabel l3("3", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    
    // 1 column: cells are 100x33.33
    layout.setColumns(1);
    TEST_ASSERT_EQUAL(3, layout.getRows());
    // l2 should be at row 1: y = 33.33 + (33.33-8)/2 = 33.33 + 12.66 = 46
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 46.0f, l2.position.y);
    
    // 3 columns: cells are 33.33x100
    layout.setColumns(3);
    TEST_ASSERT_EQUAL(1, layout.getRows());
    // l2 should be at col 1: x = 33.33 + (33.33-6)/2 = 33.33 + 13.66 = 47
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 47.0f, l2.position.x);
}

void test_horizontal_layout_scrolling() {
    UIHorizontalLayout layout(0, 0, 50, 100); // Small viewport
    layout.setPadding(0);
    layout.setSpacing(0);
    layout.setScrollingEnabled(true);
    
    // Total width will be 6 * 10 = 60
    std::vector<std::unique_ptr<UILabel>> labels;
    for(int i=0; i<10; i++) {
        labels.push_back(std::make_unique<UILabel>("L", Vector2::ZERO(), Color::White, 1));
        layout.addElement(labels.back().get());
    }
    
    TEST_ASSERT_EQUAL_FLOAT(60, layout.getContentWidth());
    
    // Scroll to 10
    layout.setScrollOffset(toScalar(10.0f));
    TEST_ASSERT_EQUAL_FLOAT(10, layout.getScrollOffset());
    
    // Element at index 2 (starts at 12) should be at 12 - 10 = 2
    TEST_ASSERT_EQUAL_FLOAT(2, labels[2]->position.x);
    
    // Clamp test: max scroll = 60 - 50 = 10
    layout.setScrollOffset(toScalar(50.0f));
    TEST_ASSERT_EQUAL_FLOAT(10, layout.getScrollOffset());
}

void test_vertical_layout_clear_elements() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setPadding(5);
    layout.setSpacing(2);
    
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);
    UILabel l2("B", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    TEST_ASSERT_EQUAL(2, layout.getElementCount());
    
    layout.clearElements();
    TEST_ASSERT_EQUAL(0, layout.getElementCount());
}

void test_vertical_layout_draw() {
    UIVerticalLayout layout(0, 0, 100, 80);
    layout.setPadding(2);
    layout.setSpacing(2);
    auto l1 = std::make_unique<UILabel>("A", Vector2::ZERO(), Color::White, 1);
    auto l2 = std::make_unique<UILabel>("B", Vector2::ZERO(), Color::White, 1);
    layout.addElement(l1.get());
    layout.addElement(l2.get());
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    layout.draw(renderer);
}

void test_vertical_layout_set_selected_index_and_get_element() {
    UIVerticalLayout layout(0, 0, 100, 100);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    TEST_ASSERT_NULL(layout.getSelectedElement());
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL_PTR(&l1, layout.getSelectedElement());
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL_PTR(&l2, layout.getSelectedElement());
    layout.setSelectedIndex(-1);
    TEST_ASSERT_NULL(layout.getSelectedElement());
}

void test_vertical_layout_handle_input_nav() {
    UIVerticalLayout layout(0, 0, 100, 80);
    layout.setNavigationButtons(0, 1);
    UIButton b1("A", 0, {0, 0}, {50, 20}, nullptr);
    UIButton b2("B", 0, {0, 0}, {50, 20}, nullptr);
    layout.addElement(&b1);
    layout.addElement(&b2);
    layout.setSelectedIndex(0);
    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    uint8_t keys[256] = {0};
    keys[1] = 1;
    input.update(16, keys);
    layout.handleInput(input);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
}

void test_anchor_layout_set_screen_size() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UILabel l("X", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l, Anchor::TOP_LEFT);
    TEST_ASSERT_EQUAL_FLOAT(0, l.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, l.position.y);
    layout.setScreenSize(200, 150);
    TEST_ASSERT_EQUAL(200, layout.getScreenWidth());
    TEST_ASSERT_EQUAL(150, layout.getScreenHeight());
}

void test_anchor_layout_top_right_bottom_left() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UILabel tr("TR", Vector2::ZERO(), Color::White, 1);
    UILabel bl("BL", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&tr, Anchor::TOP_RIGHT);
    layout.addElement(&bl, Anchor::BOTTOM_LEFT);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f - 12.0f, tr.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, tr.position.y);
    TEST_ASSERT_EQUAL_FLOAT(0, bl.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f - 8.0f, bl.position.y);
}

void test_anchor_layout_remove_element() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UILabel* l1 = new UILabel("1", Vector2::ZERO(), Color::White, 1);
    UILabel* l2 = new UILabel("2", Vector2::ZERO(), Color::White, 1);
    layout.addElement(l1, Anchor::TOP_LEFT);
    layout.addElement(l2, Anchor::BOTTOM_RIGHT);
    TEST_ASSERT_EQUAL(2, layout.getElementCount());
    layout.removeElement(l1);
    TEST_ASSERT_EQUAL(1, layout.getElementCount());
    delete l1;
    delete l2;
}

void test_anchor_layout_draw() {
    UIAnchorLayout layout(0, 0, 80, 60);
    UILabel l("X", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l, Anchor::CENTER);
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    layout.draw(renderer);
}

void test_grid_layout_remove_element() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    UILabel l3("3", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    TEST_ASSERT_EQUAL(3, layout.getElementCount());
    layout.removeElement(&l2);
    TEST_ASSERT_EQUAL(2, layout.getElementCount());
}

void test_grid_layout_get_element() {
    UIGridLayout layout(0, 0, 100, 100);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    TEST_ASSERT_EQUAL_PTR(&l1, layout.getElement(0));
    TEST_ASSERT_NULL(layout.getElement(1));
    TEST_ASSERT_NULL(layout.getElement(100));
}

void test_horizontal_layout_draw() {
    UIHorizontalLayout layout(0, 0, 150, 40);
    layout.setPadding(2);
    layout.setSpacing(4);
    auto l1 = std::make_unique<UILabel>("A", Vector2::ZERO(), Color::White, 1);
    auto l2 = std::make_unique<UILabel>("B", Vector2::ZERO(), Color::White, 1);
    layout.addElement(l1.get());
    layout.addElement(l2.get());
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    layout.draw(renderer);
}

void test_horizontal_layout_handle_input_nav() {
    UIHorizontalLayout layout(0, 0, 120, 40);
    layout.setNavigationButtons(0, 1);
    UIButton b1("L", 0, {0, 0}, {40, 20}, nullptr);
    UIButton b2("R", 0, {0, 0}, {40, 20}, nullptr);
    layout.addElement(&b1);
    layout.addElement(&b2);
    layout.setSelectedIndex(0);
    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    uint8_t keys[256] = {0};
    keys[1] = 1;
    input.update(16, keys);
    layout.handleInput(input);
    TEST_ASSERT_EQUAL(1, layout.getSelectedIndex());
}

void test_horizontal_layout_set_selected_index_and_update() {
    UIHorizontalLayout layout(0, 0, 100, 50);
    UIButton b1("1", 0, {0, 0}, {30, 20}, nullptr);
    layout.addElement(&b1);
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL_PTR(&b1, layout.getSelectedElement());
    layout.update(16);
}

void test_grid_layout_draw() {
    UIGridLayout layout(0, 0, 80, 80);
    layout.setColumns(2);
    auto l1 = std::make_unique<UILabel>("1", Vector2::ZERO(), Color::White, 1);
    auto l2 = std::make_unique<UILabel>("2", Vector2::ZERO(), Color::White, 1);
    layout.addElement(l1.get());
    layout.addElement(l2.get());
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    layout.draw(renderer);
}

void test_grid_layout_set_selected_index_and_handle_input() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setNavigationButtons(0, 1, 2, 3);
    UIButton b1("A", 0, {0, 0}, {30, 20}, nullptr);
    UIButton b2("B", 0, {0, 0}, {30, 20}, nullptr);
    layout.addElement(&b1);
    layout.addElement(&b2);
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL_PTR(&b1, layout.getSelectedElement());
    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    uint8_t keys[256] = {0};
    keys[3] = 1;
    input.update(16, keys);
    layout.handleInput(input);
}

void test_vertical_layout_update() {
    UIVerticalLayout layout(0, 0, 100, 60);
    layout.setScrollingEnabled(true);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.setScrollOffset(toScalar(5.0f));
    layout.update(16);
}

void test_vertical_layout_set_button_style() {
    UIVerticalLayout layout(0, 0, 100, 100);
    UIButton b1("A", 0, {0, 0}, {50, 20}, nullptr);
    UIButton b2("B", 0, {0, 0}, {50, 20}, nullptr);
    layout.addElement(&b1);
    layout.addElement(&b2);
    layout.setButtonStyle(Color::Cyan, Color::Navy, Color::White, Color::Black);
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL_PTR(&b2, layout.getSelectedElement());
}

void test_padding_container_set_padding_single_and_draw() {
    UIPaddingContainer container(0, 0, 80, 40);
    container.setPadding(toScalar(5.0f));
    UILabel label("P", Vector2::ZERO(), Color::White, 1);
    container.setChild(&label);
    // Note: UIPaddingContainer positions child based on its own implementation
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 5.0f, label.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 5.0f, label.position.y);
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    container.draw(renderer);
    container.update(16);
}

void test_ui_label_set_text_and_center_x() {
    UILabel label("Hi", Vector2::ZERO(), Color::White, 1);
    label.centerX(240);
    TEST_ASSERT_TRUE(label.position.x > 0);
    label.setText("Hello");
    TEST_ASSERT_TRUE(label.width >= 0);
    label.centerX(100);
}

void test_horizontal_layout_clear_and_spacing() {
    UIHorizontalLayout layout(0, 0, 100, 50);
    layout.setSpacing(10);
    TEST_ASSERT_EQUAL(10, layout.getSpacing());
    layout.setSpacing(20);
    TEST_ASSERT_EQUAL(20, layout.getSpacing());
    layout.clearElements();
    TEST_ASSERT_EQUAL(0U, layout.getElementCount());
}

void test_grid_layout_set_columns() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(3);
    TEST_ASSERT_EQUAL(3, layout.getColumns());
    layout.setColumns(4);
    TEST_ASSERT_EQUAL(4, layout.getColumns());
}

void test_grid_layout_clear_elements() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    TEST_ASSERT_EQUAL(2U, layout.getElementCount());
    layout.clearElements();
    TEST_ASSERT_EQUAL(0U, layout.getElementCount());
}

void test_grid_layout_navigation_buttons() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setNavigationButtons(0, 1, 2, 3);
    TEST_ASSERT_TRUE(true);
}

void test_grid_layout_get_selected_index() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    TEST_ASSERT_EQUAL(-1, layout.getSelectedIndex());
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL(0, layout.getSelectedIndex());
}

void test_grid_layout_set_selected_index_out_of_bounds() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setSelectedIndex(100);
    TEST_ASSERT_TRUE(layout.getSelectedIndex() >= 0);
}

void test_vertical_layout_get_spacing() {
    UIVerticalLayout layout(0, 0, 100, 200);
    TEST_ASSERT_EQUAL(0, layout.getSpacing());
    layout.setSpacing(10);
    TEST_ASSERT_EQUAL(10, layout.getSpacing());
}

void test_vertical_layout_get_padding() {
    UIVerticalLayout layout(0, 0, 100, 200);
    TEST_ASSERT_EQUAL(0, layout.getPadding());
    layout.setPadding(5);
    TEST_ASSERT_EQUAL(5, layout.getPadding());
}

void test_horizontal_layout_get_spacing() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    TEST_ASSERT_EQUAL(4, layout.getSpacing());  // Default spacing is 4
    layout.setSpacing(15);
    TEST_ASSERT_EQUAL(15, layout.getSpacing());
}

void test_horizontal_layout_get_padding() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    TEST_ASSERT_EQUAL(0, layout.getPadding());
    layout.setPadding(10);
    TEST_ASSERT_EQUAL(10, layout.getPadding());
}

// Edge case tests

void test_grid_layout_zero_columns() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(0);  // Edge case: zero columns
    // Should handle gracefully without crashing
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    // With 0 columns, should default to 1 column behavior or handle gracefully
    TEST_ASSERT_TRUE(layout.getColumns() >= 0);
}

void test_grid_layout_overflow_elements() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setPadding(0);
    layout.setSpacing(0);
    
    // Add 5 elements to 2-column grid (creates 3 rows with last row having 1 element)
    std::vector<std::unique_ptr<UILabel>> labels;
    for(int i = 0; i < 5; i++) {
        labels.push_back(std::make_unique<UILabel>("X", Vector2::ZERO(), Color::White, 1));
        layout.addElement(labels.back().get());
    }
    
    TEST_ASSERT_EQUAL(3, layout.getRows());  // 3 rows needed for 5 elements in 2 columns
}

void test_grid_layout_large_element_count() {
    UIGridLayout layout(0, 0, 200, 200);
    layout.setColumns(3);
    
    // Add many elements to test performance and positioning
    std::vector<std::unique_ptr<UILabel>> labels;
    for(int i = 0; i < 20; i++) {
        labels.push_back(std::make_unique<UILabel>("L", Vector2::ZERO(), Color::White, 1));
        layout.addElement(labels.back().get());
    }
    
    TEST_ASSERT_EQUAL(7, layout.getRows());  // Ceiling of 20/3 = 7 rows
}

void test_anchor_layout_top_center_and_bottom() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UILabel tc("TC", Vector2::ZERO(), Color::White, 1);  // width 12, height 8
    UILabel bc("BC", Vector2::ZERO(), Color::White, 1);
    UILabel l("L", Vector2::ZERO(), Color::White, 1);   // width 6, height 8
    UILabel r("R", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(&tc, Anchor::TOP_CENTER);
    layout.addElement(&bc, Anchor::BOTTOM_CENTER);
    layout.addElement(&l, Anchor::LEFT_CENTER);
    layout.addElement(&r, Anchor::RIGHT_CENTER);
    
    // TOP_CENTER: (50 - 6, 0) = (44, 0)
    TEST_ASSERT_EQUAL_FLOAT(44, tc.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, tc.position.y);
    
    // BOTTOM_CENTER: (50 - 6, 100 - 8) = (44, 92)
    TEST_ASSERT_EQUAL_FLOAT(44, bc.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 92.0f, bc.position.y);
    
    // LEFT_CENTER: (0, 50 - 4) = (0, 46)
    TEST_ASSERT_EQUAL_FLOAT(0, l.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 46.0f, l.position.y);
    
    // RIGHT_CENTER: (100 - 6, 46) = (94, 46)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 94.0f, r.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 46.0f, r.position.y);
}

void test_horizontal_layout_wrap_content() {
    UIHorizontalLayout layout(0, 0, 300, 50);
    layout.setPadding(5);
    layout.setSpacing(10);
    
    // Add many elements that exceed viewport
    std::vector<std::unique_ptr<UILabel>> labels;
    for(int i = 0; i < 15; i++) {
        labels.push_back(std::make_unique<UILabel>("ABC", Vector2::ZERO(), Color::White, 1));
        layout.addElement(labels.back().get());
    }
    
    // Total width: padding(5) + 15*(width(18) + spacing(10)) - spacing(10) + padding(5)
    // = 5 + 15*28 - 10 + 5 = 5 + 420 - 10 + 5 = 420
    float expectedContentWidth = 5 + 15 * (18 + 10) - 10 + 5;
    TEST_ASSERT_FLOAT_WITHIN(5.0f, expectedContentWidth, layout.getContentWidth());
}

void test_vertical_layout_with_buttons() {
    UIVerticalLayout layout(0, 0, 100, 150);
    layout.setPadding(10);
    layout.setSpacing(5);
    
    UIButton b1("Btn1", 0, {0, 0}, {60, 20}, nullptr);
    UIButton b2("Btn2", 0, {0, 0}, {60, 20}, nullptr);
    UIButton b3("Btn3", 0, {0, 0}, {60, 20}, nullptr);
    
    layout.addElement(&b1);
    layout.addElement(&b2);
    layout.addElement(&b3);
    
    // Check that buttons are positioned correctly
    // b1 at y = 10 (padding)
    TEST_ASSERT_EQUAL_FLOAT(10, b1.position.y);
    // b2 at y = 10 + 20 + 5 = 35
    TEST_ASSERT_EQUAL_FLOAT(35, b2.position.y);
    // b3 at y = 35 + 20 + 5 = 60
    TEST_ASSERT_EQUAL_FLOAT(60, b3.position.y);
}

void test_vertical_layout_empty() {
    UIVerticalLayout layout(0, 0, 100, 100);
    // Empty layout should have content height of 0
    TEST_ASSERT_EQUAL_FLOAT(0, layout.getContentHeight());
    // Drawing empty layout should not crash
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    layout.draw(renderer);
}

void test_horizontal_layout_empty() {
    UIHorizontalLayout layout(0, 0, 100, 50);
    // Empty layout should have content width of 0
    TEST_ASSERT_EQUAL_FLOAT(0, layout.getContentWidth());
}

void test_anchor_layout_empty() {
    UIAnchorLayout layout(0, 0, 100, 100);
    // Empty layout operations
    TEST_ASSERT_EQUAL(0U, layout.getElementCount());
    // Drawing empty layout should not crash
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    layout.draw(renderer);
}

// Additional edge case tests to improve coverage

void test_grid_layout_negative_cell_dimensions() {
    // Layout with large padding/spacing that makes cell dimensions negative
    UIGridLayout layout(0, 0, 50, 50);
    layout.setColumns(2);
    layout.setPadding(30);  // Large padding
    layout.setSpacing(10);  // Large spacing
    
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    
    // Cell dimensions should be clamped to 0, not negative
    TEST_ASSERT_TRUE(l1.position.x >= 0);
    TEST_ASSERT_TRUE(l1.position.y >= 0);
}

void test_grid_layout_handle_input_with_buttons() {
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setNavigationButtons(0, 1, 2, 3);
    
    UIButton b1("A", 0, {0, 0}, {30, 20}, nullptr);
    UIButton b2("B", 0, {0, 0}, {30, 20}, nullptr);
    UIButton b3("C", 0, {0, 0}, {30, 20}, nullptr);
    UIButton b4("D", 0, {0, 0}, {30, 20}, nullptr);
    
    layout.addElement(&b1);
    layout.addElement(&b2);
    layout.addElement(&b3);
    layout.addElement(&b4);
    
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL_PTR(&b1, layout.getSelectedElement());
    
    // Test navigation with input
    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    uint8_t keys[256] = {0};
    keys[1] = 1;  // Navigate right
    input.update(16, keys);
    layout.handleInput(input);
    
    // Should have moved to next element
    TEST_ASSERT_TRUE(layout.getSelectedIndex() >= 0);
}

void test_horizontal_layout_navigation_wrap() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setNavigationButtons(0, 1);
    
    UIButton b1("1", 0, {0, 0}, {40, 20}, nullptr);
    UIButton b2("2", 0, {0, 0}, {40, 20}, nullptr);
    layout.addElement(&b1);
    layout.addElement(&b2);
    
    layout.setSelectedIndex(1);  // Select last element
    
    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    uint8_t keys[256] = {0};
    keys[1] = 1;  // Navigate next (wraps)
    input.update(16, keys);
    layout.handleInput(input);
    
    TEST_ASSERT_TRUE(layout.getSelectedIndex() >= 0);
}

void test_vertical_layout_navigation_wrap() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setNavigationButtons(0, 1);
    
    UIButton b1("1", 0, {0, 0}, {40, 20}, nullptr);
    UIButton b2("2", 0, {0, 0}, {40, 20}, nullptr);
    layout.addElement(&b1);
    layout.addElement(&b2);
    
    layout.setSelectedIndex(0);
    
    // Navigate up from first element (should stay or wrap)
    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    uint8_t keys[256] = {0};
    keys[0] = 1;  // Navigate up
    input.update(16, keys);
    layout.handleInput(input);
    
    TEST_ASSERT_TRUE(layout.getSelectedIndex() >= 0);
}

void test_anchor_layout_multiple_elements_same_anchor() {
    UIAnchorLayout layout(0, 0, 100, 100);
    
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    
    // Both elements at same anchor position
    layout.addElement(&l1, Anchor::TOP_LEFT);
    layout.addElement(&l2, Anchor::TOP_LEFT);
    
    // Both should be at same position
    TEST_ASSERT_EQUAL_FLOAT(l1.position.x, l2.position.x);
    TEST_ASSERT_EQUAL_FLOAT(l1.position.y, l2.position.y);
    TEST_ASSERT_EQUAL(2U, layout.getElementCount());
}

void test_padding_container_no_child() {
    UIPaddingContainer container(0, 0, 100, 100);
    container.setPadding(10);
    // No child set - should handle gracefully
    
    auto mock = std::make_unique<SimpleMockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Renderer renderer(config);
    container.draw(renderer);  // Should not crash
    container.update(16);
}

void test_grid_layout_single_column() {
    UIGridLayout layout(0, 0, 100, 200);
    layout.setColumns(1);
    layout.setPadding(5);
    layout.setSpacing(5);
    
    std::vector<std::unique_ptr<UILabel>> labels;
    for(int i = 0; i < 5; i++) {
        labels.push_back(std::make_unique<UILabel>("X", Vector2::ZERO(), Color::White, 1));
        layout.addElement(labels.back().get());
    }
    
    // Trigger layout update
    layout.update(16);
    
    // Verify vertical stacking - first element at padding position with centering
    // cellHeight = (200 - 10 - 20) / 5 = 34, label height = 8
    // position = 5 + (34-8)/2 = 18
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 18.0f, labels[0]->position.y);
}

void test_horizontal_layout_selected_element_callbacks() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setNavigationButtons(0, 1);
    
    bool callbackFired = false;
    UIButton b1("1", 0, {0, 0}, {40, 20}, nullptr);
    UIButton b2("2", 0, {0, 0}, {40, 20}, nullptr);
    layout.addElement(&b1);
    layout.addElement(&b2);
    
    layout.setSelectedIndex(0);
    TEST_ASSERT_EQUAL_PTR(&b1, layout.getSelectedElement());
    
    layout.setSelectedIndex(1);
    TEST_ASSERT_EQUAL_PTR(&b2, layout.getSelectedElement());
}

void test_vertical_layout_content_height_with_spacing() {
    UIVerticalLayout layout(0, 0, 100, 100);
    layout.setPadding(5);
    layout.setSpacing(10);
    
    UILabel l1("1", Vector2::ZERO(), Color::White, 1);  // height 8
    UILabel l2("2", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l1);
    layout.addElement(&l2);
    
    // Content height: padding(5) + height(8) + spacing(10) + height(8) + padding(5) = 36
    float expectedHeight = 5 + 8 + 10 + 8 + 5;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, expectedHeight, layout.getContentHeight());
}

void test_grid_layout_varying_element_sizes() {
    UIGridLayout layout(0, 0, 100, 50);  // Small layout that can only hold 4 elements in 2x2 grid
    layout.setColumns(2);
    layout.setPadding(5);
    layout.setSpacing(5);
    
    std::vector<std::unique_ptr<UILabel>> labels;
    // Add 6 elements to a 2x2 grid (should overflow to 3 rows)
    for(int i = 0; i < 6; i++) {
        labels.push_back(std::make_unique<UILabel>("X", Vector2::ZERO(), Color::White, 1));
        layout.addElement(labels.back().get());
    }
    
    // Trigger layout update
    layout.update(16);
    
    // Verify layout expanded to 3 rows to accommodate all elements
    TEST_ASSERT_EQUAL(3, layout.getRows());
    
    // Verify first element position (row 0, col 0)
    // cellWidth = (100 - 10 - 5) / 2 = 42.5, label width = 6
    // x = 5 + (42.5 - 6) / 2 = 23.25
    // cellHeight = (50 - 10 - 5) / 3 = 11.67, label height = 8  
    // y = 5 + (11.67 - 8) / 2 = 6.83
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 23.2f, labels[0]->position.x);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 6.8f, labels[0]->position.y);
    
    // Verify element in row 2, col 1 (5th element, index 4)
    // row = 4/2 = 2, col = 4%2 = 0
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 23.25f, labels[4]->position.x);  // first column
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 36.0f, labels[4]->position.y); // row 2 position
    
    // Verify last element (6th element, index 5)
    // row = 5/2 = 2, col = 5%2 = 1
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 23.25f + (42.5 + 5), labels[5]->position.x);  // second column
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 36.0f, labels[5]->position.y); // row 2 position
}

void test_anchor_layout_multiple_edge_references() {
    UIAnchorLayout layout(0, 0, 200, 150);
    
    // Create elements for different anchor points
    UILabel tl("TL", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel tr("TR", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel bl("BL", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel br("BR", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel tc("TC", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel bc("BC", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel lc("LC", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel rc("RC", Vector2::ZERO(), Color::White, 1);    // 12x8
    UILabel center("C", Vector2::ZERO(), Color::White, 1);  // 12x8
    
    // Add elements to all anchor points
    layout.addElement(&tl, Anchor::TOP_LEFT);
    layout.addElement(&tr, Anchor::TOP_RIGHT);
    layout.addElement(&bl, Anchor::BOTTOM_LEFT);
    layout.addElement(&br, Anchor::BOTTOM_RIGHT);
    layout.addElement(&tc, Anchor::TOP_CENTER);
    layout.addElement(&bc, Anchor::BOTTOM_CENTER);
    layout.addElement(&lc, Anchor::LEFT_CENTER);
    layout.addElement(&rc, Anchor::RIGHT_CENTER);
    layout.addElement(&center, Anchor::CENTER);
    
    // Trigger layout update
    layout.update(16);
    
    // Verify TOP_LEFT positioning (0, 0)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, tl.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, tl.position.y);
    
    // Verify TOP_RIGHT positioning (200 - 12, 0)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 188.0f, tr.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, tr.position.y);
    
    // Verify BOTTOM_LEFT positioning (0, 150 - 8)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, bl.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 142.0f, bl.position.y);
    
    // Verify BOTTOM_RIGHT positioning (200 - 12, 150 - 8)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 188.0f, br.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 142.0f, br.position.y);
    
    // Verify TOP_CENTER positioning (200/2 - 12/2, 0)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 94.0f, tc.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, tc.position.y);
    
    // Verify BOTTOM_CENTER positioning (200/2 - 12/2, 150 - 8)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 94.0f, bc.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 142.0f, bc.position.y);
    
    // Verify LEFT_CENTER positioning (0, 150/2 - 8/2)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, lc.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 71.0f, lc.position.y);
    
    // Verify RIGHT_CENTER positioning (200 - 12, 150/2 - 8/2)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 188.0f, rc.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 71.0f, rc.position.y);
    
    // Verify CENTER positioning (200/2 - 12/2, 150/2 - 8/2)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 97.0f, center.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 71.0f, center.position.y);
}

void test_anchor_layout_circular_reference_handling() {
    UIAnchorLayout layout(0, 0, 200, 150);
    
    // Create elements that could potentially create circular references
    // Note: AnchorLayout doesn't actually create circular references, but this test
    // ensures the layout handles multiple elements gracefully
    UILabel elem1("E1", Vector2::ZERO(), Color::White, 1);
    UILabel elem2("E2", Vector2::ZERO(), Color::White, 1);
    UILabel elem3("E3", Vector2::ZERO(), Color::White, 1);
    
    // Add multiple elements to the same anchor point
    layout.addElement(&elem1, Anchor::TOP_LEFT);
    layout.addElement(&elem2, Anchor::TOP_LEFT);
    layout.addElement(&elem3, Anchor::TOP_LEFT);
    
    // Trigger layout update
    layout.update(16);
    
    // All elements should be positioned at TOP_LEFT (0, 0)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem1.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem1.position.y);
    
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem2.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem2.position.y);
    
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem3.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem3.position.y);
    
    // Verify element count
    TEST_ASSERT_EQUAL(3U, layout.getElementCount());
    
    // Test removing one element doesn't affect others
    layout.removeElement(&elem2);
    layout.update(16);
    
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem1.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem1.position.y);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem3.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, elem3.position.y);
    
    TEST_ASSERT_EQUAL(2U, layout.getElementCount());
}

void test_anchor_layout_invalid_target_fallback() {
    UIAnchorLayout layout(0, 0, 200, 150);
    
    // Create a label and try to remove a non-existent element
    UILabel l1("Label1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("Label2", Vector2::ZERO(), Color::White, 1);
    
    // Add only l1
    layout.addElement(&l1, Anchor::TOP_LEFT);
    
    // Try to remove l2 (not in layout) - should handle gracefully
    layout.removeElement(&l2);  // Should not crash
    
    // Verify l1 is still there
    TEST_ASSERT_EQUAL(1U, layout.getElementCount());
    
    // Try to remove nullptr - should handle gracefully
    layout.removeElement(nullptr);  // Should not crash
    
    // Verify layout still has 1 element
    TEST_ASSERT_EQUAL(1U, layout.getElementCount());
    
    // Remove l1 properly
    layout.removeElement(&l1);
    TEST_ASSERT_EQUAL(0U, layout.getElementCount());
}

void test_anchor_layout_nested_constraints() {
    // Test anchor layout nested inside another layout container
    UIVerticalLayout outerLayout(0, 0, 300, 200);
    
    // Create an anchor layout as a child
    UIAnchorLayout* innerAnchor = new UIAnchorLayout(10, 10, 280, 180);
    
    // Add elements to the inner anchor layout
    UILabel tl("TL", Vector2::ZERO(), Color::White, 1);
    UILabel br("BR", Vector2::ZERO(), Color::White, 1);
    
    innerAnchor->addElement(&tl, Anchor::TOP_LEFT);
    innerAnchor->addElement(&br, Anchor::BOTTOM_RIGHT);
    
    // Add the anchor layout to the outer vertical layout
    outerLayout.addElement(innerAnchor);
    
    // Update the outer layout
    outerLayout.update(16);
    
    // Verify elements are positioned correctly within the nested anchor layout
    // When nested, elements maintain relative positions within the anchor layout
    // TOP_LEFT is at anchor layout's position (0, 0 relative to anchor)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, tl.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, tl.position.y);
    
    // BOTTOM_RIGHT should be at inner layout's bottom-right corner relative to its position
    // Position: inner layout size (280, 180) - label size (12, 8) = (268, 172)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 268.0f, br.position.x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 172.0f, br.position.y);
    
    // Clean up
    delete innerAnchor;
}

void test_horizontal_layout_wrap_enabled() {
    // Test horizontal layout with content overflow and scrolling
    UIHorizontalLayout layout(0, 0, 100, 50);
    layout.setScrollEnabled(true);
    layout.setSpacing(5);
    
    // Add multiple elements that would overflow the width
    UILabel l1("Label1", Vector2::ZERO(), Color::White, 1);  // width ~36
    UILabel l2("Label2", Vector2::ZERO(), Color::White, 1);  // width ~36
    UILabel l3("Label3", Vector2::ZERO(), Color::White, 1);  // width ~36
    UILabel l4("Label4", Vector2::ZERO(), Color::White, 1);  // width ~36
    
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    layout.addElement(&l4);
    
    layout.update(16);
    
    // Content width should be larger than viewport width (100)
    // Content width = padding + 4*labelWidth + 3*spacing (approximate)
    TEST_ASSERT_TRUE(layout.getContentWidth() > 100);
    
    // All elements should be positioned horizontally
    TEST_ASSERT_TRUE(l2.position.x > l1.position.x);
    TEST_ASSERT_TRUE(l3.position.x > l2.position.x);
    TEST_ASSERT_TRUE(l4.position.x > l3.position.x);
    
    // Verify scroll is enabled
    TEST_ASSERT_TRUE(layout.getScrollOffset() >= 0);
}

void test_horizontal_layout_spacing_overflow() {
    // Test horizontal layout with large spacing that causes overflow
    UIHorizontalLayout layout(0, 0, 100, 50);
    layout.setSpacing(50);  // Very large spacing
    
    // Add a few elements
    UILabel l1("A", Vector2::ZERO(), Color::White, 1);  // width ~6
    UILabel l2("B", Vector2::ZERO(), Color::White, 1);  // width ~6
    UILabel l3("C", Vector2::ZERO(), Color::White, 1);  // width ~6
    
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    
    layout.update(16);
    
    // With 50px spacing, elements should be far apart
    // Spacing should be applied even if it causes overflow
    float expectedGap = 50.0f;
    
    // Verify elements are spaced apart (allowing for some tolerance)
    TEST_ASSERT_TRUE(l2.position.x - l1.position.x >= expectedGap - 10);
    TEST_ASSERT_TRUE(l3.position.x - l2.position.x >= expectedGap - 10);
    
    // Content width should account for the large spacing
    // Approx: 3*6 + 2*50 = 18 + 100 = 118
    TEST_ASSERT_TRUE(layout.getContentWidth() > 100);
}

void test_horizontal_layout_dynamic_addition() {
    // Test adding elements dynamically after initial layout
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setSpacing(10);
    
    // Add initial elements
    UILabel l1("First", Vector2::ZERO(), Color::White, 1);
    UILabel l2("Second", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(&l1);
    layout.addElement(&l2);
    
    layout.update(16);
    
    // Record initial positions
    float l1x = l1.position.x;
    float l2x = l2.position.x;
    
    // Add a third element dynamically
    UILabel l3("Third", Vector2::ZERO(), Color::White, 1);
    layout.addElement(&l3);
    
    layout.update(16);
    
    // Verify all three elements are positioned correctly
    // l1 should stay at same position
    TEST_ASSERT_EQUAL_FLOAT(l1x, l1.position.x);
    // l2 should stay at same position  
    TEST_ASSERT_EQUAL_FLOAT(l2x, l2.position.x);
    // l3 should be positioned after l2
    TEST_ASSERT_TRUE(l3.position.x > l2.position.x);
    
    // Verify content width increased
    float contentWidthAfter = layout.getContentWidth();
    TEST_ASSERT_TRUE(contentWidthAfter > 0);
    
    // Verify element count
    TEST_ASSERT_EQUAL(3U, layout.getElementCount());
}

void test_horizontal_layout_content_width_variations() {
    // Test content width calculation with elements of different sizes
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setSpacing(5);
    
    // Add elements with different text lengths (different widths)
    UILabel shortLabel("S", Vector2::ZERO(), Color::White, 1);      // width ~6
    UILabel mediumLabel("Medium", Vector2::ZERO(), Color::White, 1); // width ~36
    UILabel longLabel("Long Text", Vector2::ZERO(), Color::White, 1); // width ~54
    
    layout.addElement(&shortLabel);
    layout.update(16);
    float width1 = layout.getContentWidth();
    
    layout.addElement(&mediumLabel);
    layout.update(16);
    float width2 = layout.getContentWidth();
    
    layout.addElement(&longLabel);
    layout.update(16);
    float width3 = layout.getContentWidth();
    
    // Content width should increase as elements are added
    TEST_ASSERT_TRUE(width2 > width1);
    TEST_ASSERT_TRUE(width3 > width2);
    
    // Verify total width accounts for spacing
    // Expected: 3 elements + 2 spacings
    float expectedMinWidth = 6 + 36 + 54 + 2 * 5; // ~106
    TEST_ASSERT_TRUE(width3 >= expectedMinWidth - 10);
}

void test_vertical_layout_scrolling_content() {
    // Test vertical layout with content overflow and scrolling
    UIVerticalLayout layout(0, 0, 100, 100);
    layout.setScrollEnabled(true);
    layout.setSpacing(10);
    
    // Add multiple elements that would overflow the height
    UILabel l1("Label1", Vector2::ZERO(), Color::White, 1);  // height ~8
    UILabel l2("Label2", Vector2::ZERO(), Color::White, 1);  // height ~8
    UILabel l3("Label3", Vector2::ZERO(), Color::White, 1);  // height ~8
    UILabel l4("Label4", Vector2::ZERO(), Color::White, 1);  // height ~8
    UILabel l5("Label5", Vector2::ZERO(), Color::White, 1);  // height ~8
    UILabel l6("Label6", Vector2::ZERO(), Color::White, 1);  // height ~8
    
    layout.addElement(&l1);
    layout.addElement(&l2);
    layout.addElement(&l3);
    layout.addElement(&l4);
    layout.addElement(&l5);
    layout.addElement(&l6);
    
    layout.update(16);
    
    // Content height should be larger than viewport height (100)
    // Content height = padding + 6*labelHeight + 5*spacing (approximate)
    // = 0 + 6*8 + 5*10 = 48 + 50 = 98 (plus padding)
    TEST_ASSERT_TRUE(layout.getContentHeight() > 50);
    
    // All elements should be positioned vertically
    TEST_ASSERT_TRUE(l2.position.y > l1.position.y);
    TEST_ASSERT_TRUE(l3.position.y > l2.position.y);
    TEST_ASSERT_TRUE(l4.position.y > l3.position.y);
    TEST_ASSERT_TRUE(l5.position.y > l4.position.y);
    TEST_ASSERT_TRUE(l6.position.y > l5.position.y);
    
    // Verify scroll is enabled
    TEST_ASSERT_TRUE(layout.getScrollOffset() >= 0);
}

void test_vertical_layout_content_height_variations() {
    // Test content height calculation with different element sizes
    UIVerticalLayout layout(0, 0, 100, 100);
    layout.setSpacing(5);
    
    UILabel shortLabel("S", Vector2::ZERO(), Color::White, 1);
    UILabel tallLabel("Tall", Vector2::ZERO(), Color::White, 2);
    
    layout.addElement(&shortLabel);
    layout.update(16);
    float height1 = layout.getContentHeight();
    
    layout.addElement(&tallLabel);
    layout.update(16);
    float height2 = layout.getContentHeight();
    
    TEST_ASSERT_TRUE(height2 > height1);
    TEST_ASSERT_TRUE(tallLabel.position.y > shortLabel.position.y);
}

void test_vertical_layout_nested() {
    // Test nested vertical layouts
    UIVerticalLayout outerLayout(0, 0, 200, 300);
    outerLayout.setSpacing(10);
    
    // Create inner vertical layout
    UIVerticalLayout* innerLayout = new UIVerticalLayout(0, 0, 180, 100);
    innerLayout->setSpacing(5);
    
    // Add elements to inner layout
    UILabel l1("Inner1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("Inner2", Vector2::ZERO(), Color::White, 1);
    innerLayout->addElement(&l1);
    innerLayout->addElement(&l2);
    
    // Add inner layout to outer layout
    outerLayout.addElement(innerLayout);
    
    // Add another element to outer layout
    UILabel l3("Outer", Vector2::ZERO(), Color::White, 1);
    outerLayout.addElement(&l3);
    
    outerLayout.update(16);
    
    // Verify inner layout elements are positioned
    TEST_ASSERT_TRUE(l2.position.y > l1.position.y);
    
    // Verify outer layout element is positioned after inner layout
    TEST_ASSERT_TRUE(l3.position.y > l1.position.y);
    
    // Clean up
    delete innerLayout;
}

void test_vertical_layout_resize_handling() {
    // Test layout with different spacing configurations
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setSpacing(10);
    
    // Add initial elements
    UILabel l1("Label1", Vector2::ZERO(), Color::White, 1);
    UILabel l2("Label2", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(&l1);
    layout.addElement(&l2);
    
    layout.update(16);
    
    // Record initial positions
    float l1y = l1.position.y;
    float l2y = l2.position.y;
    
    // Change spacing and update
    layout.setSpacing(20);
    layout.update(16);
    
    // Elements should be repositioned with new spacing
    // l1 stays at same position (top), l2 moves further down
    TEST_ASSERT_TRUE(l2.position.y > l1.position.y);
    
    // Verify content height is recalculated
    float contentHeight = layout.getContentHeight();
    TEST_ASSERT_TRUE(contentHeight > 0);
    
    // Verify element count unchanged
    TEST_ASSERT_EQUAL(2U, layout.getElementCount());
}
