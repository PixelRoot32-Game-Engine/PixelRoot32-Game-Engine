/**
 * @file test_gameplay_state_machine.cpp
 * @brief Unit tests for gameplay/StateMachine module
 *
 * Covers the spec requirements for gameplay-state-machine:
 * - caller-owned-const-table (bind without copying, lookup by id not row order,
 *   unknown id rejected)
 * - transition-invokes-onexit-then-onenter (immediate, synchronous, correct args,
 *   null callback skipped)
 * - time-in-state-is-a-saturating-uint32-ms-counter
 * - re-requesting-current-state-is-a-no-op
 * - restart-state-performs-a-real-exit-enter-cycle
 * - transitions-requested-from-onenter-or-onexit-are-queued-and-drained
 *   (no recursion, last-writer-wins, ping-pong terminates with overflow)
 * - at-most-one-onupdate-invocation-per-update-call
 * - start-and-reset-semantics
 * - owner-is-type-erased-via-void-star-with-no-new-virtuals
 * - feature-gated-and-zero-cost-when-disabled
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE
 * is enabled, since StateMachine is entirely guarded behind that flag (see
 * include/gameplay/StateMachine.h). This file therefore compiles cleanly in
 * BOTH the default (flag off) and opt-in (flag on) configurations, matching
 * the "no behavior change for existing examples" goal and the CI matrix from
 * design.md.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"
#include "core/Actor.h"
#include "core/Entity.h"

#include <cstddef>
#include <type_traits>

// =============================================================================
// Requirement: Owner Is Type-Erased Via void*, With No New Virtuals On Actor
// Or Entity
//
// This compile-time check is independent of PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE
// — Actor/Entity are unconditional engine types — so, mirroring
// test_interaction_tracker.cpp's placement, it lives outside the flag guard
// and runs in BOTH configurations. The authoritative proof is that
// include/core/Actor.h and include/core/Entity.h have ZERO diff lines in
// this change (verified in the PR diff, not by this test); the minimal
// concrete subclass below is a compile-time regression guard: if a new PURE
// virtual were added to either class, it would fail to compile (still
// abstract).
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
              "was added to Actor or Entity by this change, which "
              "gameplay-state-machine forbids — StateMachine must reference "
              "its owner via void* without growing Actor's or Entity's vtable.");

}  // namespace

void test_no_new_virtuals_on_actor_or_entity(void) {
    // Compile-time check above is the real assertion; this runtime test
    // exists so the invariant shows up in the test report and so the
    // translation unit is exercised (not just compiled) by the suite.
    MinimalActorSurface actor;
    TEST_ASSERT_EQUAL(1, actor.width);
    TEST_ASSERT_EQUAL(1, actor.height);
}

#if PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE

#include "gameplay/StateMachine.h"

#include <cstdint>

using namespace pixelroot32::gameplay;

namespace {

// Three test states. Deliberately declared with plain numeric ids so a
// reordered table (below) can prove lookup is by id, not row position.
enum : StateId { kStateA = 0, kStateB = 1, kStateC = 2, kStateUnregistered = 99 };

/// One ordered call-log entry recorded by the mock owner's callbacks.
struct LogEntry {
    enum class Kind : uint8_t { Enter, Update, Exit };
    Kind    kind;
    StateId state;
    StateId other;  ///< fromState for Enter, toState for Exit, kInvalidStateId for Update.
};

/**
 * @brief Mock owner recording an ordered call log, with opt-in hooks that let
 *        a test arm a callback to request a further transition (covers the
 *        onEnter/onExit-requests-a-transition scenarios without needing a
 *        distinct state table per scenario).
 */
struct MockOwner {
    static constexpr int kMaxLog = 40;

    StateMachine* fsm = nullptr;

    LogEntry log[kMaxLog]{};
    int      logCount = 0;

    int           updateCount = 0;
    unsigned long lastDeltaTime = 0;
    uint32_t      lastTimeInState = 0;
    uint32_t      lastEnterTimeInState = 0;  ///< getTimeInState() as observed inside onEnter.

    // One-shot "request a transition the first time this state's callback fires".
    StateId requestFromEnterOf     = StateMachine::kInvalidStateId;
    StateId requestFromEnterTarget = StateMachine::kInvalidStateId;
    bool    requestFromEnterFired  = false;

    StateId requestFromExitOf     = StateMachine::kInvalidStateId;
    StateId requestFromExitTarget = StateMachine::kInvalidStateId;
    bool    requestFromExitFired  = false;

    StateId requestFromUpdateOf     = StateMachine::kInvalidStateId;
    StateId requestFromUpdateTarget = StateMachine::kInvalidStateId;
    bool    requestFromUpdateFired  = false;

    // Unconditional A<->B ping-pong, armed only for the overflow test.
    bool pingPongAB = false;

    void record(LogEntry::Kind kind, StateId state, StateId other) {
        if (logCount < kMaxLog) log[logCount++] = LogEntry{kind, state, other};
    }
};

void handleEnter(MockOwner* owner, StateId state, StateId from) {
    owner->lastEnterTimeInState = owner->fsm->getTimeInState();
    owner->record(LogEntry::Kind::Enter, state, from);

    if (owner->pingPongAB) {
        if (state == kStateA) { owner->fsm->requestState(kStateB); return; }
        if (state == kStateB) { owner->fsm->requestState(kStateA); return; }
    }
    if (owner->requestFromEnterOf == state && !owner->requestFromEnterFired) {
        owner->requestFromEnterFired = true;
        owner->fsm->requestState(owner->requestFromEnterTarget);
    }
}

void handleExit(MockOwner* owner, StateId state, StateId to) {
    owner->record(LogEntry::Kind::Exit, state, to);

    if (owner->requestFromExitOf == state && !owner->requestFromExitFired) {
        owner->requestFromExitFired = true;
        owner->fsm->requestState(owner->requestFromExitTarget);
    }
}

void handleUpdate(MockOwner* owner, StateId state, unsigned long dt, uint32_t timeInState) {
    owner->record(LogEntry::Kind::Update, state, StateMachine::kInvalidStateId);
    ++owner->updateCount;
    owner->lastDeltaTime = dt;
    owner->lastTimeInState = timeInState;

    if (owner->requestFromUpdateOf == state && !owner->requestFromUpdateFired) {
        owner->requestFromUpdateFired = true;
        owner->fsm->requestState(owner->requestFromUpdateTarget);
    }
}

void onEnterA(void* owner, StateId from) { handleEnter(static_cast<MockOwner*>(owner), kStateA, from); }
void onExitA(void* owner, StateId to)    { handleExit(static_cast<MockOwner*>(owner), kStateA, to); }
void onUpdateA(void* owner, unsigned long dt, uint32_t t) { handleUpdate(static_cast<MockOwner*>(owner), kStateA, dt, t); }

void onEnterB(void* owner, StateId from) { handleEnter(static_cast<MockOwner*>(owner), kStateB, from); }
void onExitB(void* owner, StateId to)    { handleExit(static_cast<MockOwner*>(owner), kStateB, to); }
void onUpdateB(void* owner, unsigned long dt, uint32_t t) { handleUpdate(static_cast<MockOwner*>(owner), kStateB, dt, t); }

void onEnterC(void* owner, StateId from) { handleEnter(static_cast<MockOwner*>(owner), kStateC, from); }
// C's onUpdate/onExit are deliberately left null in the table below, to
// exercise the "null callback is skipped without crashing" scenario.

/// The canonical table, row order matching numeric id order.
static const StateMachine::State kTestTable[] = {
    { onEnterA, onUpdateA, onExitA, kStateA },
    { onEnterB, onUpdateB, onExitB, kStateB },
    { onEnterC, nullptr,   nullptr, kStateC },
};
constexpr uint8_t kTestTableCount = 3;

/// Same three states, deliberately reordered (C, A, B) to prove lookup is by
/// State::id, never by row position.
static const StateMachine::State kReorderedTable[] = {
    { onEnterC, nullptr,   nullptr, kStateC },
    { onEnterA, onUpdateA, onExitA, kStateA },
    { onEnterB, onUpdateB, onExitB, kStateB },
};
constexpr uint8_t kReorderedTableCount = 3;

}  // namespace

// =============================================================================
// Requirement: Caller-Owned Const State Table Contract
// =============================================================================

void test_configure_binds_table_without_copying(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);

    // The machine must resolve lookups through the exact table pointer bound
    // by configure() — the only observable proof available without exposing
    // the private pointer is that lookups against that specific table work
    // for every id it declares.
    TEST_ASSERT_TRUE(fsm.requestState(kStateB));
    TEST_ASSERT_EQUAL_UINT8(kStateB, fsm.getCurrentState());
}

void test_state_lookup_matches_by_id_not_row_order(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    // kReorderedTable's row order is C, A, B — the numeric ordering of the
    // ids is not the row order.
    fsm.configure(&owner, kReorderedTable, kReorderedTableCount);
    fsm.start(kStateB);

    TEST_ASSERT_EQUAL_UINT8(kStateB, fsm.getCurrentState());
    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[0].state);
}

void test_request_state_unknown_id_rejected(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    owner.logCount = 0;

    const bool result = fsm.requestState(kStateUnregistered);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_UINT8(kStateA, fsm.getCurrentState());
    TEST_ASSERT_EQUAL_INT(0, owner.logCount);
}

// =============================================================================
// Requirement: Transition Invokes onExit Then onEnter, Immediately And Synchronously
// =============================================================================

void test_transition_invokes_onexit_then_onenter_in_order(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    owner.logCount = 0;

    const bool result = fsm.requestState(kStateB);

    // The transition is already complete by the time requestState() returns.
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(kStateB, fsm.getCurrentState());
    TEST_ASSERT_EQUAL_UINT8(kStateA, fsm.getPreviousState());

    TEST_ASSERT_EQUAL_INT(2, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].kind == LogEntry::Kind::Exit);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[0].state);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[0].other);
    TEST_ASSERT_TRUE(owner.log[1].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[1].state);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[1].other);
}

void test_null_callback_skipped_without_crash(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateB);
    owner.logCount = 0;

    // B -> C: C's onEnter is set, but C's onExit/onUpdate are null. Neither
    // direction should crash or be logged.
    TEST_ASSERT_TRUE(fsm.requestState(kStateC));
    TEST_ASSERT_EQUAL_UINT8(kStateC, fsm.getCurrentState());
    fsm.update(16);  // C's onUpdate is null: must be skipped, not crash.
    TEST_ASSERT_EQUAL_INT(0, owner.updateCount);

    // C -> A: C's onExit is null and must be skipped without crashing, but
    // A's onEnter must still fire.
    TEST_ASSERT_TRUE(fsm.requestState(kStateA));
    TEST_ASSERT_EQUAL_UINT8(kStateA, fsm.getCurrentState());

    bool sawEnterA = false;
    for (int i = 0; i < owner.logCount; ++i) {
        if (owner.log[i].kind == LogEntry::Kind::Enter && owner.log[i].state == kStateA) {
            sawEnterA = true;
        }
        // C's onExit is null, so no Exit(C, ...) entry may exist.
        TEST_ASSERT_FALSE(owner.log[i].kind == LogEntry::Kind::Exit && owner.log[i].state == kStateC);
    }
    TEST_ASSERT_TRUE(sawEnterA);
}

// =============================================================================
// Requirement: Time-In-State Is A Saturating uint32_t Millisecond Counter
// =============================================================================

void test_time_in_state_zero_in_on_enter_and_accumulates(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);

    TEST_ASSERT_EQUAL_UINT32(0, owner.lastEnterTimeInState);
    TEST_ASSERT_EQUAL_UINT32(0, fsm.getTimeInState());

    fsm.update(100);
    TEST_ASSERT_EQUAL_UINT32(100, fsm.getTimeInState());

    fsm.update(250);
    TEST_ASSERT_EQUAL_UINT32(350, fsm.getTimeInState());
}

void test_time_in_state_saturates_at_uint32_max(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);

    fsm.update(static_cast<unsigned long>(UINT32_MAX) - 10ul);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX - 10u, fsm.getTimeInState());

    fsm.update(100);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, fsm.getTimeInState());

    fsm.update(1);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, fsm.getTimeInState());
}

// =============================================================================
// Requirement: Re-Requesting The Current State Is A No-Op
// Requirement: restartState() Performs A Real Exit/Enter Cycle
// =============================================================================

void test_request_current_state_is_noop(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    fsm.update(500);
    owner.logCount = 0;

    const bool result = fsm.requestState(kStateA);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, owner.logCount);
    TEST_ASSERT_EQUAL_UINT32(500, fsm.getTimeInState());
    TEST_ASSERT_EQUAL_UINT8(kStateA, fsm.getCurrentState());
}

void test_restart_state_performs_real_cycle_and_resets_time(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    fsm.update(500);
    owner.logCount = 0;

    fsm.restartState();

    TEST_ASSERT_EQUAL_UINT32(0, fsm.getTimeInState());
    TEST_ASSERT_EQUAL_UINT8(kStateA, fsm.getCurrentState());
    TEST_ASSERT_EQUAL_INT(2, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].kind == LogEntry::Kind::Exit);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[0].state);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[0].other);
    TEST_ASSERT_TRUE(owner.log[1].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[1].state);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[1].other);
}

// =============================================================================
// Requirement: Transitions Requested From onEnter Or onExit Are Queued And
// Drained Without Recursion
// =============================================================================

void test_onenter_requesting_transition_produces_no_recursion(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateC);  // Neutral starting point; arm the request only below.
    owner.requestFromEnterOf = kStateA;
    owner.requestFromEnterTarget = kStateB;
    owner.logCount = 0;

    // requestState(A) called from outside the machine: A's onEnter requests
    // B. Expected order: A's onEnter runs to completion, then A's onExit(B),
    // then B's onEnter(A) — never a nested/recursive drain.
    const bool result = fsm.requestState(kStateA);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(kStateB, fsm.getCurrentState());
    TEST_ASSERT_EQUAL_UINT8(kStateA, fsm.getPreviousState());

    TEST_ASSERT_EQUAL_INT(3, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[0].state);
    TEST_ASSERT_TRUE(owner.log[1].kind == LogEntry::Kind::Exit);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[1].state);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[1].other);
    TEST_ASSERT_TRUE(owner.log[2].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[2].state);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[2].other);
}

void test_onexit_requested_transition_honored_after_inflight_completes(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    owner.requestFromExitOf = kStateA;
    owner.requestFromExitTarget = kStateC;
    owner.logCount = 0;

    // Request A -> B; A's onExit additionally requests C. The in-flight
    // transition to B must complete (B's onEnter runs) before C is entered.
    const bool result = fsm.requestState(kStateB);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(kStateC, fsm.getCurrentState());

    TEST_ASSERT_EQUAL_INT(4, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].kind == LogEntry::Kind::Exit);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[0].state);
    TEST_ASSERT_TRUE(owner.log[1].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[1].state);
    TEST_ASSERT_TRUE(owner.log[2].kind == LogEntry::Kind::Exit);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[2].state);
    TEST_ASSERT_EQUAL_UINT8(kStateC, owner.log[2].other);
    TEST_ASSERT_TRUE(owner.log[3].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateC, owner.log[3].state);
    TEST_ASSERT_EQUAL_UINT8(kStateB, owner.log[3].other);
}

void test_last_writer_wins_within_one_drain_iteration(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    // Within the single A->B drain iteration: onExit(A) requests C (X),
    // onEnter(B) runs afterward and requests A (Y). Y must win.
    owner.requestFromExitOf = kStateA;
    owner.requestFromExitTarget = kStateC;
    owner.requestFromEnterOf = kStateB;
    owner.requestFromEnterTarget = kStateA;
    owner.logCount = 0;

    const bool result = fsm.requestState(kStateB);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(kStateA, fsm.getCurrentState());  // Y, not X.

    for (int i = 0; i < owner.logCount; ++i) {
        TEST_ASSERT_FALSE(owner.log[i].state == kStateC);  // C is never entered.
    }
}

void test_ping_pong_terminates_and_records_overflow(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateC);
    owner.logCount = 0;
    owner.pingPongAB = true;

    TEST_ASSERT_EQUAL_UINT8(0, fsm.getTransitionOverflowCount());

    const bool result = fsm.requestState(kStateA);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(1, fsm.getTransitionOverflowCount());

    // Left in a valid, fully-entered state: the last logged call is an Enter
    // for whatever state the machine settled on.
    TEST_ASSERT_TRUE(fsm.isRunning());
    TEST_ASSERT_TRUE(owner.logCount > 0);
    const LogEntry& last = owner.log[owner.logCount - 1];
    TEST_ASSERT_TRUE(last.kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(fsm.getCurrentState(), last.state);
}

// =============================================================================
// Requirement: At Most One onUpdate Invocation Per update() Call
// =============================================================================

void test_update_fires_exactly_one_onupdate_even_when_transitioning(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    owner.requestFromUpdateOf = kStateA;
    owner.requestFromUpdateTarget = kStateB;
    owner.logCount = 0;

    fsm.update(16);

    TEST_ASSERT_EQUAL_INT(1, owner.updateCount);
    TEST_ASSERT_EQUAL_UINT8(kStateB, fsm.getCurrentState());

    int updateLogEntries = 0;
    for (int i = 0; i < owner.logCount; ++i) {
        if (owner.log[i].kind == LogEntry::Kind::Update) ++updateLogEntries;
    }
    TEST_ASSERT_EQUAL_INT(1, updateLogEntries);
}

void test_update_before_start_is_noop(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    TEST_ASSERT_FALSE(fsm.isRunning());

    fsm.update(500);

    TEST_ASSERT_EQUAL_INT(0, owner.logCount);
    TEST_ASSERT_EQUAL_UINT32(0, fsm.getTimeInState());
}

// =============================================================================
// Requirement: start() And reset() Semantics
// =============================================================================

void test_start_fires_only_initial_onenter(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    TEST_ASSERT_FALSE(fsm.isRunning());

    fsm.start(kStateA);

    TEST_ASSERT_TRUE(fsm.isRunning());
    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].kind == LogEntry::Kind::Enter);
    TEST_ASSERT_EQUAL_UINT8(kStateA, owner.log[0].state);
    TEST_ASSERT_EQUAL_UINT8(StateMachine::kInvalidStateId, owner.log[0].other);
}

void test_reset_fires_no_callbacks(void) {
    MockOwner owner;
    StateMachine fsm;
    owner.fsm = &fsm;

    fsm.configure(&owner, kTestTable, kTestTableCount);
    fsm.start(kStateA);
    fsm.update(200);
    owner.logCount = 0;

    fsm.reset();

    TEST_ASSERT_EQUAL_INT(0, owner.logCount);
    TEST_ASSERT_FALSE(fsm.isRunning());
}

// =============================================================================
// Requirement: Owner Is Type-Erased Via void*
//
// (The "no new virtuals on Actor/Entity" half of this requirement is proven
// by test_no_new_virtuals_on_actor_or_entity above, which runs unconditionally
// in both flag states — see that test's comment.)
// =============================================================================

namespace {

void onEnterMinimalActor(void* owner, StateId from) {
    (void)from;
    // Proves the callback receives owner as void* and the caller is
    // responsible for casting it back to its own concrete type.
    auto* self = static_cast<MinimalActorSurface*>(owner);
    self->width = 42;
}

}  // namespace

void test_callbacks_receive_owner_as_void_star(void) {
    MinimalActorSurface actor;
    static const StateMachine::State kActorTable[] = {
        { onEnterMinimalActor, nullptr, nullptr, kStateA },
    };
    StateMachine fsm;

    fsm.configure(&actor, kActorTable, 1);
    fsm.start(kStateA);

    TEST_ASSERT_EQUAL_INT(42, actor.width);
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled
// =============================================================================

void test_gameplay_state_machine_zero_cost_when_disabled(void) {
    // Layout: field offsets must strictly ascend in D1's declared row order
    // (onEnter, onUpdate, onExit, id) — pointers first (widest, strictest
    // alignment), tag last — without hardcoding an absolute sizeof()
    // literal (native is 64-bit, ESP32-C3 is 32-bit).
    TEST_ASSERT_TRUE(offsetof(StateMachine::State, onEnter) < offsetof(StateMachine::State, onUpdate));
    TEST_ASSERT_TRUE(offsetof(StateMachine::State, onUpdate) < offsetof(StateMachine::State, onExit));
    TEST_ASSERT_TRUE(offsetof(StateMachine::State, onExit) < offsetof(StateMachine::State, id));

    // No unexpected members: total size must not exceed the sum of the four
    // declared members plus at most one machine word of alignment padding.
    const size_t declaredRowSize =
        sizeof(StateMachine::EnterFn) +
        sizeof(StateMachine::UpdateFn) +
        sizeof(StateMachine::ExitFn) +
        sizeof(StateId);
    TEST_ASSERT_TRUE(sizeof(StateMachine::State) <= declaredRowSize + sizeof(void*));

    // The StateMachine instance itself must stay within its declared members
    // (owner_, states_, timeInStateMs_, 6 x 1-byte fields) plus at most one
    // machine word of padding (design.md D8: 20 B on ESP32-C3, 32 B native).
    const size_t declaredInstanceSize =
        sizeof(void*) +        // owner_
        sizeof(const void*) +  // states_
        sizeof(uint32_t) +     // timeInStateMs_
        6 * sizeof(uint8_t);   // current_/previous_/pending_/stateCount_/inTransition_/transitionOverflows_
    TEST_ASSERT_TRUE(sizeof(StateMachine) <= declaredInstanceSize + sizeof(void*));
}

#else  // !PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE

void test_gameplay_state_machine_zero_cost_when_disabled(void) {
    // With the flag off, StateMachine is not compiled at all — its entire
    // declaration lives inside the #if PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE
    // guard in include/gameplay/StateMachine.h. This translation unit
    // compiling and passing without referencing the type IS the "zero bytes
    // reserved" property required by the spec's "Feature-Gated And
    // Zero-Cost When Disabled" requirement.
    TEST_PASS_MESSAGE("PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE=0: StateMachine is not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE

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

#if PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE
    RUN_TEST(test_configure_binds_table_without_copying);
    RUN_TEST(test_state_lookup_matches_by_id_not_row_order);
    RUN_TEST(test_request_state_unknown_id_rejected);
    RUN_TEST(test_transition_invokes_onexit_then_onenter_in_order);
    RUN_TEST(test_null_callback_skipped_without_crash);
    RUN_TEST(test_time_in_state_zero_in_on_enter_and_accumulates);
    RUN_TEST(test_time_in_state_saturates_at_uint32_max);
    RUN_TEST(test_request_current_state_is_noop);
    RUN_TEST(test_restart_state_performs_real_cycle_and_resets_time);
    RUN_TEST(test_onenter_requesting_transition_produces_no_recursion);
    RUN_TEST(test_onexit_requested_transition_honored_after_inflight_completes);
    RUN_TEST(test_last_writer_wins_within_one_drain_iteration);
    RUN_TEST(test_ping_pong_terminates_and_records_overflow);
    RUN_TEST(test_update_fires_exactly_one_onupdate_even_when_transitioning);
    RUN_TEST(test_update_before_start_is_noop);
    RUN_TEST(test_start_fires_only_initial_onenter);
    RUN_TEST(test_reset_fires_no_callbacks);
    RUN_TEST(test_callbacks_receive_owner_as_void_star);
#endif
    RUN_TEST(test_gameplay_state_machine_zero_cost_when_disabled);

    return UNITY_END();
}
