/**
 * @file test_physics_limits.cpp
 * @brief Unit tests for physics capacity-limit reporting (issue #243).
 *
 * Pushes each fixed-size physics buffer past its limit in a debug build and
 * checks that the overflow is reported via the drop counters. Also pins the
 * default of the newly configurable PHYSICS_MAX_CANDIDATES_PER_BODY flag.
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

// Mock Actor for testing
class LimitMockActor : public RigidActor {
public:
    bool collisionCalled = false;

    LimitMockActor(float x, float y, int w, int h) : RigidActor(toScalar(x), toScalar(y), w, h) {
        setCollisionLayer(1);
        setCollisionMask(1);
    }

    void onCollision(Actor* other) override {
        (void)other;
        collisionCalled = true;
    }

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
// PHYSICS_MAX_ENTITIES
// =============================================================================

void test_entity_overflow_is_reported(void) {
    CollisionSystem system;
    system.resetLimitDropCounters();

    std::vector<LimitMockActor> actors;
    actors.reserve(70);
    for (int i = 0; i < 70; ++i) {
        actors.emplace_back(0.0f, 0.0f, 10, 10);
    }
    for (int i = 0; i < 70; ++i) {
        system.addEntity(&actors[static_cast<size_t>(i)]);
    }

    TEST_ASSERT_EQUAL_INT(64, static_cast<int>(system.getEntityCount()));
    TEST_ASSERT_EQUAL_UINT(6, system.getDroppedEntityCount());
}

// =============================================================================
// PHYSICS_MAX_CONTACTS
// =============================================================================

void test_contact_overflow_is_reported(void) {
    CollisionSystem system;
    system.resetLimitDropCounters();

    // Five 32px cells filled to exactly 12 mutually overlapping bodies each:
    // 8 small 10x10 bodies per cell plus 2 wide 40x10 bodies straddling each
    // internal boundary (shared with the neighbour cell). ~280 distinct
    // overlapping pairs, contacts capped at 128, every body inside the
    // logical screen so no clamping, no per-cell drops, no entity overflow.
    std::vector<LimitMockActor> smalls;
    smalls.reserve(40);
    for (int cell = 0; cell < 5; ++cell) {
        for (int i = 0; i < 8; ++i) {
            smalls.emplace_back(static_cast<float>(cell * 32 + 10), 0.0f, 10, 10);
        }
    }
    std::vector<LimitMockActor> wides;
    wides.reserve(8);
    for (int boundary = 1; boundary <= 4; ++boundary) {
        for (int i = 0; i < 2; ++i) {
            wides.emplace_back(static_cast<float>(boundary * 32 - 15), 0.0f, 40, 10);
        }
    }
    for (size_t i = 0; i < smalls.size(); ++i) {
        system.addEntity(&smalls[i]);
    }
    for (size_t i = 0; i < wides.size(); ++i) {
        system.addEntity(&wides[i]);
    }
    system.update();

    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedEntityCount());
    TEST_ASSERT_EQUAL_UINT(0, SpatialGrid::getDroppedDynamicInserts());
    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedCandidateCount());
    TEST_ASSERT_GREATER_THAN_UINT(0, system.getDroppedContactCount());
}

// =============================================================================
// SPATIAL_GRID_MAX_STATIC_PER_CELL / SPATIAL_GRID_MAX_DYNAMIC_PER_CELL
// =============================================================================

void test_static_per_cell_overflow_is_reported(void) {
    CollisionSystem system;
    system.resetLimitDropCounters();

    // 14 static 10x10 bodies in a single 32px cell (cap 12) -> 2 drops.
    // STATIC vs STATIC never generates contacts, isolating the grid limit.
    std::vector<StaticActor> statics;
    statics.reserve(14);
    for (int i = 0; i < 14; ++i) {
        statics.emplace_back(toScalar(0.0f), toScalar(0.0f), 10, 10);
    }
    for (int i = 0; i < 14; ++i) {
        statics[static_cast<size_t>(i)].setCollisionLayer(1);
        statics[static_cast<size_t>(i)].setCollisionMask(1);
        system.addEntity(&statics[static_cast<size_t>(i)]);
    }
    system.update();

    TEST_ASSERT_EQUAL_UINT(2, SpatialGrid::getDroppedStaticInserts());
    TEST_ASSERT_EQUAL_UINT(0, SpatialGrid::getDroppedDynamicInserts());
    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedContactCount());
}

void test_dynamic_per_cell_overflow_is_reported(void) {
    CollisionSystem system;
    system.resetLimitDropCounters();

    // 14 dynamic 10x10 bodies in a single 32px cell (cap 12) -> 2 drops.
    // The 12 registered bodies yield 66 contacts, below the 128 cap.
    std::vector<LimitMockActor> actors;
    actors.reserve(14);
    for (int i = 0; i < 14; ++i) {
        actors.emplace_back(0.0f, 0.0f, 10, 10);
    }
    for (int i = 0; i < 14; ++i) {
        system.addEntity(&actors[static_cast<size_t>(i)]);
    }
    system.update();

    TEST_ASSERT_EQUAL_UINT(2, SpatialGrid::getDroppedDynamicInserts());
    TEST_ASSERT_EQUAL_UINT(0, SpatialGrid::getDroppedStaticInserts());
    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedContactCount());
}

// =============================================================================
// PHYSICS_MAX_CANDIDATES_PER_BODY
// =============================================================================

void test_candidate_buffer_default_and_no_truncation(void) {
    // New config value (issue #243): per-body candidate buffer, default 64.
    TEST_ASSERT_EQUAL_INT(64, pixelroot32::platforms::config::PhysicsMaxCandidatesPerBody);

    CollisionSystem system;
    system.resetLimitDropCounters();

    std::vector<LimitMockActor> actors;
    actors.reserve(14);
    for (int i = 0; i < 14; ++i) {
        actors.emplace_back(0.0f, 0.0f, 10, 10);
    }
    for (int i = 0; i < 14; ++i) {
        system.addEntity(&actors[static_cast<size_t>(i)]);
    }
    system.update();

    // 13 candidates max per body: below the 64 cap, nothing truncated.
    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedCandidateCount());
}

// =============================================================================
// Reset
// =============================================================================

void test_limit_drop_counters_reset(void) {
    CollisionSystem system;

    std::vector<LimitMockActor> actors;
    actors.reserve(70);
    for (int i = 0; i < 70; ++i) {
        actors.emplace_back(0.0f, 0.0f, 10, 10);
    }
    for (int i = 0; i < 70; ++i) {
        system.addEntity(&actors[static_cast<size_t>(i)]);
    }
    TEST_ASSERT_GREATER_THAN_UINT(0, system.getDroppedEntityCount());

    system.resetLimitDropCounters();

    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedEntityCount());
    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedContactCount());
    TEST_ASSERT_EQUAL_UINT(0, system.getDroppedCandidateCount());
    TEST_ASSERT_EQUAL_UINT(0, SpatialGrid::getDroppedStaticInserts());
    TEST_ASSERT_EQUAL_UINT(0, SpatialGrid::getDroppedDynamicInserts());
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_entity_overflow_is_reported);
    RUN_TEST(test_contact_overflow_is_reported);
    RUN_TEST(test_static_per_cell_overflow_is_reported);
    RUN_TEST(test_dynamic_per_cell_overflow_is_reported);
    RUN_TEST(test_candidate_buffer_default_and_no_truncation);
    RUN_TEST(test_limit_drop_counters_reset);

    return UNITY_END();
}
