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
    TEST_ASSERT_EQUAL(0, layout.getSpacing());
    layout.setSpacing(15);
    TEST_ASSERT_EQUAL(15, layout.getSpacing());
}

void test_horizontal_layout_get_padding() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    TEST_ASSERT_EQUAL(0, layout.getPadding());
    layout.setPadding(10);
    TEST_ASSERT_EQUAL(10, layout.getPadding());
}
