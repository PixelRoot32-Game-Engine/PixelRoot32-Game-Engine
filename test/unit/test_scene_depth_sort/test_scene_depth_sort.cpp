/**
 * @file test_scene_depth_sort.cpp
 * @brief Unit tests for the scene-depth-sort capability:
 *        Scene::depthComparator / Scene::depthSortEnabled / Scene::shouldPrecede()
 *        and the gameplay::compareByBottomY() convenience comparator.
 *
 * Covers the spec requirements for scene-depth-sort:
 * - byte-identical-default (Default Sort Behavior Is Byte-Identical When No
 *   Comparator Is Set)
 * - comparator-orders-within-layer (Comparator Orders Entities Within The
 *   Same Render Layer)
 * - re-sort-every-frame-when-enabled (Scene Re-Sorts Every Frame When Depth
 *   Sort Is Enabled)
 * - disabled-preserves-behavior-and-cost (Depth Sort Disabled Preserves
 *   Existing Behavior And Cost)
 * - feature-gated-and-zero-cost (Feature-Gated And Zero-Cost When Disabled)
 *
 * Unlike the other 3 Phase-1 capabilities (gameplay-event-bus,
 * actor-interaction-triggers, spatial-area-query), design.md D5 deliberately
 * does NOT gate Scene::depthComparator/depthSortEnabled/shouldPrecede() behind
 * `#if PIXELROOT32_ENABLE_DEPTH_SORT` — the per-Scene cost (one null-defaulted
 * function pointer plus one bool, ~8 B) is cheap enough that it does not need
 * its storage macro-gated away the way the 512 B+ event bus and interaction
 * tracker do (see design.md D5's "cheapest of all 4 capabilities" rationale).
 * The functional tests below therefore compile and run unconditionally,
 * proving the capability's runtime guarantees (byte-identical when unset,
 * comparator ordering, per-frame re-sort, disabled cost) regardless of the
 * build-time flag. Only the zero-cost/no-heap-allocation test below follows
 * the `#if FLAG ... #else ... #endif` structure established in the other 3
 * capability test files, to match the CI matrix convention from design.md.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"
#include "core/Scene.h"
#include "core/Entity.h"
#include "gameplay/DepthCompare.h"
#include "graphics/Renderer.h"

#include <cstdlib>
#include <new>

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;
using namespace pixelroot32::math;

// =============================================================================
// Shared test fixtures
// =============================================================================

// Mock Entity implementation (mirrors test/unit/test_scene/test_scene.cpp's
// MockEntity — same construction convention, reused here so this file needs
// no cross-file dependency).
class MockEntity : public Entity {
public:
    MockEntity(float x, float y, int w, int h, EntityType t = EntityType::GENERIC)
        : Entity(x, y, w, h, t) {}

    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(Renderer& renderer) override { (void)renderer; }
};

// Exposes Scene's protected depth-sort surface for testing, exactly like
// test_scene.cpp's TestScene subclass exposes sortEntities()/entities[].
class DepthSortTestScene : public Scene {
public:
    Entity** getEntities() { return entities; }
    int getEntityCount() const { return entityCount; }
    void forceSort() { sortEntities(); }
    void setComparator(Scene::DepthComparator cmp) { depthComparator = cmp; }
    void enableDepthSort(bool enabled) { depthSortEnabled = enabled; }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Requirement: Default Sort Behavior Is Byte-Identical When No Comparator Is
// Set
// =============================================================================
void test_default_sort_byte_identical_no_comparator(void) {
    DepthSortTestScene scene;

    // 6 entities across 3 render layers, 2 per layer, added in an order that
    // is NOT already layer-sorted. With no comparator set (the default), the
    // insertion-sort predicate falls through to `false` on equal layers
    // exactly as today's raw `renderLayer` comparison does — so ties must
    // keep their original insertion-relative order.
    MockEntity a(0, 0, 10, 10);  // layer 2, added 1st
    MockEntity b(0, 0, 10, 10);  // layer 0, added 2nd
    MockEntity c(0, 0, 10, 10);  // layer 2, added 3rd
    MockEntity d(0, 0, 10, 10);  // layer 1, added 4th
    MockEntity e(0, 0, 10, 10);  // layer 0, added 5th
    MockEntity f(0, 0, 10, 10);  // layer 1, added 6th

    a.setRenderLayer(2);
    b.setRenderLayer(0);
    c.setRenderLayer(2);
    d.setRenderLayer(1);
    e.setRenderLayer(0);
    f.setRenderLayer(1);

    scene.addEntity(&a);
    scene.addEntity(&b);
    scene.addEntity(&c);
    scene.addEntity(&d);
    scene.addEntity(&e);
    scene.addEntity(&f);

    scene.forceSort();

    TEST_ASSERT_EQUAL(6, scene.getEntityCount());
    Entity** sorted = scene.getEntities();

    // Expected: today's stable insertion sort by renderLayer only —
    // layer 0: b, e ; layer 1: d, f ; layer 2: a, c
    TEST_ASSERT_EQUAL_PTR(&b, sorted[0]);
    TEST_ASSERT_EQUAL_PTR(&e, sorted[1]);
    TEST_ASSERT_EQUAL_PTR(&d, sorted[2]);
    TEST_ASSERT_EQUAL_PTR(&f, sorted[3]);
    TEST_ASSERT_EQUAL_PTR(&a, sorted[4]);
    TEST_ASSERT_EQUAL_PTR(&c, sorted[5]);
}

// =============================================================================
// Requirement: Comparator Orders Entities Within The Same Render Layer
// =============================================================================
void test_comparator_orders_entities_within_same_layer(void) {
    DepthSortTestScene scene;

    // Same render layer, added with the "wrong" bottom-Y order so a
    // layer-only sort would leave them untouched — proving any reordering
    // observed is caused by the comparator, not by the layer comparison.
    MockEntity lower(0, 50, 10, 5);   // bottom = 50 + 5 = 55, added 1st
    MockEntity higher(0, 5, 10, 5);   // bottom = 5 + 5 = 10, added 2nd

    lower.setRenderLayer(1);
    higher.setRenderLayer(1);

    scene.addEntity(&lower);
    scene.addEntity(&higher);

    scene.setComparator(&pixelroot32::gameplay::compareByBottomY);
    scene.enableDepthSort(true);
    scene.forceSort();

    Entity** sorted = scene.getEntities();
    TEST_ASSERT_EQUAL_PTR(&higher, sorted[0]);  // smaller bottom-Y draws first
    TEST_ASSERT_EQUAL_PTR(&lower, sorted[1]);
}

// =============================================================================
// Requirement: Scene Re-Sorts Every Frame When Depth Sort Is Enabled
// =============================================================================
void test_scene_resorts_every_frame_when_depth_sort_enabled(void) {
    DepthSortTestScene scene;
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    Renderer renderer(config);

    MockEntity e1(0, 5, 10, 5);    // bottom = 10, added 1st
    MockEntity e2(0, 50, 10, 5);   // bottom = 55, added 2nd
    e1.setRenderLayer(1);
    e2.setRenderLayer(1);

    scene.addEntity(&e1);
    scene.addEntity(&e2);
    scene.setComparator(&pixelroot32::gameplay::compareByBottomY);
    scene.enableDepthSort(true);

    // First draw(): needsSorting is true from addEntity() (and
    // depthSortEnabled is also true), so this sorts and resets needsSorting
    // to false. Initial order (e1 bottom=10, e2 bottom=55) is already
    // correctly ordered, so no visible change here — this call exists only
    // to reach the "needsSorting == false" state the requirement targets.
    scene.draw(renderer);
    TEST_ASSERT_EQUAL_PTR(&e1, scene.getEntities()[0]);
    TEST_ASSERT_EQUAL_PTR(&e2, scene.getEntities()[1]);

    // Simulate a frame where only e1's position.y changed (e.g. gameplay
    // movement) — this does NOT dirty needsSorting (per design.md D5,
    // needsSorting is only ever set in addEntity()).
    e1.position.y = toScalar(100.0f);  // new bottom = 105, now greater than e2's 55

    // Second draw(): needsSorting is false, but depthSortEnabled is true, so
    // sortEntities() MUST re-run anyway and reflect the updated position.
    scene.draw(renderer);
    TEST_ASSERT_EQUAL_PTR(&e2, scene.getEntities()[0]);
    TEST_ASSERT_EQUAL_PTR(&e1, scene.getEntities()[1]);
}

// =============================================================================
// Requirement: Depth Sort Disabled Preserves Existing Behavior And Cost
// =============================================================================
void test_disabled_preserves_existing_behavior_and_cost(void) {
    DepthSortTestScene scene;
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    Renderer renderer(config);

    // 9 entities across 3 render layers, 3 per layer — resembling the
    // multi-layer / several-entities-per-layer patterns used across the
    // 15 existing examples under examples/.
    MockEntity e0(0, 0, 10, 10), e1(0, 0, 10, 10), e2(0, 0, 10, 10);
    MockEntity e3(0, 0, 10, 10), e4(0, 0, 10, 10), e5(0, 0, 10, 10);
    MockEntity e6(0, 0, 10, 10), e7(0, 0, 10, 10), e8(0, 0, 10, 10);
    Entity* pool[9] = {&e0, &e1, &e2, &e3, &e4, &e5, &e6, &e7, &e8};
    for (int i = 0; i < 9; ++i) {
        pool[i]->setRenderLayer(static_cast<unsigned char>(i % 3));
        scene.addEntity(pool[i]);
    }

    // depthSortEnabled left at its default (false) — no call to
    // enableDepthSort() or setComparator() anywhere in this test.
    scene.draw(renderer);  // needsSorting was true (from addEntity), so this sorts once.

    // Directly scramble the now-sorted array WITHOUT going through
    // addEntity(), so needsSorting stays false (it is only ever set inside
    // addEntity() — design.md D5). depthSortEnabled also stays false.
    Entity** raw = scene.getEntities();
    Entity* snapshotBeforeScramble[9];
    for (int i = 0; i < 9; ++i) snapshotBeforeScramble[i] = raw[i];
    Entity* tmp = raw[0];
    raw[0] = raw[8];
    raw[8] = tmp;

    // A second draw() with both needsSorting and depthSortEnabled false MUST
    // NOT re-run sortEntities() — matching today's pre-capability cost and
    // behavior exactly (draw() only sorted `if (needsSorting)` before this
    // change).
    scene.draw(renderer);

    Entity** afterSecondDraw = scene.getEntities();
    TEST_ASSERT_EQUAL_PTR(snapshotBeforeScramble[8], afterSecondDraw[0]);
    TEST_ASSERT_EQUAL_PTR(snapshotBeforeScramble[0], afterSecondDraw[8]);
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled
// (functional half: no dynamic allocation at MAX_ENTITIES capacity)
//
// The Scene depth-sort surface itself is not gated behind
// PIXELROOT32_ENABLE_DEPTH_SORT (see the file header comment / design.md D5),
// so this property holds unconditionally. This test still follows the
// #if FLAG ... #else ... #endif structure established in the other 3
// capability test files (test_gameplay_event_bus.cpp,
// test_interaction_tracker.cpp, test_spatial_query.cpp) to match the CI
// matrix convention from design.md; the #else branch documents why, rather
// than claiming a storage difference that does not exist for this capability.
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

#if PIXELROOT32_ENABLE_DEPTH_SORT

namespace {
// MockEntity has no default constructor; a thin default-constructible
// wrapper lets us build a fixed-size MAX_ENTITIES pool (mirrors
// QueryTestActor in test_spatial_query.cpp).
struct PoolEntity : public MockEntity {
    PoolEntity() : MockEntity(0.0f, 0.0f, 8, 8) {}
};
}  // namespace

void test_depth_sort_zero_cost_when_disabled_and_no_heap_allocation(void) {
    DepthSortTestScene scene;
    constexpr int kCount = pixelroot32::platforms::config::MaxEntities;

    static PoolEntity pool[kCount];
    for (int i = 0; i < kCount; ++i) {
        // Spread across a few layers so the comparator is actually
        // consulted for same-layer runs, and vary Y so bottom-Y comparisons
        // are non-trivial.
        pool[i].setRenderLayer(static_cast<unsigned char>(i % 4));
        pool[i].position.y = toScalar(static_cast<float>((kCount - i) * 2));
        scene.addEntity(&pool[i]);
    }

    scene.setComparator(&pixelroot32::gameplay::compareByBottomY);
    scene.enableDepthSort(true);

    g_heapAllocCount = 0;
    scene.forceSort();  // Full sort at MAX_ENTITIES capacity, comparator active.

    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(g_heapAllocCount));
    TEST_ASSERT_EQUAL(kCount, scene.getEntityCount());
}

#else  // !PIXELROOT32_ENABLE_DEPTH_SORT

void test_depth_sort_zero_cost_when_disabled_and_no_heap_allocation(void) {
    // PIXELROOT32_ENABLE_DEPTH_SORT is off (the default). Unlike the other 3
    // capabilities, Scene::depthComparator/depthSortEnabled/shouldPrecede()
    // are NOT gated behind this flag in include/core/Scene.h — design.md D5
    // deliberately keeps them unconditional because their total cost (one
    // null-defaulted function pointer plus one bool, ~8 B per Scene) is
    // cheap enough not to need macro-gating, unlike the 512 B+ event bus and
    // interaction tracker. The zero-heap-allocation proof above therefore
    // holds regardless of this flag; this branch is a trivial pass-path that
    // documents that fact and keeps this test file structurally consistent
    // with the #if/#else convention used by the other 3 capability test
    // files and the CI matrix in design.md.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_DEPTH_SORT=0: Scene's depth-sort members are "
        "unconditional (design.md D5) and already proven zero-heap-alloc "
        "when the flag is on; nothing additional is gated off here.");
}

#endif  // PIXELROOT32_ENABLE_DEPTH_SORT

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_default_sort_byte_identical_no_comparator);
    RUN_TEST(test_comparator_orders_entities_within_same_layer);
    RUN_TEST(test_scene_resorts_every_frame_when_depth_sort_enabled);
    RUN_TEST(test_disabled_preserves_existing_behavior_and_cost);
    RUN_TEST(test_depth_sort_zero_cost_when_disabled_and_no_heap_allocation);

    return UNITY_END();
}
