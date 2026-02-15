#include <unity.h>
#include "graphics/DisplayConfig.h"
#include "graphics/Renderer.h"
#include "../../mocks/MockDrawSurface.h"

using namespace pixelroot32::graphics;

int MockDrawSurface::instances = 0;

void setUp(void) {
    MockDrawSurface::instances = 0;
}

void tearDown(void) {
}

void test_display_config_ownership(void) {
    TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
    
    {
        MockDrawSurface* mock = new MockDrawSurface();
        TEST_ASSERT_EQUAL(1, MockDrawSurface::instances);
        
        DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
        TEST_ASSERT_EQUAL(1, MockDrawSurface::instances);
    }
    
    // DisplayConfig should have deleted the mock
    TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
}

void test_display_config_move_semantics(void) {
    TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
    
    {
        MockDrawSurface* mock = new MockDrawSurface();
        DisplayConfig config1 = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
        TEST_ASSERT_EQUAL(1, MockDrawSurface::instances);
        
        DisplayConfig config2 = std::move(config1);
        TEST_ASSERT_EQUAL(1, MockDrawSurface::instances);
    }
    
    TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
}

void test_renderer_ownership_transfer(void) {
    TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
    
    {
        MockDrawSurface* mock = new MockDrawSurface();
        DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
        
        {
            Renderer renderer(config);
            TEST_ASSERT_EQUAL(1, MockDrawSurface::instances);
            
            // Renderer should have taken ownership. 
            // config.releaseDrawSurface() should now return null.
            TEST_ASSERT_NULL(config.releaseDrawSurface().get());
        }
        
        // Renderer goes out of scope, mock should be deleted
        TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_display_config_ownership);
    RUN_TEST(test_display_config_move_semantics);
    RUN_TEST(test_renderer_ownership_transfer);
    return UNITY_END();
}
