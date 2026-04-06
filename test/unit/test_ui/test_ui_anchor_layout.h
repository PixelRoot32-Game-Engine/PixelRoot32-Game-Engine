#pragma once

#include <unity.h>
#include "../../test_config.h"
#include "graphics/ui/UIAnchorLayout.h"
#include "graphics/ui/UIPanel.h"
#include "graphics/ui/UILabel.h"
#include <memory>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;

// ============================================================================
// UIAnchorLayout Tests
// ============================================================================

void test_anchor_layout_initialization() {
    UIAnchorLayout layout(0, 0, 240, 240);
    TEST_ASSERT_EQUAL_FLOAT(0, layout.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, layout.position.y);
    TEST_ASSERT_EQUAL(240, static_cast<int>(layout.getScreenWidth()));
    TEST_ASSERT_EQUAL(240, static_cast<int>(layout.getScreenHeight()));
}

void test_anchor_layout_set_screen_size() {
    UIAnchorLayout layout(0, 0, 240, 240);
    layout.setScreenSize(320, 240);
    TEST_ASSERT_EQUAL(320, static_cast<int>(layout.getScreenWidth()));
    TEST_ASSERT_EQUAL(240, static_cast<int>(layout.getScreenHeight()));
}

void test_anchor_top_left() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::TOP_LEFT);
    
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.y);
}

void test_anchor_top_right() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::TOP_RIGHT);
    
    TEST_ASSERT_EQUAL_FLOAT(80, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.y);
}

void test_anchor_bottom_left() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::BOTTOM_LEFT);
    
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, panel.position.y);
}

void test_anchor_bottom_right() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::BOTTOM_RIGHT);
    
    TEST_ASSERT_EQUAL_FLOAT(80, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, panel.position.y);
}

void test_anchor_center() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::CENTER);
    
    TEST_ASSERT_EQUAL_FLOAT(40, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(45, panel.position.y);
}

void test_anchor_top_center() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::TOP_CENTER);
    
    TEST_ASSERT_EQUAL_FLOAT(40, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.y);
}

void test_anchor_bottom_center() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::BOTTOM_CENTER);
    
    TEST_ASSERT_EQUAL_FLOAT(40, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, panel.position.y);
}

void test_anchor_left_center() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::LEFT_CENTER);
    
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(45, panel.position.y);
}

void test_anchor_right_center() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::RIGHT_CENTER);
    
    TEST_ASSERT_EQUAL_FLOAT(80, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(45, panel.position.y);
}

void test_anchor_layout_multiple_elements() {
    UIAnchorLayout layout(0, 0, 100, 100);
    
    UIPanel tl(0, 0, 10, 10);
    UIPanel tr(0, 0, 10, 10);
    UIPanel bl(0, 0, 10, 10);
    UIPanel br(0, 0, 10, 10);
    UIPanel center(0, 0, 10, 10);
    
    layout.addElement(&tl, Anchor::TOP_LEFT);
    layout.addElement(&tr, Anchor::TOP_RIGHT);
    layout.addElement(&bl, Anchor::BOTTOM_LEFT);
    layout.addElement(&br, Anchor::BOTTOM_RIGHT);
    layout.addElement(&center, Anchor::CENTER);
    
    TEST_ASSERT_EQUAL_FLOAT(0, tl.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, tr.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, bl.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, br.position.x);
    TEST_ASSERT_EQUAL_FLOAT(45, center.position.x);
}

void test_anchor_layout_remove_element() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 10, 10);
    
    layout.addElement(&panel, Anchor::CENTER);
    TEST_ASSERT_EQUAL(1, static_cast<int>(layout.getElementCount()));
    
    layout.removeElement(&panel);
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

void test_anchor_layout_update_layout() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 20, 10);
    
    layout.addElement(&panel, Anchor::TOP_LEFT);
    layout.setScreenSize(200, 200);
    layout.updateLayout();
    
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.y);
}

void test_anchor_layout_default_anchor() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel panel(0, 0, 10, 10);
    
    layout.addElement(&panel);
    
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, panel.position.y);
}

void test_anchor_layout_different_sizes() {
    UIAnchorLayout layout(0, 0, 320, 240);
    
    UIPanel small(0, 0, 10, 10);
    UIPanel large(0, 0, 50, 30);
    
    layout.addElement(&small, Anchor::TOP_RIGHT);
    layout.addElement(&large, Anchor::BOTTOM_RIGHT);
    
    TEST_ASSERT_EQUAL_FLOAT(310, small.position.x);
    TEST_ASSERT_EQUAL_FLOAT(270, large.position.x);
    TEST_ASSERT_EQUAL_FLOAT(210, large.position.y);
}

void test_anchor_layout_vector2_constructor() {
    UIAnchorLayout layout(Vector2(toScalar(10), toScalar(20)), 100, 100);
    
    TEST_ASSERT_EQUAL_FLOAT(10, layout.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20, layout.position.y);
}

void test_anchor_layout_empty() {
    UIAnchorLayout layout(0, 0, 100, 100);
    
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
    layout.updateLayout();
    TEST_ASSERT_TRUE(true);
}

void test_anchor_layout_clear_elements() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UIPanel p1(0, 0, 10, 10);
    UIPanel p2(0, 0, 10, 10);
    
    layout.addElement(&p1, Anchor::TOP_LEFT);
    layout.addElement(&p2, Anchor::BOTTOM_RIGHT);
    TEST_ASSERT_EQUAL(2, static_cast<int>(layout.getElementCount()));
    
    layout.clearElements();
    TEST_ASSERT_EQUAL(0, static_cast<int>(layout.getElementCount()));
}

void test_anchor_with_label() {
    UIAnchorLayout layout(0, 0, 100, 100);
    UILabel label("Test", Vector2::ZERO(), Color::White, 1);
    
    layout.addElement(&label, Anchor::CENTER);
    
    TEST_ASSERT_TRUE(true);
}

void test_anchor_all_nine_positions() {
    UIAnchorLayout layout(0, 0, 200, 200);
    
    UIPanel tl(0, 0, 20, 20);
    UIPanel tc(0, 0, 20, 20);
    UIPanel tr(0, 0, 20, 20);
    UIPanel lc(0, 0, 20, 20);
    UIPanel center(0, 0, 20, 20);
    UIPanel rc(0, 0, 20, 20);
    UIPanel bl(0, 0, 20, 20);
    UIPanel bc(0, 0, 20, 20);
    UIPanel br(0, 0, 20, 20);
    
    layout.addElement(&tl, Anchor::TOP_LEFT);
    layout.addElement(&tc, Anchor::TOP_CENTER);
    layout.addElement(&tr, Anchor::TOP_RIGHT);
    layout.addElement(&lc, Anchor::LEFT_CENTER);
    layout.addElement(&center, Anchor::CENTER);
    layout.addElement(&rc, Anchor::RIGHT_CENTER);
    layout.addElement(&bl, Anchor::BOTTOM_LEFT);
    layout.addElement(&bc, Anchor::BOTTOM_CENTER);
    layout.addElement(&br, Anchor::BOTTOM_RIGHT);
    
    TEST_ASSERT_EQUAL_FLOAT(90, tc.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0, tc.position.y);
    TEST_ASSERT_EQUAL_FLOAT(90, bc.position.x);
    TEST_ASSERT_EQUAL_FLOAT(180, bc.position.y);
    TEST_ASSERT_EQUAL_FLOAT(0, lc.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, lc.position.y);
    TEST_ASSERT_EQUAL_FLOAT(180, rc.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, rc.position.y);
    TEST_ASSERT_EQUAL_FLOAT(90, center.position.x);
    TEST_ASSERT_EQUAL_FLOAT(90, center.position.y);
}
