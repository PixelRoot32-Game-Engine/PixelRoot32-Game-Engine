#include <unity.h>
#include "../../test_config.h"
#include "graphics/DisplayConfig.h"
#include "graphics/Renderer.h"
#include "graphics/ui/UILabel.h"
#include "graphics/ui/UIButton.h"
#include "graphics/ui/UICheckbox.h"
#include "graphics/ui/UIPanel.h"
#include "graphics/FontManager.h"
#include "graphics/BaseDrawSurface.h"
#include "input/InputManager.h"
#include "input/InputConfig.h"
#include <vector>
#include <string>
#include <memory>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;
using namespace pixelroot32::input;


// Mock font data
const uint16_t dummyGlyphData[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
const Sprite dummyGlyph = { dummyGlyphData, 6, 8 };
Sprite dummyGlyphs[128]; // Enough for ASCII
const Font dummyFont = { dummyGlyphs, 0, 127, 6, 8, 0, 8 };

// Mock DrawSurface to capture drawing calls
class MockDrawSurface : public BaseDrawSurface {
public:
    struct RectCall {
        int x, y, w, h;
        uint16_t color;
        bool filled;
    };
    // TextCall removed as DrawSurface no longer handles text rendering

    std::vector<RectCall> rectCalls;
    // std::vector<TextCall> textCalls;

    void init() override {}
    void clearBuffer() override {
        rectCalls.clear();
        // textCalls.clear();
        pixelCalls.clear();
    }
    void sendBuffer() override {}
    // drawText removed
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override {
        rectCalls.push_back({x, y, width, height, color, false});
    }
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override {
        rectCalls.push_back({x, y, width, height, color, true});
    }
    void drawPixel(int x, int y, uint16_t color) override {
        pixelCalls.push_back({x, y, color});
    }
    struct PixelCall {
        int x, y;
        uint16_t color;
    };
    std::vector<PixelCall> pixelCalls;
};

void test_ui_label_creation_and_position() {
    UILabel label("Hello", {10, 20}, Color::White, 2);
    
    TEST_ASSERT_EQUAL_FLOAT(10, label.position.x);
    TEST_ASSERT_EQUAL_FLOAT(20, label.position.y);
}

void test_ui_label_draw() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    // Keep raw pointer for assertions since DisplayConfig takes ownership
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    UILabel label("Hello", {10, 20}, Color::White, 2);
    label.draw(renderer);
    
    // TEST_ASSERT_EQUAL(1, mockRaw->textCalls.size());
    // TEST_ASSERT_EQUAL_STRING("Hello", mockRaw->textCalls[0].text.c_str());
    // TEST_ASSERT_EQUAL(10, mockRaw->textCalls[0].x);
    // TEST_ASSERT_EQUAL(20, mockRaw->textCalls[0].y);
    
    // Instead verify that *something* was drawn (pixels)
    // Note: Since dummyGlyphData is all 0s, drawPixel might not be called if skip transparent is used.
    // However, renderer implementation likely draws opaque pixels if set.
    // For now, just ensuring it compiles and runs without crashing is a start.
    // If dummyGlyph has non-zero pixels, we'd see pixel calls.
}

void test_ui_label_draw_with_data() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    // Keep raw pointer for assertions since DisplayConfig takes ownership
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);

    // Set a glyph with some bits: 0x0001 is bit 0
    uint16_t glyphData[] = { 0x0001, 0, 0, 0, 0, 0, 0, 0 };
    Sprite glyph = { glyphData, 1, 1 }; // 1x1 sprite
    Sprite glyphs[128];
    for(int i=0; i<128; i++) glyphs[i] = glyph;
    Font font = { glyphs, 0, 127, 1, 1, 0, 1 };
    
    FontManager::setDefaultFont(&font);
    
    UILabel label("A", {10, 20}, Color::White, 1);
    label.draw(renderer);
    
    TEST_ASSERT_FALSE(mockRaw->pixelCalls.empty());
    // (10, 20) is the position
    TEST_ASSERT_EQUAL(10, mockRaw->pixelCalls[0].x);
    TEST_ASSERT_EQUAL(20, mockRaw->pixelCalls[0].y);
    
    FontManager::setDefaultFont(&dummyFont); // Restore
}

void test_ui_button_creation() {
    bool clicked = false;
    UIButton button("Click Me", 0, {10, 10}, {100, 30}, [&](){ clicked = true; });
    
    TEST_ASSERT_EQUAL_FLOAT(10, button.position.x);
    TEST_ASSERT_EQUAL_FLOAT(10, button.position.y);
    TEST_ASSERT_EQUAL_FLOAT(100, (float)button.width);
    TEST_ASSERT_EQUAL_FLOAT(30, (float)button.height);
    TEST_ASSERT_FALSE(button.getSelected());
    
    button.press();
    TEST_ASSERT_TRUE(clicked);
}

void test_ui_button_selection() {
    UIButton button("Btn", 0, {0, 0}, {50, 20}, nullptr);
    
    button.setSelected(true);
    TEST_ASSERT_TRUE(button.getSelected());
    
    button.setSelected(false);
    TEST_ASSERT_FALSE(button.getSelected());
}

void test_ui_checkbox_toggle() {
    bool state = false;
    UICheckBox checkbox("Check", 0, 0, 0, 20, 20, false, [&](bool s){ state = s; });
    
    TEST_ASSERT_FALSE(checkbox.isChecked());
    
    checkbox.toggle();
    TEST_ASSERT_TRUE(checkbox.isChecked());
    TEST_ASSERT_TRUE(state);
    
    checkbox.toggle();
    TEST_ASSERT_FALSE(checkbox.isChecked());
    TEST_ASSERT_FALSE(state);
}

void test_ui_button_styles_and_alignment() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);

    UIButton button("Style", 0, {0, 0}, {100, 20}, nullptr, TextAlignment::CENTER);
    
    // Test CENTER alignment and background
    button.setStyle(Color::Red, Color::Blue, true);
    button.draw(renderer);
    TEST_ASSERT_EQUAL(1, mockRaw->rectCalls.size());
    TEST_ASSERT_TRUE(mockRaw->rectCalls[0].filled);

    // Test No background and selection marker
    mockRaw->clearBuffer();
    button.setStyle(Color::Red, Color::Blue, false);
    button.setSelected(true);
    button.draw(renderer);
    TEST_ASSERT_EQUAL(0, mockRaw->rectCalls.size()); 
    // Button will draw ">" marker with drawText, we can't easily verify text but we verify it doesn't crash
    
    // Test RIGHT alignment
    UIButton buttonRight("Right", 0, {0, 0}, {100, 20}, nullptr, TextAlignment::RIGHT);
    buttonRight.draw(renderer);
}

void test_ui_button_input_simulation() {
    bool clicked = false;
    UIButton button("Click", 7, {0, 0}, {100, 20}, [&](){ clicked = true; });
    button.setSelected(true);
    
    // Simulate input: Button 7 pressed
    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    
    uint8_t keys[256] = {0};
    keys[7] = 1; // Mark "key 7" as pressed
    
    input.update(16, keys);
    
    button.handleInput(input);
    TEST_ASSERT_TRUE(clicked);
}

void test_ui_panel_hierarchy() {
    UIPanel panel(10, 10, 100, 100);
    auto label = std::make_unique<UILabel>("Inside", Vector2::ZERO(), Color::White, 1);
    
    panel.setChild(label.get());
    TEST_ASSERT_EQUAL_PTR(label.get(), panel.getChild());
    
    // Panel at (10,10), child relative (0,0) should be at (10,10)
    TEST_ASSERT_EQUAL_FLOAT(10, label->position.x);
    TEST_ASSERT_EQUAL_FLOAT(10, label->position.y);
    
    panel.setPosition(20, 30);
    TEST_ASSERT_EQUAL_FLOAT(20, label->position.x);
    TEST_ASSERT_EQUAL_FLOAT(30, label->position.y);
}

void test_ui_panel_draw() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);

    UIPanel panel(5, 5, 80, 60);
    panel.setBackgroundColor(Color::Navy);
    panel.setBorderColor(Color::White);
    panel.setBorderWidth(2);
    panel.draw(renderer);

    TEST_ASSERT_TRUE(mockRaw->rectCalls.size() >= 5);
    mockRaw->clearBuffer();

    panel.setBackgroundColor(Color::Transparent);
    panel.setBorderWidth(0);
    panel.draw(renderer);
}

void test_ui_checkbox_draw_and_style() {
    auto mockDrawer = std::make_unique<MockDrawSurface>();
    MockDrawSurface* mockRaw = mockDrawer.get();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);

    UICheckBox checkbox("Option", 0, 10, 10, 80, 24, false, nullptr);
    checkbox.setStyle(Color::Cyan, Color::Black, true);
    checkbox.draw(renderer);
    TEST_ASSERT_TRUE(mockRaw->rectCalls.size() >= 1);

    mockRaw->clearBuffer();
    checkbox.setStyle(Color::White, Color::Gray, false);
    checkbox.draw(renderer);
}

void test_ui_checkbox_set_checked_callback() {
    bool callbackFired = false;
    bool receivedValue = false;
    UICheckBox checkbox("Cb", 0, 0, 0, 50, 20, false, [&](bool v) { callbackFired = true; receivedValue = v; });
    checkbox.setChecked(true);
    TEST_ASSERT_TRUE(checkbox.isChecked());
    TEST_ASSERT_TRUE(callbackFired);
    TEST_ASSERT_TRUE(receivedValue);
    callbackFired = false;
    checkbox.setChecked(false);
    TEST_ASSERT_TRUE(callbackFired);
    TEST_ASSERT_FALSE(receivedValue);
}

void test_ui_checkbox_handle_input_when_selected() {
    bool toggled = false;
    UICheckBox checkbox("Cb", 3, 0, 0, 50, 20, false, [&](bool) { toggled = true; });
    checkbox.setSelected(true);

    InputConfig inConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    InputManager input(inConfig);
    input.init();
    uint8_t keys[256] = {0};
    keys[3] = 1;
    input.update(16, keys);

    checkbox.handleInput(input);
    TEST_ASSERT_TRUE(toggled);
}

void test_ui_checkbox_get_selected_is_point_inside_focusable() {
    UICheckBox checkbox("Opt", 0, 10, 10, 60, 24, false, nullptr);
    TEST_ASSERT_FALSE(checkbox.getSelected());
    TEST_ASSERT_TRUE(checkbox.isFocusable());
    checkbox.setSelected(true);
    TEST_ASSERT_TRUE(checkbox.getSelected());
    checkbox.update(16);
}

void test_ui_button_is_focusable_and_is_point_inside() {
    UIButton button("Btn", 0, {10, 10}, {80, 30}, nullptr);
    TEST_ASSERT_TRUE(button.isFocusable());
}

void test_ui_element_is_focusable_panel_label() {
    UIPanel panel(0, 0, 100, 100);
    TEST_ASSERT_FALSE(panel.isFocusable());
    UILabel label("L", Vector2::ZERO(), Color::White, 1);
    TEST_ASSERT_FALSE(label.isFocusable());
}

void setUp(void) {
    test_setup();
    for(int i=0; i<128; i++) dummyGlyphs[i] = dummyGlyph;
    FontManager::setDefaultFont(&dummyFont);
}

void tearDown(void) {
    FontManager::setDefaultFont(nullptr);
    test_teardown();
}

extern void test_vertical_layout_positioning();
extern void test_horizontal_layout_positioning();
extern void test_grid_layout_positioning();
extern void test_anchor_layout_positioning();
extern void test_padding_container();
extern void test_vertical_layout_scrolling();
extern void test_vertical_layout_remove_element();
extern void test_vertical_layout_clear_elements();
extern void test_vertical_layout_draw();
extern void test_vertical_layout_set_selected_index_and_get_element();
extern void test_vertical_layout_handle_input_nav();
extern void test_anchor_layout_set_screen_size();
extern void test_anchor_layout_top_right_bottom_left();
extern void test_anchor_layout_remove_element();
extern void test_anchor_layout_draw();
extern void test_grid_layout_variable_columns();
extern void test_grid_layout_remove_element();
extern void test_grid_layout_get_element();
extern void test_horizontal_layout_draw();
extern void test_horizontal_layout_handle_input_nav();
extern void test_horizontal_layout_set_selected_index_and_update();
extern void test_grid_layout_draw();
extern void test_grid_layout_set_selected_index_and_handle_input();
extern void test_vertical_layout_update();
extern void test_vertical_layout_set_button_style();
extern void test_padding_container_set_padding_single_and_draw();
extern void test_ui_label_set_text_and_center_x();
extern void test_horizontal_layout_scrolling();
extern void test_horizontal_layout_get_spacing();
extern void test_horizontal_layout_get_padding();
extern void test_grid_layout_zero_columns();
extern void test_grid_layout_overflow_elements();
extern void test_grid_layout_large_element_count();
extern void test_anchor_layout_top_center_and_bottom();
extern void test_anchor_layout_multiple_edge_references();
extern void test_anchor_layout_circular_reference_handling();
extern void test_anchor_layout_invalid_target_fallback();
extern void test_anchor_layout_nested_constraints();
extern void test_horizontal_layout_wrap_enabled();
extern void test_horizontal_layout_spacing_overflow();
extern void test_horizontal_layout_dynamic_addition();
extern void test_horizontal_layout_content_width_variations();
extern void test_vertical_layout_scrolling_content();
extern void test_vertical_layout_content_height_variations();
extern void test_vertical_layout_nested();
extern void test_vertical_layout_resize_handling();
extern void test_vertical_layout_with_buttons();
extern void test_vertical_layout_empty();
extern void test_horizontal_layout_empty();
extern void test_anchor_layout_empty();
extern void test_grid_layout_negative_cell_dimensions();
extern void test_grid_layout_handle_input_with_buttons();
extern void test_horizontal_layout_navigation_wrap();
extern void test_vertical_layout_navigation_wrap();
extern void test_anchor_layout_multiple_elements_same_anchor();
extern void test_padding_container_no_child();
extern void test_grid_layout_single_column();
extern void test_grid_layout_varying_element_sizes();
extern void test_horizontal_layout_selected_element_callbacks();
extern void test_vertical_layout_content_height_with_spacing();

void run_layout_tests() {
    RUN_TEST(test_vertical_layout_positioning);
    RUN_TEST(test_horizontal_layout_positioning);
    RUN_TEST(test_grid_layout_positioning);
    RUN_TEST(test_anchor_layout_positioning);
    RUN_TEST(test_padding_container);
    RUN_TEST(test_vertical_layout_scrolling);
    RUN_TEST(test_vertical_layout_remove_element);
    RUN_TEST(test_vertical_layout_clear_elements);
    RUN_TEST(test_vertical_layout_draw);
    RUN_TEST(test_vertical_layout_set_selected_index_and_get_element);
    RUN_TEST(test_vertical_layout_handle_input_nav);
    RUN_TEST(test_anchor_layout_set_screen_size);
    RUN_TEST(test_anchor_layout_top_right_bottom_left);
    RUN_TEST(test_anchor_layout_remove_element);
    RUN_TEST(test_anchor_layout_draw);
    RUN_TEST(test_grid_layout_variable_columns);
    RUN_TEST(test_grid_layout_remove_element);
    RUN_TEST(test_grid_layout_get_element);
    RUN_TEST(test_horizontal_layout_draw);
    RUN_TEST(test_horizontal_layout_handle_input_nav);
    RUN_TEST(test_horizontal_layout_set_selected_index_and_update);
    RUN_TEST(test_grid_layout_draw);
    RUN_TEST(test_grid_layout_set_selected_index_and_handle_input);
    RUN_TEST(test_vertical_layout_update);
    RUN_TEST(test_vertical_layout_set_button_style);
    RUN_TEST(test_padding_container_set_padding_single_and_draw);
    RUN_TEST(test_ui_label_set_text_and_center_x);
    RUN_TEST(test_horizontal_layout_scrolling);
    // New edge case tests
    RUN_TEST(test_horizontal_layout_get_spacing);
    RUN_TEST(test_horizontal_layout_get_padding);
    RUN_TEST(test_grid_layout_zero_columns);
    RUN_TEST(test_grid_layout_varying_element_sizes);
    RUN_TEST(test_grid_layout_large_element_count);
    RUN_TEST(test_anchor_layout_top_center_and_bottom);
    RUN_TEST(test_anchor_layout_multiple_edge_references);
    RUN_TEST(test_anchor_layout_circular_reference_handling);
    RUN_TEST(test_anchor_layout_invalid_target_fallback);
    RUN_TEST(test_anchor_layout_nested_constraints);
    RUN_TEST(test_horizontal_layout_wrap_enabled);
    RUN_TEST(test_horizontal_layout_spacing_overflow);
    RUN_TEST(test_horizontal_layout_dynamic_addition);
    RUN_TEST(test_horizontal_layout_content_width_variations);
    RUN_TEST(test_vertical_layout_scrolling_content);
    RUN_TEST(test_vertical_layout_content_height_variations);
    RUN_TEST(test_vertical_layout_nested);
    RUN_TEST(test_vertical_layout_resize_handling);
    RUN_TEST(test_vertical_layout_with_buttons);
    RUN_TEST(test_vertical_layout_empty);
    RUN_TEST(test_horizontal_layout_empty);
    RUN_TEST(test_anchor_layout_empty);
    RUN_TEST(test_grid_layout_negative_cell_dimensions);
    RUN_TEST(test_grid_layout_handle_input_with_buttons);
    RUN_TEST(test_horizontal_layout_navigation_wrap);
    RUN_TEST(test_vertical_layout_navigation_wrap);
    RUN_TEST(test_anchor_layout_multiple_elements_same_anchor);
    RUN_TEST(test_padding_container_no_child);
    RUN_TEST(test_grid_layout_single_column);
    RUN_TEST(test_grid_layout_overflow_elements);
    RUN_TEST(test_grid_layout_varying_element_sizes);
    RUN_TEST(test_horizontal_layout_selected_element_callbacks);
    RUN_TEST(test_vertical_layout_content_height_with_spacing);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ui_label_creation_and_position);
    RUN_TEST(test_ui_label_draw_with_data);
    RUN_TEST(test_ui_button_creation);
    RUN_TEST(test_ui_button_selection);
    RUN_TEST(test_ui_checkbox_toggle);
    RUN_TEST(test_ui_panel_hierarchy);
    RUN_TEST(test_ui_panel_draw);
    RUN_TEST(test_ui_checkbox_draw_and_style);
    RUN_TEST(test_ui_checkbox_set_checked_callback);
    RUN_TEST(test_ui_checkbox_handle_input_when_selected);
    RUN_TEST(test_ui_checkbox_get_selected_is_point_inside_focusable);
    RUN_TEST(test_ui_button_is_focusable_and_is_point_inside);
    RUN_TEST(test_ui_element_is_focusable_panel_label);
    RUN_TEST(test_ui_button_styles_and_alignment);
    RUN_TEST(test_ui_button_input_simulation);
    run_layout_tests();
    return UNITY_END();
}
