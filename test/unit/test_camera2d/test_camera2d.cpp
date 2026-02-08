/**
 * @file test_camera2d.cpp
 * @brief Unit tests for graphics/Camera2D module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for 2D camera including:
 * - Position and viewport management
 * - Bounds clamping
 * - Target following with dead zones
 */

#include <unity.h>
#include <cstdint>
#include "../../test_config.h"

// Mock implementation
namespace pixelroot32 {
namespace graphics {

class Renderer {
public:
    int xOffset = 0;
    int yOffset = 0;
    
    void setDisplayOffset(int x, int y) {
        xOffset = x;
        yOffset = y;
    }
};

class Camera2D {
public:
    float x, y;
    int viewportWidth, viewportHeight;
    float minX, maxX, minY, maxY;
    
    Camera2D(int viewportWidth, int viewportHeight)
        : x(0.0f), y(0.0f)
        , viewportWidth(viewportWidth), viewportHeight(viewportHeight)
        , minX(0.0f), maxX(0.0f)
        , minY(0.0f), maxY(0.0f) {}
    
    void setBounds(float minXValue, float maxXValue) {
        minX = minXValue;
        maxX = maxXValue;
    }
    
    void setVerticalBounds(float minYValue, float maxYValue) {
        minY = minYValue;
        maxY = maxYValue;
    }
    
    void setPosition(float newX, float newY) {
        x = newX;
        y = newY;
        
        if (x < minX) x = minX;
        if (x > maxX) x = maxX;
        if (y < minY) y = minY;
        if (y > maxY) y = maxY;
    }
    
    void followTarget(float targetX) {
        float deadZoneLeft = viewportWidth * 0.3f;
        float deadZoneRight = viewportWidth * 0.7f;
        
        float screenX = targetX - x;
        
        if (screenX < deadZoneLeft) {
            float newX = targetX - deadZoneLeft;
            setPosition(newX, y);
        } else if (screenX > deadZoneRight) {
            float newX = targetX - deadZoneRight;
            setPosition(newX, y);
        }
    }
    
    void followTarget(float targetX, float targetY) {
        followTarget(targetX);
        
        float deadZoneTop = viewportHeight * 0.3f;
        float deadZoneBottom = viewportHeight * 0.7f;
        
        float screenY = targetY - y;
        
        if (screenY < deadZoneTop) {
            float newY = targetY - deadZoneTop;
            setPosition(x, newY);
        } else if (screenY > deadZoneBottom) {
            float newY = targetY - deadZoneBottom;
            setPosition(x, newY);
        }
    }
    
    float getX() const { return x; }
    float getY() const { return y; }
    
    void apply(Renderer& renderer) const {
        renderer.setDisplayOffset(static_cast<int>(-x), static_cast<int>(-y));
    }
    
    void setViewportSize(int width, int height) {
        viewportWidth = width;
        viewportHeight = height;
    }
};

}
}

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for initialization
// =============================================================================

void test_camera_initialization(void) {
    Camera2D camera(240, 240);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.y);
    TEST_ASSERT_EQUAL_INT(240, camera.viewportWidth);
    TEST_ASSERT_EQUAL_INT(240, camera.viewportHeight);
}

void test_camera_initial_position(void) {
    Camera2D camera(100, 100);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.getX());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.getY());
}

// =============================================================================
// Tests for setPosition
// =============================================================================

void test_camera_set_position(void) {
    Camera2D camera(240, 240);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    
    camera.setPosition(50.0f, 75.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(50.0f, camera.getX());
    TEST_ASSERT_EQUAL_FLOAT(75.0f, camera.getY());
}

void test_camera_set_position_zero(void) {
    Camera2D camera(240, 240);
    camera.setPosition(100.0f, 100.0f);
    
    camera.setPosition(0.0f, 0.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.getX());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.getY());
}

void test_camera_set_position_negative(void) {
    Camera2D camera(240, 240);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    
    camera.setPosition(-50.0f, -100.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(-50.0f, camera.getX());
    TEST_ASSERT_EQUAL_FLOAT(-100.0f, camera.getY());
}

// =============================================================================
// Tests for bounds clamping
// =============================================================================

void test_camera_bounds_clamp_x(void) {
    Camera2D camera(240, 240);
    camera.setBounds(0.0f, 100.0f);
    
    camera.setPosition(150.0f, 50.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(100.0f, camera.getX());
}

void test_camera_bounds_clamp_y(void) {
    Camera2D camera(240, 240);
    camera.setVerticalBounds(0.0f, 100.0f);
    
    camera.setPosition(50.0f, 150.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(100.0f, camera.getY());
}

void test_camera_bounds_clamp_negative(void) {
    Camera2D camera(240, 240);
    camera.setBounds(0.0f, 100.0f);
    
    camera.setPosition(-50.0f, 50.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.getX());
}

void test_camera_bounds_no_clamp(void) {
    Camera2D camera(240, 240);
    camera.setBounds(0.0f, 100.0f);
    camera.setVerticalBounds(0.0f, 100.0f);
    
    camera.setPosition(50.0f, 50.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(50.0f, camera.getX());
    TEST_ASSERT_EQUAL_FLOAT(50.0f, camera.getY());
}

// =============================================================================
// Tests for viewport size
// =============================================================================

void test_camera_set_viewport_size(void) {
    Camera2D camera(100, 100);
    
    camera.setViewportSize(320, 240);
    
    TEST_ASSERT_EQUAL_INT(320, camera.viewportWidth);
    TEST_ASSERT_EQUAL_INT(240, camera.viewportHeight);
}

void test_camera_set_viewport_square(void) {
    Camera2D camera(100, 100);
    
    camera.setViewportSize(240, 240);
    
    TEST_ASSERT_EQUAL_INT(240, camera.viewportWidth);
    TEST_ASSERT_EQUAL_INT(240, camera.viewportHeight);
}

// =============================================================================
// Tests for followTarget (horizontal)
// =============================================================================

void test_camera_follow_no_move_in_deadzone(void) {
    Camera2D camera(100, 100);
    camera.setPosition(0.0f, 0.0f);
    
    // Target at 50 (center of 100 viewport), should not move camera
    camera.followTarget(50.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, camera.getX());
}

void test_camera_follow_move_left(void) {
    Camera2D camera(100, 100);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setPosition(0.0f, 0.0f);
    
    // Target at 20 (left of deadzone), camera should move left
    camera.followTarget(20.0f);
    
    // Deadzone left is 30% = 30px
    // New camera position = target - deadZoneLeft = 20 - 30 = -10
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, camera.getX());
}

void test_camera_follow_move_right(void) {
    Camera2D camera(100, 100);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setPosition(0.0f, 0.0f);
    
    // Target at 80 (right of deadzone), camera should move right
    camera.followTarget(80.0f);
    
    // Deadzone right is 70% = 70px
    // New camera position = target - deadZoneRight = 80 - 70 = 10
    TEST_ASSERT_EQUAL_FLOAT(10.0f, camera.getX());
}

// =============================================================================
// Tests for followTarget (2D)
// =============================================================================

void test_camera_follow_2d_vertical(void) {
    Camera2D camera(100, 100);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    camera.setPosition(0.0f, 0.0f);
    
    // Target at y=20 (top of deadzone), camera should move up
    camera.followTarget(50.0f, 20.0f);
    
    // Deadzone top is 30% = 30px
    // New camera Y = target - deadZoneTop = 20 - 30 = -10
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, camera.getY());
}

void test_camera_follow_2d_bottom(void) {
    Camera2D camera(100, 100);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    camera.setPosition(0.0f, 0.0f);
    
    // Target at y=80 (bottom of deadzone), camera should move down
    camera.followTarget(50.0f, 80.0f);
    
    // Deadzone bottom is 70% = 70px
    // New camera Y = target - deadZoneBottom = 80 - 70 = 10
    TEST_ASSERT_EQUAL_FLOAT(10.0f, camera.getY());
}

// =============================================================================
// Tests for apply to renderer
// =============================================================================

void test_camera_apply(void) {
    Camera2D camera(240, 240);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    camera.setPosition(50.0f, 75.0f);
    Renderer renderer;
    
    camera.apply(renderer);
    
    TEST_ASSERT_EQUAL_INT(-50, renderer.xOffset);
    TEST_ASSERT_EQUAL_INT(-75, renderer.yOffset);
}

void test_camera_apply_zero(void) {
    Camera2D camera(240, 240);
    camera.setPosition(0.0f, 0.0f);
    Renderer renderer;
    
    camera.apply(renderer);
    
    TEST_ASSERT_EQUAL_INT(0, renderer.xOffset);
    TEST_ASSERT_EQUAL_INT(0, renderer.yOffset);
}

void test_camera_apply_negative(void) {
    Camera2D camera(240, 240);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    camera.setPosition(-50.0f, -100.0f);
    Renderer renderer;
    
    camera.apply(renderer);
    
    TEST_ASSERT_EQUAL_INT(50, renderer.xOffset);
    TEST_ASSERT_EQUAL_INT(100, renderer.yOffset);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_camera_initialization);
    RUN_TEST(test_camera_initial_position);
    
    // Position tests
    RUN_TEST(test_camera_set_position);
    RUN_TEST(test_camera_set_position_zero);
    RUN_TEST(test_camera_set_position_negative);
    
    // Bounds tests
    RUN_TEST(test_camera_bounds_clamp_x);
    RUN_TEST(test_camera_bounds_clamp_y);
    RUN_TEST(test_camera_bounds_clamp_negative);
    RUN_TEST(test_camera_bounds_no_clamp);
    
    // Viewport tests
    RUN_TEST(test_camera_set_viewport_size);
    RUN_TEST(test_camera_set_viewport_square);
    
    // Follow tests (horizontal)
    RUN_TEST(test_camera_follow_no_move_in_deadzone);
    RUN_TEST(test_camera_follow_move_left);
    RUN_TEST(test_camera_follow_move_right);
    
    // Follow tests (2D)
    RUN_TEST(test_camera_follow_2d_vertical);
    RUN_TEST(test_camera_follow_2d_bottom);
    
    // Apply tests
    RUN_TEST(test_camera_apply);
    RUN_TEST(test_camera_apply_zero);
    RUN_TEST(test_camera_apply_negative);
    
    return UNITY_END();
}
