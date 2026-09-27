/**
 * @file test_position_correction.cpp
 * @brief Unit tests for penetration position correction, BIAS and SLOP (issue #245).
 *
 * Pins the BIAS/SLOP defaults and the per-step correction mechanism:
 * each step removes exactly (penetration - SLOP) * BIAS, so a larger BIAS
 * separates overlapping bodies in fewer steps by construction. Convergence
 * stops at the SLOP floor.
 */

#include <unity.h>
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
class CorrectionMockCircle : public RigidActor {
public:
    CorrectionMockCircle(float x, float y, float r)
        : RigidActor(toScalar(x), toScalar(y), 16, 16) {
        setShape(CollisionShape::CIRCLE);
        setRadius(toScalar(r));
        setCollisionLayer(1);
        setCollisionMask(1);
        setGravityScale(toScalar(0.0f));
    }

    void onCollision(Actor* other) override { (void)other; }
    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }
};

static float centerDistance(const PhysicsActor& a, const PhysicsActor& b) {
    float ax = static_cast<float>(a.position.x) + static_cast<float>(a.getRadius());
    float ay = static_cast<float>(a.position.y) + static_cast<float>(a.getRadius());
    float bx = static_cast<float>(b.position.x) + static_cast<float>(b.getRadius());
    float by = static_cast<float>(b.position.y) + static_cast<float>(b.getRadius());
    float dx = ax - bx;
    float dy = ay - by;
    return std::sqrt(dx * dx + dy * dy);
}

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Defaults (issue #245: flags keep the current values)
// =============================================================================

void test_bias_slop_defaults(void) {
    // Epsilon covers the Fixed16 quantum (~1.5e-5) as well as float.
    TEST_ASSERT_FLOAT_EQUAL_EPS(0.2f, static_cast<float>(CollisionSystem::BIAS), 1e-3f);
    TEST_ASSERT_FLOAT_EQUAL_EPS(0.02f, static_cast<float>(CollisionSystem::SLOP), 1e-3f);
    TEST_ASSERT_EQUAL_INT(2, CollisionSystem::VELOCITY_ITERATIONS);
}

// =============================================================================
// Per-step correction equals (penetration - SLOP) * BIAS
// =============================================================================

void test_single_step_correction_matches_bias(void) {
    CollisionSystem system;
    StaticActor wall(toScalar(100.0f), toScalar(100.0f), 16, 16);
    wall.setShape(CollisionShape::CIRCLE);
    wall.setRadius(toScalar(8.0f));
    wall.setCollisionLayer(1);
    wall.setCollisionMask(1);
    // Rigid center (108, 120): distance 12, penetration 16 - 12 = 4.
    CorrectionMockCircle ball(100.0f, 112.0f, 8.0f);

    system.addEntity(&wall);
    system.addEntity(&ball);
    system.update();

    // Correction = (4 - 0.02) * 0.2 = 0.796 applied fully to the rigid body.
    float dist = centerDistance(wall, ball);
    TEST_ASSERT_FLOAT_EQUAL_EPS(12.796f, dist, 1e-3f);
}

// =============================================================================
// Convergence is bounded and stops at the SLOP floor
// =============================================================================

void test_overlapping_circles_separate_within_bounded_steps(void) {
    CollisionSystem system;
    StaticActor wall(toScalar(100.0f), toScalar(100.0f), 16, 16);
    wall.setShape(CollisionShape::CIRCLE);
    wall.setRadius(toScalar(8.0f));
    wall.setCollisionLayer(1);
    wall.setCollisionMask(1);
    CorrectionMockCircle ball(100.0f, 112.0f, 8.0f);

    system.addEntity(&wall);
    system.addEntity(&ball);
    for (int i = 0; i < 200; ++i) {
        system.update();
    }

    // Penetration converged to the SLOP floor: distance >= 16 - 0.02 - eps.
    float dist = centerDistance(wall, ball);
    TEST_ASSERT_TRUE(dist >= 16.0f - 0.02f - 1e-2f);
}

void test_correction_stops_at_slop_floor(void) {
    CollisionSystem system;
    StaticActor wall(toScalar(100.0f), toScalar(100.0f), 16, 16);
    wall.setShape(CollisionShape::CIRCLE);
    wall.setRadius(toScalar(8.0f));
    wall.setCollisionLayer(1);
    wall.setCollisionMask(1);
    CorrectionMockCircle ball(100.0f, 112.0f, 8.0f);

    system.addEntity(&wall);
    system.addEntity(&ball);
    for (int i = 0; i < 200; ++i) {
        system.update();
    }
    float settled = centerDistance(wall, ball);
    for (int i = 0; i < 50; ++i) {
        system.update();
    }
    float later = centerDistance(wall, ball);

    // No drift past the floor once penetration <= SLOP.
    TEST_ASSERT_FLOAT_EQUAL_EPS(settled, later, 1e-3f);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_bias_slop_defaults);
    RUN_TEST(test_single_step_correction_matches_bias);
    RUN_TEST(test_overlapping_circles_separate_within_bounded_steps);
    RUN_TEST(test_correction_stops_at_slop_floor);

    return UNITY_END();
}
