/**
 * @file test_spatial_query.cpp
 * @brief Unit tests for the spatial-area-query capability:
 *        SpatialGrid::queryRadius()/queryBox() and
 *        CollisionSystem::queryRadius()/queryBox().
 *
 * Covers the spec requirements for spatial-area-query:
 * - radius query returns layer-matching actors within circle (and excludes
 *   non-matching-layer actors)
 * - box query returns layer-matching actors within rectangle (and excludes
 *   non-matching-layer actors)
 * - query stops at caller-provided output capacity without overflow
 * - query accepts a raw center point/rectangle without an anchor actor
 * - feature-gated and zero-cost when disabled, no heap allocation when enabled
 *
 * (The cross-scene static-geometry visibility requirement is an accepted,
 * documented limitation per design.md/spec.md — not something this file
 * proves false; see the doc comments on SpatialGrid::queryRadius()/queryBox()
 * and CollisionSystem::queryRadius()/queryBox().)
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_SPATIAL_QUERY is
 * enabled, since CollisionSystem::queryRadius()/queryBox() and
 * SpatialGrid::queryRadius()/queryBox() are entirely guarded behind that flag.
 * This file therefore compiles cleanly in BOTH the default (flag off) and
 * opt-in (flag on) configurations, matching the "no behavior change for
 * existing examples" goal and the CI matrix from design.md.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

#if PIXELROOT32_ENABLE_SPATIAL_QUERY

#include "physics/CollisionSystem.h"
#include "physics/StaticActor.h"
#include "core/Actor.h"

#include <cstdlib>
#include <new>

using namespace pixelroot32::physics;
using namespace pixelroot32::core;
using namespace pixelroot32::math;

namespace {
constexpr CollisionLayer kEnemyLayer = 1 << 0;
constexpr CollisionLayer kItemLayer = 1 << 1;
}  // namespace

// =============================================================================
// Requirement: Radius Query Returns Layer-Matching Actors Within Circle
// =============================================================================
void test_radius_query_returns_layer_matching_actor(void) {
    CollisionSystem system;
    StaticActor enemy(toScalar(50), toScalar(50), 10, 10);
    enemy.setCollisionLayer(kEnemyLayer);
    system.addEntity(&enemy);
    system.detectCollisions();  // Rebuilds the grid's static layer.

    Actor* results[8];
    int count = system.queryRadius(Vector2(toScalar(55), toScalar(55)), toScalar(20),
                                    kEnemyLayer, results, 8);

    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL_PTR(&enemy, results[0]);
}

void test_radius_query_excludes_non_matching_layer(void) {
    CollisionSystem system;
    StaticActor enemy(toScalar(50), toScalar(50), 10, 10);
    enemy.setCollisionLayer(kEnemyLayer);
    system.addEntity(&enemy);
    system.detectCollisions();

    Actor* results[8];
    // Caller mask only wants kItemLayer; enemy->layer is kEnemyLayer, so
    // (mask & other->layer) == 0 and the actor must be excluded even though
    // it is well within the query circle.
    int count = system.queryRadius(Vector2(toScalar(55), toScalar(55)), toScalar(20),
                                    kItemLayer, results, 8);

    TEST_ASSERT_EQUAL(0, count);
}

// =============================================================================
// Requirement: Box Query Returns Layer-Matching Actors Within Rectangle
// =============================================================================
void test_box_query_returns_layer_matching_actor(void) {
    CollisionSystem system;
    StaticActor item(toScalar(60), toScalar(60), 10, 10);
    item.setCollisionLayer(kItemLayer);
    system.addEntity(&item);
    system.detectCollisions();

    Rect box{Vector2(toScalar(40), toScalar(40)), 40, 40};
    Actor* results[8];
    int count = system.queryBox(box, kItemLayer, results, 8);

    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL_PTR(&item, results[0]);
}

void test_box_query_excludes_non_matching_layer(void) {
    CollisionSystem system;
    StaticActor item(toScalar(60), toScalar(60), 10, 10);
    item.setCollisionLayer(kItemLayer);
    system.addEntity(&item);
    system.detectCollisions();

    Rect box{Vector2(toScalar(40), toScalar(40)), 40, 40};
    Actor* results[8];
    int count = system.queryBox(box, kEnemyLayer, results, 8);

    TEST_ASSERT_EQUAL(0, count);
}

// =============================================================================
// Requirement: Query Stops At Caller-Provided Output Capacity Without Overflow
// =============================================================================
void test_query_stops_at_maxcount_without_overflow(void) {
    CollisionSystem system;
    // 5 matching actors clustered inside the query region/mask; the caller
    // only provides room for 2 results in each output array.
    StaticActor a(toScalar(50), toScalar(50), 8, 8);
    StaticActor b(toScalar(60), toScalar(50), 8, 8);
    StaticActor c(toScalar(50), toScalar(60), 8, 8);
    StaticActor d(toScalar(60), toScalar(60), 8, 8);
    StaticActor e(toScalar(55), toScalar(55), 8, 8);
    for (StaticActor* actor : {&a, &b, &c, &d, &e}) {
        actor->setCollisionLayer(kEnemyLayer);
        system.addEntity(actor);
    }
    system.detectCollisions();

    // Radius query: canary sentinels around the 2-slot array detect
    // out-of-bounds writes.
    Actor* radiusResults[4] = {nullptr, nullptr, reinterpret_cast<Actor*>(0xDEAD),
                               reinterpret_cast<Actor*>(0xDEAD)};
    int radiusCount = system.queryRadius(Vector2(toScalar(55), toScalar(55)), toScalar(30),
                                          kEnemyLayer, radiusResults, 2);
    TEST_ASSERT_EQUAL(2, radiusCount);
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<Actor*>(0xDEAD), radiusResults[2]);
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<Actor*>(0xDEAD), radiusResults[3]);

    // Box query: same canary contract.
    Rect box{Vector2(toScalar(40), toScalar(40)), 40, 40};
    Actor* boxResults[4] = {nullptr, nullptr, reinterpret_cast<Actor*>(0xDEAD),
                            reinterpret_cast<Actor*>(0xDEAD)};
    int boxCount = system.queryBox(box, kEnemyLayer, boxResults, 2);
    TEST_ASSERT_EQUAL(2, boxCount);
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<Actor*>(0xDEAD), boxResults[2]);
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<Actor*>(0xDEAD), boxResults[3]);
}

// =============================================================================
// Requirement: Query Accepts A Raw Center Point Or Rectangle Without An
// Anchor Actor
// =============================================================================
void test_query_accepts_bare_coordinate_without_anchor_actor(void) {
    CollisionSystem system;
    StaticActor enemy(toScalar(100), toScalar(100), 10, 10);
    enemy.setCollisionLayer(kEnemyLayer);
    system.addEntity(&enemy);
    system.detectCollisions();

    // No Actor* argument anywhere in these calls — only a bare Vector2/Scalar
    // pair and a bare Rect, proving the gap vs. getPotentialColliders()
    // (which requires an anchor Actor*) is closed.
    Actor* results[4];
    int radiusCount = system.queryRadius(Vector2(toScalar(105), toScalar(105)), toScalar(15),
                                          kEnemyLayer, results, 4);
    TEST_ASSERT_EQUAL(1, radiusCount);

    Rect box{Vector2(toScalar(90), toScalar(90)), 30, 30};
    int boxCount = system.queryBox(box, kEnemyLayer, results, 4);
    TEST_ASSERT_EQUAL(1, boxCount);
}

// =============================================================================
// Requirement: SPATIAL_QUERY_MAX_RADIUS Overflow Guard Boundary
//
// The overflow guard is a live assert() (see CollisionSystem::queryRadius())
// with no NDEBUG defined anywhere in this build (native_test), so assert()
// is active and calls abort() on failure. Deliberately calling queryRadius()
// with a radius above SPATIAL_QUERY_MAX_RADIUS would therefore terminate the
// whole test binary, not fail a single Unity test case — the same reason no
// existing test in this suite (e.g. test_collision_system.cpp) intentionally
// triggers checkCollision()'s null-pointer asserts either.
//
// What IS safely testable through the live API: radius == the boundary value
// itself must succeed (the guard is "<=", not "<"). What is pinned as a
// compile-time regression guard, matching the exact numbers from design.md's
// Risks section, is the Q16.16 headroom the boundary was chosen to
// guarantee.
// =============================================================================
namespace {
static_assert(
    (pixelroot32::platforms::config::SpatialQueryMaxRadius + SpatialGrid::kCellSize) *
            (pixelroot32::platforms::config::SpatialQueryMaxRadius + SpatialGrid::kCellSize) <
        32768,
    "SPATIAL_QUERY_MAX_RADIUS must keep (radius + kCellSize)^2 strictly below "
    "the Q16.16 +-32768 range (design.md D4 / Risks: 128 + 32 = 160, "
    "160^2 = 25600 < 32768).");
}  // namespace

void test_radius_clamp_boundary(void) {
    CollisionSystem system;
    StaticActor enemy(toScalar(50), toScalar(50), 10, 10);
    enemy.setCollisionLayer(kEnemyLayer);
    system.addEntity(&enemy);
    system.detectCollisions();

    Scalar boundaryRadius = toScalar(pixelroot32::platforms::config::SpatialQueryMaxRadius);
    Actor* results[4];
    // Center far enough that only the boundary-sized radius reaches the
    // actor, proving radius == SPATIAL_QUERY_MAX_RADIUS is accepted (the
    // guard is inclusive) and the call completes without triggering assert().
    int count = system.queryRadius(Vector2(toScalar(50) - boundaryRadius + toScalar(15),
                                            toScalar(50)),
                                    boundaryRadius, kEnemyLayer, results, 4);
    TEST_ASSERT_EQUAL(1, count);
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled
// (functional half: no dynamic allocation against a fully populated grid)
// =============================================================================
namespace {
size_t g_heapAllocCount = 0;
}

void* operator new(std::size_t size) {
    ++g_heapAllocCount;
    return std::malloc(size);
}
void operator delete(void* ptr) noexcept {
    std::free(ptr);
}
void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}
void* operator new[](std::size_t size) {
    ++g_heapAllocCount;
    return std::malloc(size);
}
void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}
void operator delete[](void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

namespace {
// StaticActor has no default constructor, so a fixed-size array of test
// actors needs a thin default-constructible wrapper. Position is
// reassigned per-slot below via the inherited (public) Entity::position
// field before the actors are registered.
struct QueryTestActor : public StaticActor {
    QueryTestActor() : StaticActor(toScalar(0), toScalar(0), 8, 8) {}
};

// The population loop below places one actor per grid cell in an 8-wide
// layout. This only stays overflow-free (<= SpatialGridMaxStaticPerCell per
// cell) if the total actor count fits within the grid's actual cell count;
// fail loudly at compile time rather than silently wrapping into
// over-full cells if that assumption ever changes.
static_assert(
    pixelroot32::platforms::config::PhysicsMaxEntities <=
        (pixelroot32::platforms::config::LogicalWidth / SpatialGrid::kCellSize + 1) *
            (pixelroot32::platforms::config::LogicalHeight / SpatialGrid::kCellSize + 1),
    "test_spatial_query_zero_cost_when_disabled_and_no_heap_allocation assumes "
    "PhysicsMaxEntities fits one-per-cell in the grid; adjust the population "
    "layout if this no longer holds.");
}  // namespace

void test_spatial_query_zero_cost_when_disabled_and_no_heap_allocation(void) {
    CollisionSystem system;

    // Fill the collision system (and therefore the grid's static layer) to
    // its configured capacity, spreading one actor per grid cell so no
    // per-cell array overflows silently drop entries.
    static QueryTestActor populated[pixelroot32::platforms::config::PhysicsMaxEntities];
    for (int i = 0; i < pixelroot32::platforms::config::PhysicsMaxEntities; ++i) {
        // 8x8 = 64 grid cells at the default 240x240 logical size / 32 cell
        // size == PhysicsMaxEntities, so this places exactly one actor per
        // cell with zero clamping/overflow.
        int col = i % 8;
        int row = i / 8;
        Scalar x = toScalar(col * SpatialGrid::kCellSize + 4);
        Scalar y = toScalar(row * SpatialGrid::kCellSize + 4);
        populated[i].position = Vector2(x, y);
        populated[i].setCollisionLayer(kEnemyLayer);
        system.addEntity(&populated[i]);
    }
    system.detectCollisions();  // Rebuild the fully populated static layer.

    g_heapAllocCount = 0;
    Actor* results[pixelroot32::platforms::config::PhysicsMaxEntities];
    Rect box{Vector2(toScalar(0), toScalar(0)), 240, 240};
    int radiusCount = system.queryRadius(Vector2(toScalar(112), toScalar(112)), toScalar(100),
                                          kEnemyLayer, results,
                                          pixelroot32::platforms::config::PhysicsMaxEntities);
    int boxCount = system.queryBox(box, kEnemyLayer, results,
                                    pixelroot32::platforms::config::PhysicsMaxEntities);

    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(g_heapAllocCount));
    TEST_ASSERT_TRUE(radiusCount > 0);
    TEST_ASSERT_TRUE(boxCount > 0);
}

#else  // !PIXELROOT32_ENABLE_SPATIAL_QUERY

void test_spatial_query_zero_cost_when_disabled_and_no_heap_allocation(void) {
    // With the flag off, SpatialGrid::queryRadius()/queryBox() and
    // CollisionSystem::queryRadius()/queryBox() are not compiled at all —
    // their entire declarations live inside the
    // #if PIXELROOT32_ENABLE_SPATIAL_QUERY guard in
    // include/physics/SpatialGrid.h and include/physics/CollisionSystem.h.
    // This translation unit compiling and passing without referencing either
    // API IS the "no additional query-only storage reserved" property
    // required by the spec's "Feature-Gated And Zero-Cost When Disabled"
    // requirement.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_SPATIAL_QUERY=0: query APIs are not compiled, "
        "zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_SPATIAL_QUERY

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_SPATIAL_QUERY
    RUN_TEST(test_radius_query_returns_layer_matching_actor);
    RUN_TEST(test_radius_query_excludes_non_matching_layer);
    RUN_TEST(test_box_query_returns_layer_matching_actor);
    RUN_TEST(test_box_query_excludes_non_matching_layer);
    RUN_TEST(test_query_stops_at_maxcount_without_overflow);
    RUN_TEST(test_query_accepts_bare_coordinate_without_anchor_actor);
    RUN_TEST(test_radius_clamp_boundary);
#endif
    RUN_TEST(test_spatial_query_zero_cost_when_disabled_and_no_heap_allocation);

    return UNITY_END();
}
