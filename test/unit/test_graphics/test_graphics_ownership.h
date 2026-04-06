#pragma once

#include <unity.h>
#include "graphics/DisplayConfig.h"
#include "graphics/Renderer.h"
#include "../../mocks/MockDrawSurface.h"

using namespace pixelroot32::graphics;

// Forward declaration for setUp/tearDown
extern int mock_surface_instances;

// ============================================================================
// Graphics Ownership Tests
// ============================================================================

void test_display_config_ownership(void) {
    // Reset counter at start of each ownership test
    MockDrawSurface::instances = 0;
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
    // Reset counter at start of each ownership test
    MockDrawSurface::instances = 0;
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
    // Reset counter at start of each ownership test
    MockDrawSurface::instances = 0;
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
