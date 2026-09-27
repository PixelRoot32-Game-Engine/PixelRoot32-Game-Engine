/**
 * @file test_physics_rest.cpp
 * @brief Unit tests for the rest threshold and at-rest queries (issue #246).
 *
 * One suite proves both halves of the contract through if-constexpr branches
 * on RigidActor::kRestThreshold: under builds with the threshold at 0
 * (e.g. native_test) the default branch pins unchanged behavior, and under
 * builds with PHYSICS_REST_THRESHOLD set (e.g. native_test_physics_rest) the
 * threshold branch pins snap-to-rest behavior.
 */

#include <unity.h>
#include <vector>
#include "../../test_config.h"
#include "physics/CollisionSystem.h"
#include "physics/RigidActor.h"
#include "physics/StaticActor.h"
#include "core/Actor.h"
#include "core/PhysicsActor.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

// Mock rigid body for testing
class RestMockBody : public RigidActor {
public:
    RestMockBody(float x, float y, int w, int h) : RigidActor(toScalar(x), toScalar(y), w, h) {
        setCollisionLayer(1);
        setCollisionMask(1);
        setGravityScale(toScalar(0.0f));
    }

    void onCollision(Actor* other) override { (void)other; }
    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }
};

static bool velocityIsExactlyZero(const PhysicsActor& body) {
    return body.getVelocityX() == toScalar(0) && body.getVelocityY() == toScalar(0);
}

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Queries (both builds)
// =============================================================================

void test_is_at_rest_basics(void) {
    RestMockBody moving(0.0f, 0.0f, 10, 10);
    moving.setVelocity(10.0f, 0.0f);
    TEST_ASSERT_FALSE(moving.isAtRest());

    RestMockBody still(0.0f, 0.0f, 10, 10);
    TEST_ASSERT_TRUE(still.isAtRest());

    StaticActor wall(toScalar(0.0f), toScalar(0.0f), 10, 10);
    TEST_ASSERT_TRUE(wall.isAtRest());
}

void test_all_bodies_at_rest_query(void) {
    CollisionSystem system;
    TEST_ASSERT_TRUE(system.allBodiesAtRest());

    RestMockBody a(0.0f, 0.0f, 10, 10);
    RestMockBody b(50.0f, 0.0f, 10, 10);
    b.setVelocity(10.0f, 0.0f);
    system.addEntity(&a);
    system.addEntity(&b);
    TEST_ASSERT_FALSE(system.allBodiesAtRest());

    b.setVelocity(0.0f, 0.0f);
    TEST_ASSERT_TRUE(system.allBodiesAtRest());
}

// =============================================================================
// Default branch: threshold 0, behavior unchanged
// =============================================================================

void test_no_threshold_creeping_continues(void) {
    if constexpr (RigidActor::kRestThreshold > toScalar(0)) {
        TEST_IGNORE_MESSAGE("Threshold set: covered by test_threshold_snaps_sliding_body");
        return;
    } else {
        RestMockBody body(0.0f, 0.0f, 10, 10);
        body.setFriction(toScalar(1.0f));
        body.setVelocity(300.0f, 0.0f);
        for (int i = 0; i < 600; ++i) {
            body.integrate(CollisionSystem::FIXED_DT);
        }
        // Proportional friction never reaches exact zero: still moving.
        TEST_ASSERT_FALSE(velocityIsExactlyZero(body));
        TEST_ASSERT_FALSE(body.isAtRest());
    }
}

// =============================================================================
// Threshold branch: snap-to-rest behavior
// =============================================================================

void test_threshold_snaps_sliding_body(void) {
    if constexpr (RigidActor::kRestThreshold > toScalar(0)) {
        RestMockBody body(0.0f, 0.0f, 10, 10);
        body.setFriction(toScalar(1.0f));
        body.setVelocity(300.0f, 0.0f);
        int steps = -1;
        for (int i = 0; i < 600; ++i) {
            body.integrate(CollisionSystem::FIXED_DT);
            if (velocityIsExactlyZero(body)) {
                steps = i + 1;
                break;
            }
        }
        // Bounded: friction decays 300 below the threshold, then it snaps.
        TEST_ASSERT_TRUE(steps > 0);
        TEST_ASSERT_TRUE(body.isAtRest());
        // Stays at rest with no applied force.
        for (int i = 0; i < 10; ++i) {
            body.integrate(CollisionSystem::FIXED_DT);
        }
        TEST_ASSERT_TRUE(velocityIsExactlyZero(body));
    } else {
        TEST_IGNORE_MESSAGE("Threshold 0: covered by test_no_threshold_creeping_continues");
    }
}

void test_threshold_boundary_snaps(void) {
    if constexpr (RigidActor::kRestThreshold > toScalar(0)) {
        RestMockBody body(0.0f, 0.0f, 10, 10);
        body.setFriction(toScalar(1.0f));
        body.setVelocity(RigidActor::kRestThreshold, toScalar(0));
        body.integrate(CollisionSystem::FIXED_DT);
        // Friction pulls it strictly below the threshold, then it snaps.
        TEST_ASSERT_TRUE(velocityIsExactlyZero(body));
    } else {
        TEST_IGNORE_MESSAGE("Threshold 0: boundary branches to the default path");
    }
}

void test_threshold_ignores_forced_body(void) {
    if constexpr (RigidActor::kRestThreshold > toScalar(0)) {
        RestMockBody body(0.0f, 0.0f, 10, 10);
        body.setFriction(toScalar(1.0f));
        body.setVelocity(1.0f, 0.0f);  // Below the threshold, but pushed.
        for (int i = 0; i < 10; ++i) {
            body.applyForce(Vector2(toScalar(0.0f), toScalar(-50.0f)));
            body.integrate(CollisionSystem::FIXED_DT);
        }
        // A body with an applied force never snaps.
        TEST_ASSERT_FALSE(velocityIsExactlyZero(body));
    } else {
        TEST_IGNORE_MESSAGE("Threshold 0: covered by test_no_threshold_creeping_continues");
    }
}

void test_all_bodies_at_rest_after_snap(void) {
    if constexpr (RigidActor::kRestThreshold > toScalar(0)) {
        CollisionSystem system;
        RestMockBody slider(0.0f, 0.0f, 10, 10);
        slider.setFriction(toScalar(1.0f));
        slider.setVelocity(300.0f, 0.0f);
        RestMockBody still(200.0f, 200.0f, 10, 10);
        system.addEntity(&slider);
        system.addEntity(&still);
        TEST_ASSERT_FALSE(system.allBodiesAtRest());

        int steps = -1;
        for (int i = 0; i < 600; ++i) {
            system.update();
            if (system.allBodiesAtRest()) {
                steps = i + 1;
                break;
            }
        }
        TEST_ASSERT_TRUE(steps > 0);
    } else {
        TEST_IGNORE_MESSAGE("Threshold 0: covered by test_all_bodies_at_rest_query");
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_is_at_rest_basics);
    RUN_TEST(test_all_bodies_at_rest_query);
    RUN_TEST(test_no_threshold_creeping_continues);
    RUN_TEST(test_threshold_snaps_sliding_body);
    RUN_TEST(test_threshold_boundary_snaps);
    RUN_TEST(test_threshold_ignores_forced_body);
    RUN_TEST(test_all_bodies_at_rest_after_snap);

    return UNITY_END();
}
