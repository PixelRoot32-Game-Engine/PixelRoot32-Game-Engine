#include <unity.h>
#include "../../test_config.h"
#include "graphics/ui/UIVerticalLayout.h"
#include "graphics/ui/UIHorizontalLayout.h"
#include "graphics/ui/UIGridLayout.h"
#include "graphics/ui/UIAnchorLayout.h"
#include "graphics/ui/UIPaddingContainer.h"
#include "graphics/ui/UILabel.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include <vector>
#include <memory>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;

void test_vertical_layout_positioning() {
    UIVerticalLayout layout(0, 0, 100, 200);
    layout.setPadding(10);
    layout.setSpacing(5);
    
    auto label1 = std::make_unique<UILabel>("L1", 0, 0, Color::White, 1); 
    auto label2 = std::make_unique<UILabel>("L2", 0, 0, Color::White, 1); 
    
    layout.addElement(label1.get());
    layout.addElement(label2.get());
    
    // label1 should be centered horizontally: (100 - 12) / 2 = 44
    // label1 should be at y = padding = 10
    TEST_ASSERT_EQUAL_FLOAT(44, label1->x);
    TEST_ASSERT_EQUAL_FLOAT(10, label1->y);
    
    // label2 should be centered horizontally: (100 - 12) / 2 = 44
    // label2 should be at y = padding + label1.height + spacing = 10 + 8 + 5 = 23
    TEST_ASSERT_EQUAL_FLOAT(44, label2->x);
    TEST_ASSERT_EQUAL_FLOAT(23, label2->y);
}

void test_horizontal_layout_positioning() {
    UIHorizontalLayout layout(0, 0, 200, 50);
    layout.setPadding(5);
    layout.setSpacing(10);
    
    auto label1 = std::make_unique<UILabel>("L1", 0, 0, Color::White, 1);
    auto label2 = std::make_unique<UILabel>("L2", 0, 0, Color::White, 1);
    
    layout.addElement(label1.get());
    layout.addElement(label2.get());
    
    // label1: (padding, centeredY) = (5, (50-8)/2 = 21)
    TEST_ASSERT_EQUAL_FLOAT(5, label1->x);
    TEST_ASSERT_EQUAL_FLOAT(21, label1->y);
    
    // label2: (padding + label1.width + spacing, centeredY)
    // 5 + 12 + 10 = 27
    TEST_ASSERT_EQUAL_FLOAT(27, label2->x);
    TEST_ASSERT_EQUAL_FLOAT(21, label2->y);
}

void test_grid_layout_positioning() {
    // 2 columns grid
    UIGridLayout layout(0, 0, 100, 100);
    layout.setColumns(2);
    layout.setPadding(0);
    layout.setSpacing(0);
    
    auto l1 = std::make_unique<UILabel>("1", 0, 0, Color::White, 1);
    auto l2 = std::make_unique<UILabel>("2", 0, 0, Color::White, 1);
    auto l3 = std::make_unique<UILabel>("3", 0, 0, Color::White, 1);
    
    layout.addElement(l1.get());
    layout.addElement(l2.get());
    layout.addElement(l3.get());
    
    // Grid 2 cols, width 100 -> cell width 50, cell height 50
    // l1: col 0, row 0 -> centered in (0,0,50,50) -> (22, 21)
    TEST_ASSERT_EQUAL_FLOAT(22, l1->x);
    TEST_ASSERT_EQUAL_FLOAT(21, l1->y);
    
    // l2: col 1, row 0 -> centered in (50,0,50,50) -> (50+22, 21) = (72, 21)
    TEST_ASSERT_EQUAL_FLOAT(72, l2->x);
    TEST_ASSERT_EQUAL_FLOAT(21, l2->y);
    
    // l3: col 0, row 1 -> centered in (0,50,50,50) -> (22, 50+21) = (22, 71)
    TEST_ASSERT_EQUAL_FLOAT(22, l3->x);
    TEST_ASSERT_EQUAL_FLOAT(71, l3->y);
}

void test_padding_container() {
    UIPaddingContainer container(10, 20, 100, 100);
    container.setPadding(20, 10, 5, 15); // left, right, top, bottom
    
    UILabel* label = new UILabel("P", 0, 0, Color::White, 1);
    container.setChild(label);
    
    // Parent at (10, 20), left padding 20, top padding 5
    // Label should be at (10 + 20, 20 + 5) = (30, 25)
    TEST_ASSERT_EQUAL_FLOAT(30, label->x);
    TEST_ASSERT_EQUAL_FLOAT(25, label->y);
    
    delete label;
}

void test_anchor_layout_positioning() {
    UIAnchorLayout layout(0, 0, 100, 100);
    
    UILabel* l1 = new UILabel("TL", 0, 0, Color::White, 1); // width 12, height 8
    UILabel* l2 = new UILabel("BR", 0, 0, Color::White, 1);
    UILabel* l3 = new UILabel("C", 0, 0, Color::White, 1); // width 6, height 8
    
    layout.addElement(l1, Anchor::TOP_LEFT);
    layout.addElement(l2, Anchor::BOTTOM_RIGHT);
    layout.addElement(l3, Anchor::CENTER);
    
    // TOP_LEFT: (0, 0)
    TEST_ASSERT_EQUAL_FLOAT(0, l1->x);
    TEST_ASSERT_EQUAL_FLOAT(0, l1->y);
    
    // BOTTOM_RIGHT: (100 - 12, 100 - 8) = (88, 92)
    TEST_ASSERT_EQUAL_FLOAT(88, l2->x);
    TEST_ASSERT_EQUAL_FLOAT(92, l2->y);
    
    // CENTER: (50 - 3, 50 - 4) = (47, 46)
    TEST_ASSERT_EQUAL_FLOAT(47, l3->x);
    TEST_ASSERT_EQUAL_FLOAT(46, l3->y);
    
    delete l1;
    delete l2;
    delete l3;
}
