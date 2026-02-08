/**
 * @file test_collision_primitives.cpp
 * @brief Unit tests for physics/CollisionPrimitives module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for collision detection functions:
 * - Circle vs Circle
 * - Circle vs Rect
 * - Segment vs Rect
 * - sweepCircleVsRect
 */

#include <unity.h>
#include <cstdint>
#include "../../test_config.h"

// Definiciones locales para evitar dependencias de Arduino
namespace pixelroot32 {
namespace core {
struct Rect {
    float x, y;
    int width, height;
    
    bool intersects(const Rect& other) const {
        return !(x + width < other.x || x > other.x + other.width ||
                 y + height < other.y || y > other.y + other.height);
    }
};
}

namespace physics {

struct Circle {
    float x;
    float y;
    float radius;
};

struct Segment {
    float x1;
    float y1;
    float x2;
    float y2;
};

// Implementaciones de funciones de colisión
bool intersects(const Circle& a, const Circle& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float r = a.radius + b.radius;
    return dx * dx + dy * dy <= r * r;
}

bool intersects(const Circle& c, const core::Rect& r) {
    float closestX = c.x;
    float closestY = c.y;

    if (closestX < r.x) closestX = r.x;
    else if (closestX > r.x + r.width) closestX = r.x + r.width;

    if (closestY < r.y) closestY = r.y;
    else if (closestY > r.y + r.height) closestY = r.y + r.height;

    float dx = c.x - closestX;
    float dy = c.y - closestY;
    return dx * dx + dy * dy <= c.radius * c.radius;
}

bool intersects(const Segment& s, const core::Rect& r) {
    float x1 = s.x1;
    float y1 = s.y1;
    float x2 = s.x2;
    float y2 = s.y2;

    float dx = x2 - x1;
    float dy = y2 - y1;

    float tMin = 0.0f;
    float tMax = 1.0f;

    if (dx != 0.0f) {
        float invDx = 1.0f / dx;
        float tx1 = (r.x - x1) * invDx;
        float tx2 = (r.x + r.width - x1) * invDx;
        if (tx1 > tx2) {
            float tmp = tx1;
            tx1 = tx2;
            tx2 = tmp;
        }
        if (tx1 > tMin) tMin = tx1;
        if (tx2 < tMax) tMax = tx2;
        if (tMin > tMax) return false;
    } else {
        if (x1 < r.x || x1 > r.x + r.width) {
            return false;
        }
    }

    if (dy != 0.0f) {
        float invDy = 1.0f / dy;
        float ty1 = (r.y - y1) * invDy;
        float ty2 = (r.y + r.height - y1) * invDy;
        if (ty1 > ty2) {
            float tmp = ty1;
            ty1 = ty2;
            ty2 = tmp;
        }
        if (ty1 > tMin) tMin = ty1;
        if (ty2 < tMax) tMax = ty2;
        if (tMin > tMax) return false;
    } else {
        if (y1 < r.y || y1 > r.y + r.height) {
            return false;
        }
    }

    return tMax >= 0.0f && tMin <= 1.0f;
}

bool sweepCircleVsRect(const Circle& start,
                       const Circle& end,
                       const core::Rect& rect,
                       float& tHit) {
    core::Rect expanded;
    expanded.x = rect.x - start.radius;
    expanded.y = rect.y - start.radius;
    expanded.width = rect.width + static_cast<int>(start.radius * 2.0f);
    expanded.height = rect.height + static_cast<int>(start.radius * 2.0f);

    Segment path;
    path.x1 = start.x;
    path.y1 = start.y;
    path.x2 = end.x;
    path.y2 = end.y;

    float x1 = path.x1;
    float y1 = path.y1;
    float x2 = path.x2;
    float y2 = path.y2;

    float dx = x2 - x1;
    float dy = y2 - y1;

    float tMin = 0.0f;
    float tMax = 1.0f;

    if (dx != 0.0f) {
        float invDx = 1.0f / dx;
        float tx1 = (expanded.x - x1) * invDx;
        float tx2 = (expanded.x + expanded.width - x1) * invDx;
        if (tx1 > tx2) {
            float tmp = tx1;
            tx1 = tx2;
            tx2 = tmp;
        }
        if (tx1 > tMin) tMin = tx1;
        if (tx2 < tMax) tMax = tx2;
        if (tMin > tMax) return false;
    } else {
        if (x1 < expanded.x || x1 > expanded.x + expanded.width) {
            return false;
        }
    }

    if (dy != 0.0f) {
        float invDy = 1.0f / dy;
        float ty1 = (expanded.y - y1) * invDy;
        float ty2 = (expanded.y + expanded.height - y1) * invDy;
        if (ty1 > ty2) {
            float tmp = ty1;
            ty1 = ty2;
            ty2 = tmp;
        }
        if (ty1 > tMin) tMin = ty1;
        if (ty2 < tMax) tMax = ty2;
        if (tMin > tMax) return false;
    } else {
        if (y1 < expanded.y || y1 > expanded.y + expanded.height) {
            return false;
        }
    }

    if (tMax < 0.0f || tMin > 1.0f) {
        return false;
    }

    tHit = tMin;
    return true;
}

} // namespace physics
} // namespace pixelroot32

using namespace pixelroot32::core;
using namespace pixelroot32::physics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for intersects(Circle, Circle)
// =============================================================================

void test_circle_intersects_same_center(void) {
    Circle c1 = {0, 0, 5};
    Circle c2 = {0, 0, 3};
    TEST_ASSERT_TRUE(intersects(c1, c2));
}

void test_circle_intersects_overlapping(void) {
    Circle c1 = {0, 0, 5};
    Circle c2 = {6, 0, 5};
    TEST_ASSERT_TRUE(intersects(c1, c2));
}

void test_circle_intersects_touching(void) {
    Circle c1 = {0, 0, 5};
    Circle c2 = {10, 0, 5};
    TEST_ASSERT_TRUE(intersects(c1, c2));
}

void test_circle_intersects_separate(void) {
    Circle c1 = {0, 0, 5};
    Circle c2 = {20, 0, 5};
    TEST_ASSERT_FALSE(intersects(c1, c2));
}

void test_circle_intersects_diagonal(void) {
    Circle c1 = {0, 0, 5};
    Circle c2 = {3, 4, 5};
    TEST_ASSERT_TRUE(intersects(c1, c2));
}

void test_circle_intersects_negative_coords(void) {
    Circle c1 = {-10, -10, 5};
    Circle c2 = {-12, -10, 5};
    TEST_ASSERT_TRUE(intersects(c1, c2));
}

void test_circle_intersects_symmetric(void) {
    Circle c1 = {0, 0, 5};
    Circle c2 = {10, 0, 5};
    TEST_ASSERT_EQUAL(intersects(c1, c2), intersects(c2, c1));
}

// =============================================================================
// Tests for intersects(Circle, Rect)
// =============================================================================

void test_circle_rect_center_inside(void) {
    Circle c = {50, 50, 10};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(c, r));
}

void test_circle_rect_overlapping(void) {
    Circle c = {15, 50, 10};
    Rect r = {20, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(c, r));
}

void test_circle_rect_touching_edge(void) {
    Circle c = {10, 50, 10};
    Rect r = {20, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(c, r));
}

void test_circle_rect_separate(void) {
    Circle c = {0, 0, 5};
    Rect r = {20, 20, 10, 10};
    TEST_ASSERT_FALSE(intersects(c, r));
}

void test_circle_rect_corner_close(void) {
    Circle c = {15, 15, 5};
    Rect r = {20, 20, 10, 10};
    // Distance from circle center (15,15) to rect corner (20,20) is ~7.07
    // which is greater than radius 5, so no intersection
    TEST_ASSERT_FALSE(intersects(c, r));
}

void test_circle_rect_corner_far(void) {
    Circle c = {5, 5, 5};
    Rect r = {20, 20, 10, 10};
    TEST_ASSERT_FALSE(intersects(c, r));
}

void test_circle_rect_zero_radius(void) {
    Circle c = {50, 50, 0};
    Rect r = {40, 40, 20, 20};
    // A point inside the rect
    TEST_ASSERT_TRUE(intersects(c, r));
}

void test_circle_rect_negative_coords(void) {
    Circle c = {-15, -50, 10};
    Rect r = {-20, -40, 20, 20};
    TEST_ASSERT_TRUE(intersects(c, r));
}

// =============================================================================
// Tests for intersects(Segment, Rect)
// =============================================================================

void test_segment_rect_crossing(void) {
    Segment s = {0, 50, 100, 50};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(s, r));
}

void test_segment_rect_vertical_crossing(void) {
    Segment s = {50, 0, 50, 100};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(s, r));
}

void test_segment_rect_diagonal_crossing(void) {
    Segment s = {0, 0, 100, 100};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(s, r));
}

void test_segment_rect_start_inside(void) {
    Segment s = {50, 50, 100, 100};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(s, r));
}

void test_segment_rect_end_inside(void) {
    Segment s = {0, 0, 50, 50};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(s, r));
}

void test_segment_rect_separate(void) {
    Segment s = {0, 0, 30, 30};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_FALSE(intersects(s, r));
}

void test_segment_rect_touching_corner(void) {
    Segment s = {0, 0, 40, 40};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(s, r));
}

void test_segment_rect_parallel_above(void) {
    Segment s = {0, 20, 100, 20};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_FALSE(intersects(s, r));
}

void test_segment_rect_point_inside(void) {
    Segment s = {50, 50, 50, 50};
    Rect r = {40, 40, 20, 20};
    TEST_ASSERT_TRUE(intersects(s, r));
}

// =============================================================================
// Tests for sweepCircleVsRect
// =============================================================================

void test_sweep_circle_rect_direct_hit(void) {
    Circle start = {10, 50, 5};
    Circle end = {90, 50, 5};
    Rect r = {40, 40, 20, 20};
    float tHit = -1.0f;
    
    TEST_ASSERT_TRUE(sweepCircleVsRect(start, end, r, tHit));
    TEST_ASSERT_TRUE(tHit >= 0.0f && tHit <= 1.0f);
}

void test_sweep_circle_rect_no_hit(void) {
    Circle start = {10, 10, 5};
    Circle end = {90, 10, 5};
    Rect r = {40, 40, 20, 20};
    float tHit = -1.0f;
    
    TEST_ASSERT_FALSE(sweepCircleVsRect(start, end, r, tHit));
}

void test_sweep_circle_rect_start_inside(void) {
    Circle start = {50, 50, 5};
    Circle end = {90, 50, 5};
    Rect r = {40, 40, 20, 20};
    float tHit = -1.0f;
    
    TEST_ASSERT_TRUE(sweepCircleVsRect(start, end, r, tHit));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, tHit);
}

void test_sweep_circle_rect_already_past(void) {
    Circle start = {90, 50, 5};
    Circle end = {100, 50, 5};
    Rect r = {40, 40, 20, 20};
    float tHit = -1.0f;
    
    TEST_ASSERT_FALSE(sweepCircleVsRect(start, end, r, tHit));
}

void test_sweep_circle_rect_vertical_hit(void) {
    Circle start = {50, 10, 5};
    Circle end = {50, 90, 5};
    Rect r = {40, 40, 20, 20};
    float tHit = -1.0f;
    
    TEST_ASSERT_TRUE(sweepCircleVsRect(start, end, r, tHit));
    TEST_ASSERT_TRUE(tHit >= 0.0f && tHit <= 1.0f);
}

void test_sweep_circle_rect_diagonal_hit(void) {
    Circle start = {10, 10, 5};
    Circle end = {90, 90, 5};
    Rect r = {40, 40, 20, 20};
    float tHit = -1.0f;
    
    TEST_ASSERT_TRUE(sweepCircleVsRect(start, end, r, tHit));
}

void test_sweep_circle_rect_large_radius(void) {
    Circle start = {10, 50, 20};
    Circle end = {90, 50, 20};
    Rect r = {40, 40, 20, 20};
    float tHit = -1.0f;
    
    TEST_ASSERT_TRUE(sweepCircleVsRect(start, end, r, tHit));
}

void test_sweep_circle_rect_negative_coords(void) {
    Circle start = {-50, -50, 5};
    Circle end = {-10, -10, 5};
    Rect r = {-30, -30, 10, 10};
    float tHit = -1.0f;
    
    TEST_ASSERT_TRUE(sweepCircleVsRect(start, end, r, tHit));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    // Circle vs Circle tests
    RUN_TEST(test_circle_intersects_same_center);
    RUN_TEST(test_circle_intersects_overlapping);
    RUN_TEST(test_circle_intersects_touching);
    RUN_TEST(test_circle_intersects_separate);
    RUN_TEST(test_circle_intersects_diagonal);
    RUN_TEST(test_circle_intersects_negative_coords);
    RUN_TEST(test_circle_intersects_symmetric);
    
    // Circle vs Rect tests
    RUN_TEST(test_circle_rect_center_inside);
    RUN_TEST(test_circle_rect_overlapping);
    RUN_TEST(test_circle_rect_touching_edge);
    RUN_TEST(test_circle_rect_separate);
    RUN_TEST(test_circle_rect_corner_close);
    RUN_TEST(test_circle_rect_corner_far);
    RUN_TEST(test_circle_rect_zero_radius);
    RUN_TEST(test_circle_rect_negative_coords);
    
    // Segment vs Rect tests
    RUN_TEST(test_segment_rect_crossing);
    RUN_TEST(test_segment_rect_vertical_crossing);
    RUN_TEST(test_segment_rect_diagonal_crossing);
    RUN_TEST(test_segment_rect_start_inside);
    RUN_TEST(test_segment_rect_end_inside);
    RUN_TEST(test_segment_rect_separate);
    RUN_TEST(test_segment_rect_touching_corner);
    RUN_TEST(test_segment_rect_parallel_above);
    RUN_TEST(test_segment_rect_point_inside);
    
    // Sweep tests
    RUN_TEST(test_sweep_circle_rect_direct_hit);
    RUN_TEST(test_sweep_circle_rect_no_hit);
    RUN_TEST(test_sweep_circle_rect_start_inside);
    RUN_TEST(test_sweep_circle_rect_already_past);
    RUN_TEST(test_sweep_circle_rect_vertical_hit);
    RUN_TEST(test_sweep_circle_rect_diagonal_hit);
    RUN_TEST(test_sweep_circle_rect_large_radius);
    RUN_TEST(test_sweep_circle_rect_negative_coords);
    
    return UNITY_END();
}
