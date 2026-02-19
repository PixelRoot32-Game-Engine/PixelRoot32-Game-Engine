/**
 * @file test_camera2d.cpp
 * @brief Unit tests for graphics/Camera2D module
 * @version 1.1
 * @date 2026-02-08
 */

#include <unity.h>
#include <cstdint>
#include "../../test_config.h"
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_camera_initialization(void) {
    Camera2D camera(240, 240);
    
    TEST_ASSERT_FLOAT_EQUAL(0.0f, camera.getX());
    TEST_ASSERT_FLOAT_EQUAL(0.0f, camera.getY());
}

void test_camera_set_position(void) {
    Camera2D camera(240, 240);
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    
    camera.setPosition({100.5f, 200.7f});
    
    TEST_ASSERT_FLOAT_EQUAL(100.5f, camera.getX());
    TEST_ASSERT_FLOAT_EQUAL(200.7f, camera.getY());
}

void test_camera_bounds_clamping(void) {
    Camera2D camera(240, 240);
    camera.setBounds(0.0f, 500.0f);
    camera.setVerticalBounds(0.0f, 500.0f);
    
    // Test clamping to min
    camera.setPosition({-10.0f, -20.0f});
    TEST_ASSERT_FLOAT_EQUAL(0.0f, camera.getX());
    TEST_ASSERT_FLOAT_EQUAL(0.0f, camera.getY());
    
    // Test clamping to max
    camera.setPosition({600.0f, 700.0f});
    TEST_ASSERT_FLOAT_EQUAL(500.0f, camera.getX());
    TEST_ASSERT_FLOAT_EQUAL(500.0f, camera.getY());
}

void test_camera_follow_target_dead_zone(void) {
    Camera2D camera(100, 100); // Small viewport for easy math
    camera.setBounds(-1000.0f, 1000.0f);
    camera.setVerticalBounds(-1000.0f, 1000.0f);
    
    // Initial position 0,0. Dead zones: 30 to 70
    
    // Target inside dead zone - camera shouldn't move
    camera.followTarget({50.0f, 50.0f});
    TEST_ASSERT_FLOAT_EQUAL(0.0f, camera.getX());
    TEST_ASSERT_FLOAT_EQUAL(0.0f, camera.getY());
    
    // Target moves past right dead zone (70)
    camera.followTarget({80.0f, 50.0f});
    // 80 - 70 = 10. Camera should move to 10 to keep target at 70% of viewport
    TEST_ASSERT_FLOAT_EQUAL(10.0f, camera.getX());
    
    // Target moves past left dead zone (30 on screen, which is 10 + 30 = 40 in world)
    camera.followTarget({35.0f, 50.0f});
    // Target is at 35 in world. Camera is at 10. Screen pos is 25.
    // 35 - 30 = 5. Camera should move to 5.
    TEST_ASSERT_FLOAT_EQUAL(5.0f, camera.getX());
}

void test_camera_apply_to_renderer(void) {
    Camera2D camera(240, 240);
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    Renderer renderer(config);
    
    camera.setPosition({100.0f, 150.0f});
    camera.apply(renderer);
    
    // Since we don't have getDisplayOffset in Renderer, we can't verify directly
    // but we can check if it compiles and runs without crashing.
    // In a more complex mock we could verify Renderer internal state.
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_camera_initialization);
    RUN_TEST(test_camera_set_position);
    RUN_TEST(test_camera_bounds_clamping);
    RUN_TEST(test_camera_follow_target_dead_zone);
    RUN_TEST(test_camera_apply_to_renderer);
    
    return UNITY_END();
}
