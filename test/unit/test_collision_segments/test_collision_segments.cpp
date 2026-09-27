/**
 * @file test_collision_segments.cpp
 * @brief Unit tests for static segment collision vs circles (issue #241).
 *
 * Pins the API shape taken from the validated Lunar Pool demo
 * (PixelRoot32-Demo-Projects games/pool): one static segment per actor,
 * circle-vs-segment contact with the normal perpendicular to the segment,
 * segment end points behaving as zero-radius circles, and contacts flowing
 * through the existing impulse/restitution path.
 */

#include <unity.h>
#include <cmath>
#include "../../test_config.h"
#include "physics/CollisionSystem.h"
#include "physics/RigidActor.h"
#include "physics/StaticActor.h"
#include "core/Actor.h"
#include "core/PhysicsActor.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

// Mock rigid circle for testing
class SegmentMockCircle : public RigidActor {
public:
    bool collisionCalled = false;

    SegmentMockCircle(float x, float y, float r)
        : RigidActor(toScalar(x), toScalar(y), 16, 16) {
        setShape(CollisionShape::CIRCLE);
        setRadius(toScalar(r));
        setCollisionLayer(1);
        setCollisionMask(1);
        // system.update() integrates gravity before detecting; zero it so
        // reflection assertions stay exact.
        setGravityScale(toScalar(0.0f));
    }

    void onCollision(Actor* other) override { (void)other; collisionCalled = true; }
    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }
};

// Mock rigid box for testing (default AABB shape)
class SegmentMockBox : public RigidActor {
public:
    bool collisionCalled = false;

    SegmentMockBox(float x, float y, int w, int h) : RigidActor(toScalar(x), toScalar(y), w, h) {
        setCollisionLayer(1);
        setCollisionMask(1);
    }

    void onCollision(Actor* other) override { (void)other; collisionCalled = true; }
    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }
};

class SegmentMockWall : public StaticActor {
public:
    bool collisionCalled = false;

    SegmentMockWall(float x, float y, float ax, float ay, float bx, float by)
        : StaticActor(toScalar(x), toScalar(y), 8, 8) {
        setShape(CollisionShape::SEGMENT);
        setSegment(Vector2(toScalar(ax), toScalar(ay)), Vector2(toScalar(bx), toScalar(by)));
        setCollisionLayer(1);
        setCollisionMask(1);
    }

    void onCollision(Actor* other) override { (void)other; collisionCalled = true; }
    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Segment API
// =============================================================================

void test_segment_endpoints_are_world_coordinates(void) {
    SegmentMockWall wall(100.0f, 100.0f, 0.0f, 0.0f, 40.0f, 20.0f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(100.0f, static_cast<float>(wall.getSegmentA().x), 1e-6f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(100.0f, static_cast<float>(wall.getSegmentA().y), 1e-6f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(140.0f, static_cast<float>(wall.getSegmentB().x), 1e-6f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(120.0f, static_cast<float>(wall.getSegmentB().y), 1e-6f);
}

void test_segment_hitbox_covers_whole_segment(void) {
    SegmentMockWall wall(100.0f, 100.0f, 0.0f, 0.0f, 40.0f, 20.0f);
    Rect box = wall.getHitBox();
    float left = static_cast<float>(box.position.x);
    float top = static_cast<float>(box.position.y);
    float right = left + static_cast<float>(box.width);
    float bottom = top + static_cast<float>(box.height);
    TEST_ASSERT_TRUE(left <= 100.0f && right >= 140.0f);
    TEST_ASSERT_TRUE(top <= 100.0f && bottom >= 120.0f);
}

// =============================================================================
// Perpendicular hit: normal reflection, tangent preserved
// =============================================================================

void test_perpendicular_hit_reflects_normal_only(void) {
    CollisionSystem system;
    // Horizontal wall (80,100)-(120,100). Circle r=5, center (102,97):
    // 3 units above the wall, overlapping by 2.
    SegmentMockWall wall(80.0f, 100.0f, 0.0f, 0.0f, 40.0f, 0.0f);
    SegmentMockCircle ball(97.0f, 92.0f, 5.0f);
    ball.setVelocity(30.0f, 60.0f);  // Into the wall, with tangent drift.

    system.addEntity(&ball);
    system.addEntity(&wall);
    system.update();

    TEST_ASSERT_TRUE(ball.collisionCalled);
    TEST_ASSERT_TRUE(wall.collisionCalled);
    // Restitution defaults to 1: normal (y) flips, tangent (x) is unchanged.
    TEST_ASSERT_FLOAT_EQUAL_EPS(-60.0f, static_cast<float>(ball.getVelocityY()), 1.0f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(30.0f, static_cast<float>(ball.getVelocityX()), 1.0f);
    // Penetration correction pushed the circle back out above the wall.
    float centerY = static_cast<float>(ball.position.y) + 5.0f;
    TEST_ASSERT_TRUE(centerY < 100.0f);
}

// Same geometry with the registration order swapped: the contact normal must
// still point from the segment toward the circle (bodyB flip path).
void test_segment_as_second_body_still_reflects(void) {
    CollisionSystem system;
    SegmentMockWall wall(80.0f, 100.0f, 0.0f, 0.0f, 40.0f, 0.0f);
    SegmentMockCircle ball(97.0f, 92.0f, 5.0f);
    ball.setVelocity(30.0f, 60.0f);

    system.addEntity(&wall);
    system.addEntity(&ball);
    system.update();

    TEST_ASSERT_TRUE(ball.collisionCalled);
    TEST_ASSERT_TRUE(wall.collisionCalled);
    TEST_ASSERT_FLOAT_EQUAL_EPS(-60.0f, static_cast<float>(ball.getVelocityY()), 1.0f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(30.0f, static_cast<float>(ball.getVelocityX()), 1.0f);
}

// =============================================================================
// 45-degree wall: reflection across the diagonal
// =============================================================================

void test_45_degree_wall_reflects(void) {
    CollisionSystem system;
    // Wall (100,120)-(120,100), slope -1. Circle r=5, center (112,112):
    // closest point (110,110), overlap by 5 - 2*sqrt(2).
    SegmentMockWall wall(100.0f, 100.0f, 0.0f, 20.0f, 20.0f, 0.0f);
    SegmentMockCircle ball(107.0f, 107.0f, 5.0f);
    ball.setVelocity(-60.0f, -60.0f);  // Straight into the wall along -normal.

    system.addEntity(&ball);
    system.addEntity(&wall);
    system.update();

    TEST_ASSERT_TRUE(ball.collisionCalled);
    // v' = v - 2(v.n)n with n = (1,1)/sqrt(2): (-60,-60) -> (60,60).
    TEST_ASSERT_FLOAT_EQUAL_EPS(60.0f, static_cast<float>(ball.getVelocityX()), 1.5f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(60.0f, static_cast<float>(ball.getVelocityY()), 1.5f);
}

// =============================================================================
// Corner joint: two segments sharing an end point, no pass-through
// =============================================================================

void test_corner_joint_does_not_let_circle_through(void) {
    CollisionSystem system;
    // L corner at (120,100): horizontal (100,100)-(120,100),
    // vertical (120,100)-(120,120).
    SegmentMockWall horizontal(100.0f, 100.0f, 0.0f, 0.0f, 20.0f, 0.0f);
    SegmentMockWall vertical(120.0f, 100.0f, 0.0f, 0.0f, 0.0f, 20.0f);
    SegmentMockCircle ball(105.0f, 85.0f, 5.0f);  // Center (110,90).
    ball.setVelocity(60.0f, 60.0f);  // Aimed at the shared corner.

    system.addEntity(&ball);
    system.addEntity(&horizontal);
    system.addEntity(&vertical);
    for (int i = 0; i < 30; ++i) {
        system.update();
    }

    TEST_ASSERT_TRUE(ball.collisionCalled);
    float cx = static_cast<float>(ball.position.x) + 5.0f;
    float cy = static_cast<float>(ball.position.y) + 5.0f;
    // Never tunneled past the corner into the bottom-right region.
    TEST_ASSERT_FALSE(cx > 125.0f && cy > 105.0f);
    // Elastic bounce (restitution 1, no friction): speed preserved.
    float vx = static_cast<float>(ball.getVelocityX());
    float vy = static_cast<float>(ball.getVelocityY());
    float speed = std::sqrt(vx * vx + vy * vy);
    TEST_ASSERT_FLOAT_EQUAL_EPS(std::sqrt(60.0f * 60.0f * 2.0f), speed, 2.0f);
}

// =============================================================================
// Degenerate segment: zero length behaves as a point
// =============================================================================

void test_zero_length_segment_behaves_as_point(void) {
    CollisionSystem system;
    SegmentMockWall point(50.0f, 50.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    SegmentMockCircle ball(47.0f, 47.0f, 5.0f);  // Center (52,52), dist ~2.8 < 5.
    ball.setVelocity(0.0f, 0.0f);

    system.addEntity(&ball);
    system.addEntity(&point);
    system.update();

    TEST_ASSERT_TRUE(ball.collisionCalled);
    TEST_ASSERT_TRUE(point.collisionCalled);
}

// =============================================================================
// Non-circle segment pairs produce no contact
// =============================================================================

void test_aabb_vs_segment_produces_no_contact(void) {
    CollisionSystem system;
    SegmentMockWall wall(80.0f, 100.0f, 0.0f, 0.0f, 40.0f, 0.0f);
    SegmentMockBox box(95.0f, 95.0f, 10, 10);  // Hitbox overlaps the segment.
    box.setVelocity(0.0f, 0.0f);

    system.addEntity(&box);
    system.addEntity(&wall);
    system.update();

    TEST_ASSERT_FALSE(box.collisionCalled);
    TEST_ASSERT_FALSE(wall.collisionCalled);
}

void test_segment_vs_segment_produces_no_contact(void) {
    CollisionSystem system;
    SegmentMockWall wallA(80.0f, 100.0f, 0.0f, 0.0f, 40.0f, 0.0f);
    SegmentMockWall wallB(90.0f, 90.0f, 0.0f, 20.0f, 20.0f, 0.0f);

    system.addEntity(&wallA);
    system.addEntity(&wallB);
    system.update();

    TEST_ASSERT_FALSE(wallA.collisionCalled);
    TEST_ASSERT_FALSE(wallB.collisionCalled);
}

// =============================================================================
// checkCollision query with segments
// =============================================================================

void test_check_collision_circle_vs_segment(void) {
    CollisionSystem system;
    SegmentMockWall wall(80.0f, 100.0f, 0.0f, 0.0f, 40.0f, 0.0f);
    SegmentMockCircle near(97.0f, 92.0f, 5.0f);
    SegmentMockCircle far(10.0f, 10.0f, 5.0f);

    system.addEntity(&near);
    system.addEntity(&far);
    system.addEntity(&wall);

    Actor* out[4];
    int count = 0;
    TEST_ASSERT_TRUE(system.checkCollision(&near, out, count, 4));
    TEST_ASSERT_EQUAL_INT(1, count);
    TEST_ASSERT_EQUAL_PTR(&wall, out[0]);

    count = 0;
    TEST_ASSERT_FALSE(system.checkCollision(&far, out, count, 4));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_segment_endpoints_are_world_coordinates);
    RUN_TEST(test_segment_hitbox_covers_whole_segment);
    RUN_TEST(test_perpendicular_hit_reflects_normal_only);
    RUN_TEST(test_segment_as_second_body_still_reflects);
    RUN_TEST(test_45_degree_wall_reflects);
    RUN_TEST(test_corner_joint_does_not_let_circle_through);
    RUN_TEST(test_zero_length_segment_behaves_as_point);
    RUN_TEST(test_aabb_vs_segment_produces_no_contact);
    RUN_TEST(test_segment_vs_segment_produces_no_contact);
    RUN_TEST(test_check_collision_circle_vs_segment);

    return UNITY_END();
}
