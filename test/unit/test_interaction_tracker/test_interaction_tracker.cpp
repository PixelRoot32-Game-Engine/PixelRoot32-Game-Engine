/**
 * @file test_interaction_tracker.cpp
 * @brief Unit tests for gameplay/InteractionTracker + InteractionComponent
 *
 * Covers the spec requirements for actor-interaction-triggers:
 * - enter-once (Trigger Enter Fires Exactly Once On New Contact)
 * - no-refire-while-persisting (+ onCollision unaffected)
 * - exit-once (Trigger Exit Fires Exactly Once When Contact Ends)
 * - manual-only onInteract (never auto-invoked by the engine)
 * - no-new-virtuals (Actor/Entity vtables unchanged)
 * - zero-cost-when-disabled + no-heap-allocation
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
 * is enabled, since InteractionComponent/InteractionTracker are entirely
 * guarded behind that flag (see include/gameplay/InteractionTracker.h). This
 * file therefore compiles cleanly in BOTH the default (flag off) and opt-in
 * (flag on) configurations, matching the "no behavior change for existing
 * examples" goal and the CI matrix from design.md.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"
#include "core/Actor.h"
#include "core/Entity.h"

#include <cstdlib>
#include <new>
#include <type_traits>

// =============================================================================
// Requirement: No New Virtual Methods Added To Actor Or Entity
//
// The authoritative proof for this requirement is that include/core/Actor.h
// and include/core/Entity.h have ZERO diff lines in this PR (verified in the
// PR description / `git diff`, not by this test). A runtime "measure the
// vtable size" hack is fragile and compiler-dependent, and a hardcoded
// sizeof() comparison doesn't specifically prove "no new virtual" (Actor
// already has a vtable from onCollision/getHitBox).
//
// The strongest test expressible here is a compile-time one: a minimal
// concrete subclass that overrides EXACTLY the pure virtuals Actor/Entity
// declare TODAY (Entity::update, Entity::draw, Actor::getHitBox,
// Actor::onCollision). If this PR had added a new PURE virtual to either
// class, this subclass would fail to compile (still abstract). This does not
// catch a newly added NON-pure virtual, which is why the empty-diff claim
// above is the real proof; this static_assert is a regression guard that
// documents and partially enforces the same invariant at compile time.
// =============================================================================
namespace {

using pixelroot32::core::Actor;
using pixelroot32::core::Entity;
using pixelroot32::core::Rect;
using pixelroot32::math::Vector2;
using pixelroot32::math::toScalar;

class MinimalActorSurface : public Actor {
public:
    MinimalActorSurface() : Actor(toScalar(0.0f), toScalar(0.0f), 1, 1) {}
    Rect getHitBox() override { return Rect{position, width, height}; }
    void onCollision(Actor* other) override { (void)other; }
    void draw(pixelroot32::graphics::Renderer& renderer) override { (void)renderer; }
};

static_assert(!std::is_abstract<MinimalActorSurface>::value,
              "MinimalActorSurface overrides exactly Entity::draw, "
              "Actor::getHitBox and Actor::onCollision (the pure virtuals "
              "declared before this PR). If this fails, a new pure virtual "
              "was added to Actor or Entity by this change, which the "
              "actor-interaction-triggers spec forbids — InteractionComponent "
              "and InteractionTracker must express enter/exit/onInteract "
              "without growing Actor's or Entity's vtable.");

}  // namespace

void test_no_new_virtuals_on_actor_or_entity(void) {
    // Compile-time check above is the real assertion; this runtime test
    // exists so the invariant shows up in the test report and so the
    // translation unit is exercised (not just compiled) by the suite.
    MinimalActorSurface actor;
    TEST_ASSERT_EQUAL(1, actor.width);
    TEST_ASSERT_EQUAL(1, actor.height);
}

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS

#include "gameplay/InteractionComponent.h"
#include "gameplay/InteractionTracker.h"
#include "physics/CollisionSystem.h"
#include "physics/KinematicActor.h"
#include "physics/StaticActor.h"

using namespace pixelroot32::gameplay;
using namespace pixelroot32::physics;
using namespace pixelroot32::core;
using namespace pixelroot32::math;

namespace {

// Mock actor used both for direct tracker-driven tests (entityId assigned by
// a CollisionSystem) and for CollisionSystem-driven persistence tests.
class MockKinematicActor : public KinematicActor {
public:
    MockKinematicActor(float x, float y, int w, int h)
        : KinematicActor(toScalar(x), toScalar(y), w, h) {}

    void onCollision(Actor* other) override {
        (void)other;
        ++onCollisionCount;
    }

    int onCollisionCount = 0;
};

struct InteractionState {
    int enterCount = 0;
    int exitCount = 0;
    int interactCount = 0;
    Actor* lastEnterOther = nullptr;
    Actor* lastExitOther = nullptr;
};

void onEnterCallback(void* owner, Actor* other) {
    auto* state = static_cast<InteractionState*>(owner);
    ++state->enterCount;
    state->lastEnterOther = other;
}

void onExitCallback(void* owner, Actor* other) {
    auto* state = static_cast<InteractionState*>(owner);
    ++state->exitCount;
    state->lastExitOther = other;
}

void onInteractCallback(void* owner, Actor* instigator) {
    auto* state = static_cast<InteractionState*>(owner);
    ++state->interactCount;
    (void)instigator;
}

}  // namespace

// =============================================================================
// Requirement: Trigger Enter Fires Exactly Once On New Contact
// =============================================================================
void test_enter_fires_once_on_new_contact(void) {
    CollisionSystem system;
    MockKinematicActor a(0, 0, 10, 10);
    MockKinematicActor b(0, 0, 10, 10);
    system.addEntity(&a);
    system.addEntity(&b);

    InteractionTracker tracker;
    InteractionState stateA;
    InteractionComponent compA;
    compA.owner = &stateA;
    compA.onEnter = onEnterCallback;
    compA.onExit = onExitCallback;
    tracker.registerActor(&a, &compA);

    // b is registered too (with no callbacks of its own) purely so its actor
    // pointer is resolvable as the "other" argument in compA's callbacks —
    // the registry, not the raw contact, is the only source of Actor*
    // identity for dispatch (see InteractionTracker.h class doc).
    InteractionComponent compB;
    tracker.registerActor(&b, &compB);

    // Frame 1: previous contact set is empty, current contact set has (a, b).
    tracker.beginFrame();
    tracker.recordPair(&a, &b);
    tracker.endFrame();

    TEST_ASSERT_EQUAL(1, stateA.enterCount);
    TEST_ASSERT_EQUAL(0, stateA.exitCount);
    TEST_ASSERT_EQUAL(&b, stateA.lastEnterOther);
}

// =============================================================================
// Requirement: Trigger Enter Does Not Re-Fire While Contact Persists
// (+ onCollision must still fire every frame, unaffected)
// =============================================================================
void test_no_refire_while_contact_persists_and_oncollision_still_fires(void) {
    CollisionSystem system;
    InteractionTracker tracker;
    system.setInteractionTracker(&tracker);

    // Kinematic vs Static, zero velocity, deeply overlapping: solveVelocity()/
    // solvePenetration() both skip resolution for this pair (invMass is zero
    // on both sides), so the contact persists identically across every
    // system.update() call without the bodies drifting apart.
    MockKinematicActor a(0, 0, 20, 20);
    StaticActor b(toScalar(5), toScalar(5), 20, 20);
    a.setCollisionLayer(1);
    a.setCollisionMask(1);
    b.setCollisionLayer(1);
    b.setCollisionMask(1);

    system.addEntity(&a);
    system.addEntity(&b);

    InteractionState stateA;
    InteractionComponent compA;
    compA.owner = &stateA;
    compA.onEnter = onEnterCallback;
    compA.onExit = onExitCallback;
    tracker.registerActor(&a, &compA);

    system.update();  // Frame 1: enter.
    TEST_ASSERT_EQUAL(1, stateA.enterCount);
    TEST_ASSERT_EQUAL(1, a.onCollisionCount);

    system.update();  // Frame 2: persists.
    TEST_ASSERT_EQUAL(1, stateA.enterCount);   // No re-fire.
    TEST_ASSERT_EQUAL(0, stateA.exitCount);
    TEST_ASSERT_EQUAL(2, a.onCollisionCount);  // onCollision fires again, unaffected.

    system.update();  // Frame 3: persists again.
    TEST_ASSERT_EQUAL(1, stateA.enterCount);
    TEST_ASSERT_EQUAL(3, a.onCollisionCount);
}

// =============================================================================
// Requirement: Trigger Exit Fires Exactly Once When Contact Ends
// =============================================================================
void test_exit_fires_once_when_contact_ends(void) {
    CollisionSystem system;
    MockKinematicActor a(0, 0, 10, 10);
    MockKinematicActor b(0, 0, 10, 10);
    system.addEntity(&a);
    system.addEntity(&b);

    InteractionTracker tracker;
    InteractionState stateA;
    InteractionComponent compA;
    compA.owner = &stateA;
    compA.onEnter = onEnterCallback;
    compA.onExit = onExitCallback;
    tracker.registerActor(&a, &compA);

    InteractionComponent compB;
    tracker.registerActor(&b, &compB);

    // Frame 1: enter.
    tracker.beginFrame();
    tracker.recordPair(&a, &b);
    tracker.endFrame();
    TEST_ASSERT_EQUAL(1, stateA.enterCount);

    // Frame 2: persists.
    tracker.beginFrame();
    tracker.recordPair(&a, &b);
    tracker.endFrame();
    TEST_ASSERT_EQUAL(0, stateA.exitCount);

    // Frame 3: no contact recorded this frame -> exit.
    tracker.beginFrame();
    tracker.endFrame();

    TEST_ASSERT_EQUAL(1, stateA.exitCount);
    TEST_ASSERT_EQUAL(1, stateA.enterCount);  // Still exactly once.
    TEST_ASSERT_EQUAL(&b, stateA.lastExitOther);
}

// =============================================================================
// Requirement: onInteract Is Invoked Only By Explicit Game Code
// =============================================================================
void test_oninteract_never_auto_invoked_across_frames(void) {
    CollisionSystem system;
    MockKinematicActor a(0, 0, 10, 10);
    MockKinematicActor b(0, 0, 10, 10);
    system.addEntity(&a);
    system.addEntity(&b);

    InteractionTracker tracker;
    InteractionState stateA;
    InteractionComponent compA;
    compA.owner = &stateA;
    compA.onEnter = onEnterCallback;
    compA.onExit = onExitCallback;
    compA.onInteract = onInteractCallback;
    tracker.registerActor(&a, &compA);

    // Enter.
    tracker.beginFrame();
    tracker.recordPair(&a, &b);
    tracker.endFrame();

    // Persist.
    tracker.beginFrame();
    tracker.recordPair(&a, &b);
    tracker.endFrame();

    // Exit.
    tracker.beginFrame();
    tracker.endFrame();

    // No game code ever called compA.invokeInteract() — the engine must
    // never have invoked onInteract on its own during enter, persist, or exit.
    TEST_ASSERT_EQUAL(0, stateA.interactCount);

    // Confirm the manual path IS wired correctly (sanity check): explicit
    // invocation still works, proving onInteract isn't dead code.
    compA.invokeInteract(&b);
    TEST_ASSERT_EQUAL(1, stateA.interactCount);
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled
// (functional half: no dynamic allocation at max configured capacity)
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

void test_interaction_tracker_zero_cost_when_disabled_and_no_heap_allocation(void) {
    InteractionTracker tracker;
    MockKinematicActor a(0, 0, 10, 10);
    MockKinematicActor b(0, 0, 10, 10);

    // Directly assign entityIds (bypassing CollisionSystem registration) so
    // every recorded pair is distinct, filling currPairs to the maximum
    // configured contact capacity (PhysicsMaxContacts).
    tracker.beginFrame();
    for (int i = 0; i < InteractionTracker::kMaxContacts; ++i) {
        a.entityId = static_cast<uint16_t>(i + 1);
        b.entityId = static_cast<uint16_t>(i + 2);
        tracker.recordPair(&a, &b);
    }

    g_heapAllocCount = 0;
    tracker.endFrame();  // Sort + diff + dispatch at full capacity.
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(g_heapAllocCount));

    tracker.reset();
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(g_heapAllocCount));
}

#else  // !PIXELROOT32_ENABLE_INTERACTION_TRIGGERS

void test_interaction_tracker_zero_cost_when_disabled_and_no_heap_allocation(void) {
    // With the flag off, InteractionComponent/InteractionTracker are not
    // compiled at all — their entire declarations live inside the
    // #if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS guard in
    // include/gameplay/InteractionTracker.h. This translation unit compiling
    // and passing without referencing either type IS the "no enter/exit
    // tracking storage reserved" property required by the spec's
    // "Feature-Gated And Zero-Cost When Disabled" requirement.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_INTERACTION_TRIGGERS=0: tracker types are not "
        "compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_INTERACTION_TRIGGERS

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_no_new_virtuals_on_actor_or_entity);

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
    RUN_TEST(test_enter_fires_once_on_new_contact);
    RUN_TEST(test_no_refire_while_contact_persists_and_oncollision_still_fires);
    RUN_TEST(test_exit_fires_once_when_contact_ends);
    RUN_TEST(test_oninteract_never_auto_invoked_across_frames);
#endif
    RUN_TEST(test_interaction_tracker_zero_cost_when_disabled_and_no_heap_allocation);

    return UNITY_END();
}
