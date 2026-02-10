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
#include <vector>
#include <string>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;

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
    struct TextCall {
        std::string text;
        int x, y;
        uint16_t color;
        uint8_t size;
    };

    std::vector<RectCall> rectCalls;
    std::vector<TextCall> textCalls;

    void init() override {}
    void clearBuffer() override {
        rectCalls.clear();
        textCalls.clear();
        pixelCalls.clear();
    }
    void sendBuffer() override {}
    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) override {
        textCalls.push_back({text, x, y, color, size});
    }
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
    UILabel label("Hello", 10, 20, Color::White, 2);
    
    TEST_ASSERT_EQUAL_FLOAT(10, label.x);
    TEST_ASSERT_EQUAL_FLOAT(20, label.y);
}

void test_ui_label_draw() {
    MockDrawSurface* mockDrawer = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer, 240, 240);
    Renderer renderer(config);
    
    UILabel label("Hello", 10, 20, Color::White, 2);
    label.draw(renderer);
    
    TEST_ASSERT_EQUAL(1, mockDrawer->textCalls.size());
    TEST_ASSERT_EQUAL_STRING("Hello", mockDrawer->textCalls[0].text.c_str());
    TEST_ASSERT_EQUAL(10, mockDrawer->textCalls[0].x);
    TEST_ASSERT_EQUAL(20, mockDrawer->textCalls[0].y);
}

void test_ui_label_draw_with_data() {
    MockDrawSurface* mockDrawer = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer, 240, 240);
    Renderer renderer(config);

    // Set a glyph with some bits: 0x0001 is bit 0
    uint16_t glyphData[] = { 0x0001, 0, 0, 0, 0, 0, 0, 0 };
    Sprite glyph = { glyphData, 1, 1 }; // 1x1 sprite
    Sprite glyphs[128];
    for(int i=0; i<128; i++) glyphs[i] = glyph;
    Font font = { glyphs, 0, 127, 1, 1, 0, 1 };
    
    FontManager::setDefaultFont(&font);
    
    UILabel label("A", 10, 20, Color::White, 1);
    label.draw(renderer);
    
    TEST_ASSERT_FALSE(mockDrawer->pixelCalls.empty());
    // (10, 20) is the position
    TEST_ASSERT_EQUAL(10, mockDrawer->pixelCalls[0].x);
    TEST_ASSERT_EQUAL(20, mockDrawer->pixelCalls[0].y);
    
    FontManager::setDefaultFont(&dummyFont); // Restore
}

void test_ui_button_creation() {
    bool clicked = false;
    UIButton button("Click Me", 0, 10, 10, 100, 30, [&](){ clicked = true; });
    
    TEST_ASSERT_EQUAL_FLOAT(10, button.x);
    TEST_ASSERT_EQUAL_FLOAT(10, button.y);
    TEST_ASSERT_EQUAL_FLOAT(100, (float)button.width);
    TEST_ASSERT_EQUAL_FLOAT(30, (float)button.height);
    TEST_ASSERT_FALSE(button.getSelected());
    
    button.press();
    TEST_ASSERT_TRUE(clicked);
}

void test_ui_button_selection() {
    UIButton button("Btn", 0, 0, 0, 50, 20, nullptr);
    
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

void test_ui_panel_hierarchy() {
    UIPanel panel(10, 10, 100, 100);
    UILabel* label = new UILabel("Inside", 0, 0, Color::White, 1);
    
    panel.setChild(label);
    TEST_ASSERT_EQUAL_PTR(label, panel.getChild());
    
    // Panel at (10,10), child relative (0,0) should be at (10,10)
    TEST_ASSERT_EQUAL_FLOAT(10, label->x);
    TEST_ASSERT_EQUAL_FLOAT(10, label->y);
    
    panel.setPosition(20, 30);
    TEST_ASSERT_EQUAL_FLOAT(20, label->x);
    TEST_ASSERT_EQUAL_FLOAT(30, label->y);
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

// Layout tests moved here to avoid multiple main()
extern void test_vertical_layout_positioning();
extern void test_horizontal_layout_positioning();
extern void test_grid_layout_positioning();
extern void test_anchor_layout_positioning();
extern void test_padding_container();

void run_layout_tests() {
    RUN_TEST(test_vertical_layout_positioning);
    RUN_TEST(test_horizontal_layout_positioning);
    RUN_TEST(test_grid_layout_positioning);
    RUN_TEST(test_anchor_layout_positioning);
    RUN_TEST(test_padding_container);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ui_label_creation_and_position);
    RUN_TEST(test_ui_label_draw_with_data);
    RUN_TEST(test_ui_button_creation);
    RUN_TEST(test_ui_button_selection);
    RUN_TEST(test_ui_checkbox_toggle);
    RUN_TEST(test_ui_panel_hierarchy);
    run_layout_tests();
    return UNITY_END();
}
