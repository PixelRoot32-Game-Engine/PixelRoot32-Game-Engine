/**
 * @file test_gameplay_object_pool.cpp
 * @brief Unit tests for gameplay/ObjectPool module
 *
 * Covers the spec requirements for gameplay-object-pool:
 * - acquire-returns-usable-object-or-null-at-capacity-never-overflows-or-allocates
 * - t-requires-no-default-constructor-construction-forwards-caller-arguments
 * - destructor-invocation-on-release-and-reset
 * - iteration-visits-only-live-slots-in-ascending-index-order
 * - free-live-bookkeeping-correctness-across-interleaved-acquire-release-cycles
 * - feature-gated-and-zero-cost-when-disabled
 *
 * (Arena-Backed Pool Storage Is Not Supported and Pool Owned By A Scene Must
 * Be Reset After The Base Scene::init() Call are documented contracts, not
 * runtime-testable behaviors — see include/gameplay/ObjectPool.h's
 * header-level doc comment for the former and design.md D6 for both.)
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL
 * is enabled, since ObjectPool is entirely guarded behind that flag (see
 * include/gameplay/ObjectPool.h). This file therefore compiles cleanly in
 * BOTH the default (flag off) and opt-in (flag on) configurations, matching
 * the "no behavior change for existing examples" goal and the CI matrix from
 * design.md.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL

#include "gameplay/ObjectPool.h"

#include <cstddef>
#include <cstdint>

using namespace pixelroot32::gameplay;

namespace {

/**
 * @brief Mock payload with NO default constructor — only a two-argument
 *        constructor — so that acquire()'s forwarding is exercised, and
 *        with static construct/destruct counters plus an ordered destruct
 *        log so release()/reset()/~ObjectPool() lifetime can be verified.
 */
struct TrackedPayload {
    static constexpr int kMaxLog = 64;
    static int constructCount;
    static int destructCount;
    static int destructOrderLog[kMaxLog];
    static int destructOrderCount;

    static void resetCounters() {
        constructCount = 0;
        destructCount = 0;
        destructOrderCount = 0;
    }

    int value;
    int tag;

    // No default constructor on purpose (spec: "T Requires No Default
    // Constructor; Construction Forwards Caller Arguments").
    TrackedPayload(int v, int t) : value(v), tag(t) { ++constructCount; }

    ~TrackedPayload() {
        ++destructCount;
        if (destructOrderCount < kMaxLog) {
            destructOrderLog[destructOrderCount++] = tag;
        }
    }
};

int TrackedPayload::constructCount = 0;
int TrackedPayload::destructCount = 0;
int TrackedPayload::destructOrderLog[TrackedPayload::kMaxLog] = {};
int TrackedPayload::destructOrderCount = 0;

/// Over-aligned mock payload for the alignment scenario (task 3.14).
struct alignas(16) AlignedPayload {
    int value;
    explicit AlignedPayload(int v) : value(v) {}
};

}  // namespace

// =============================================================================
// Requirement: Acquire Returns A Usable Object Or Null At Capacity, Never
// Overflows Or Allocates
// =============================================================================

void test_acquire_on_nonfull_pool_returns_live_constructed_object(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;

    TrackedPayload* obj = pool.acquire(11, 0);

    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_EQUAL_INT(11, obj->value);
    TEST_ASSERT_EQUAL_UINT16(1, pool.size());
    const uint16_t index = pool.indexOf(obj);
    // Alias, not the spelled-out template: the comma in <TrackedPayload, 4>
    // is an argument separator to the preprocessor, so naming the template
    // inside a Unity macro splits it into two arguments and fails to compile.
    using Pool = ObjectPool<TrackedPayload, 4>;
    TEST_ASSERT_TRUE(index != Pool::kEnd);
    TEST_ASSERT_TRUE(pool.isLive(index));
    TEST_ASSERT_EQUAL_INT(1, TrackedPayload::constructCount);
}

void test_acquire_on_full_pool_returns_null_without_corrupting_live_objects(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;

    TrackedPayload* objs[4];
    for (int i = 0; i < 4; ++i) {
        objs[i] = pool.acquire(100 + i, i);
        TEST_ASSERT_NOT_NULL(objs[i]);
    }
    TEST_ASSERT_TRUE(pool.isFull());
    TEST_ASSERT_EQUAL_UINT16(4, pool.size());

    TrackedPayload* overflow = pool.acquire(999, 999);

    TEST_ASSERT_NULL(overflow);
    TEST_ASSERT_EQUAL_UINT16(4, pool.size());
    // Every previously-live object must still read back its original value —
    // acquire() on a full pool must not touch any existing slot.
    for (int i = 0; i < 4; ++i) {
        TEST_ASSERT_EQUAL_INT(100 + i, objs[i]->value);
        TEST_ASSERT_TRUE(pool.isLive(pool.indexOf(objs[i])));
    }
}

void test_acquire_never_allocates_dynamically(void) {
    // Structural guarantee, verified by inspection of ObjectPool.h: acquire()
    // only performs placement-new into alignas(T) raw storage that is a
    // member of the pool itself — there is no `new`, `malloc`, or any other
    // dynamic allocator anywhere on the acquire() path (design.md D4). This
    // test exercises acquire() across empty, partial, and full states to
    // confirm none of those paths crash or behave as if memory had to be
    // sourced externally.
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 2> pool;

    TEST_ASSERT_NOT_NULL(pool.acquire(1, 0));
    TEST_ASSERT_NOT_NULL(pool.acquire(2, 1));
    TEST_ASSERT_NULL(pool.acquire(3, 2));  // full: no allocation fallback exists
}

// =============================================================================
// Requirement: T Requires No Default Constructor; Construction Forwards
// Caller Arguments
// =============================================================================

void test_acquire_constructs_t_with_no_default_constructor_via_forwarded_args(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 2> pool;

    // TrackedPayload has no default constructor; this compiling and
    // constructing correctly IS the requirement.
    TrackedPayload* obj = pool.acquire(42, 7);

    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_EQUAL_INT(42, obj->value);
    TEST_ASSERT_EQUAL_INT(7, obj->tag);
}

// =============================================================================
// Requirement: Destructor Invocation On Release And Reset
// =============================================================================

void test_release_destroys_exactly_once_and_frees_slot(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;

    TrackedPayload* obj = pool.acquire(5, 0);
    const uint16_t index = pool.indexOf(obj);

    const bool released = pool.release(obj);

    TEST_ASSERT_TRUE(released);
    TEST_ASSERT_EQUAL_INT(1, TrackedPayload::destructCount);
    TEST_ASSERT_FALSE(pool.isLive(index));
    TEST_ASSERT_EQUAL_UINT16(0, pool.size());
}

void test_release_by_index_destroys_exactly_once_and_frees_slot(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;

    TrackedPayload* obj = pool.acquire(6, 0);
    const uint16_t index = pool.indexOf(obj);

    const bool released = pool.releaseAt(index);

    TEST_ASSERT_TRUE(released);
    TEST_ASSERT_EQUAL_INT(1, TrackedPayload::destructCount);
    TEST_ASSERT_FALSE(pool.isLive(index));
}

void test_releasing_null_foreign_or_already_free_pointer_is_safe_noop(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;

    // Null pointer.
    TEST_ASSERT_FALSE(pool.release(nullptr));
    TEST_ASSERT_EQUAL_INT(0, TrackedPayload::destructCount);
    TEST_ASSERT_EQUAL_UINT16(0, pool.size());

    // Foreign pointer: a live TrackedPayload that does not belong to `pool`.
    TrackedPayload standalone(1, 1);
    TEST_ASSERT_FALSE(pool.release(&standalone));
    TEST_ASSERT_EQUAL_UINT16(0, pool.size());
    using Pool = ObjectPool<TrackedPayload, 4>;
    TEST_ASSERT_EQUAL_UINT16(Pool::kEnd, pool.indexOf(&standalone));

    // Already-free slot: release a live object, then release it again.
    TrackedPayload* obj = pool.acquire(2, 2);
    TEST_ASSERT_TRUE(pool.release(obj));
    const int destructCountAfterFirstRelease = TrackedPayload::destructCount;

    TEST_ASSERT_FALSE(pool.release(obj));
    TEST_ASSERT_EQUAL_INT(destructCountAfterFirstRelease, TrackedPayload::destructCount);
    TEST_ASSERT_EQUAL_UINT16(0, pool.size());

    // Out-of-range index is likewise a safe no-op.
    TEST_ASSERT_FALSE(pool.releaseAt(999));
}

void test_reset_destroys_all_live_slots_in_ascending_index_order(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 8> pool;

    // Acquire several, release a non-contiguous subset, leaving a fragmented
    // mix of live slots — reset() must still destroy the survivors in
    // ascending INDEX order, not acquisition order.
    TrackedPayload* p0 = pool.acquire(0, 100);
    TrackedPayload* p1 = pool.acquire(1, 101);
    TrackedPayload* p2 = pool.acquire(2, 102);
    TrackedPayload* p3 = pool.acquire(3, 103);
    TEST_ASSERT_TRUE(pool.release(p1));  // fragment the middle

    const uint16_t idx0 = pool.indexOf(p0);
    const uint16_t idx2 = pool.indexOf(p2);
    const uint16_t idx3 = pool.indexOf(p3);
    (void)p1;

    TrackedPayload::destructOrderCount = 0;  // ignore p1's destruction above
    pool.reset();

    TEST_ASSERT_EQUAL_UINT16(0, pool.size());
    for (uint16_t i = 0; i < pool.capacity(); ++i) {
        TEST_ASSERT_FALSE(pool.isLive(i));
    }

    // The three survivors (tags 100, 102, 103 at idx0 < idx2 < idx3) must be
    // destroyed in that ascending-index order.
    TEST_ASSERT_TRUE(idx0 < idx2);
    TEST_ASSERT_TRUE(idx2 < idx3);
    TEST_ASSERT_EQUAL_INT(3, TrackedPayload::destructOrderCount);
    TEST_ASSERT_EQUAL_INT(100, TrackedPayload::destructOrderLog[0]);
    TEST_ASSERT_EQUAL_INT(102, TrackedPayload::destructOrderLog[1]);
    TEST_ASSERT_EQUAL_INT(103, TrackedPayload::destructOrderLog[2]);
}

void test_pool_destructor_destroys_remaining_live_slots_on_scope_exit(void) {
    TrackedPayload::resetCounters();
    {
        ObjectPool<TrackedPayload, 4> pool;
        pool.acquire(1, 0);
        pool.acquire(2, 1);
        pool.acquire(3, 2);
        TEST_ASSERT_EQUAL_INT(0, TrackedPayload::destructCount);
    }  // ~ObjectPool() must run here, destroying all three live slots.
    TEST_ASSERT_EQUAL_INT(3, TrackedPayload::destructCount);
}

// =============================================================================
// Requirement: Iteration Visits Only Live Slots, In Ascending Index Order
// =============================================================================

void test_nextlive_visits_exactly_live_indices_in_ascending_order(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 8> pool;

    TrackedPayload* items[8];
    for (int i = 0; i < 8; ++i) {
        items[i] = pool.acquire(i, i);
    }
    // Release a non-contiguous subset: indices 1, 3, 6.
    TEST_ASSERT_TRUE(pool.release(items[1]));
    TEST_ASSERT_TRUE(pool.release(items[3]));
    TEST_ASSERT_TRUE(pool.release(items[6]));

    uint16_t visited[8];
    int visitedCount = 0;
    using Pool = ObjectPool<TrackedPayload, 8>;
    for (uint16_t i = pool.nextLive(0); i != Pool::kEnd; i = pool.nextLive(static_cast<uint16_t>(i + 1))) {
        TEST_ASSERT_TRUE(visitedCount < 8);
        visited[visitedCount++] = i;
    }

    // Expected surviving indices, strictly ascending: 0, 2, 4, 5, 7.
    TEST_ASSERT_EQUAL_INT(5, visitedCount);
    TEST_ASSERT_EQUAL_UINT16(0, visited[0]);
    TEST_ASSERT_EQUAL_UINT16(2, visited[1]);
    TEST_ASSERT_EQUAL_UINT16(4, visited[2]);
    TEST_ASSERT_EQUAL_UINT16(5, visited[3]);
    TEST_ASSERT_EQUAL_UINT16(7, visited[4]);
    for (int i = 1; i < visitedCount; ++i) {
        TEST_ASSERT_TRUE(visited[i - 1] < visited[i]);
    }
}

void test_nextlive_on_empty_and_fully_live_pool(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;
    using Pool = ObjectPool<TrackedPayload, 4>;

    // Empty pool: no live index anywhere.
    TEST_ASSERT_EQUAL_UINT16(Pool::kEnd, pool.nextLive(0));

    for (int i = 0; i < 4; ++i) {
        pool.acquire(i, i);
    }
    // Fully live: visits every index 0..3, then kEnd.
    uint16_t i = pool.nextLive(0);
    for (uint16_t expected = 0; expected < 4; ++expected) {
        TEST_ASSERT_EQUAL_UINT16(expected, i);
        i = pool.nextLive(static_cast<uint16_t>(i + 1));
    }
    TEST_ASSERT_EQUAL_UINT16(Pool::kEnd, i);
}

// =============================================================================
// Requirement: Free/Live Bookkeeping Correctness Across Interleaved
// Acquire/Release Cycles
// =============================================================================

void test_bookkeeping_correct_across_interleaved_acquire_release(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;

    TrackedPayload* a = pool.acquire(1, 0);
    TrackedPayload* b = pool.acquire(2, 1);
    const uint16_t indexA = pool.indexOf(a);
    TEST_ASSERT_EQUAL_UINT16(2, pool.size());
    TEST_ASSERT_FALSE(pool.isFull());

    TEST_ASSERT_TRUE(pool.release(a));
    TEST_ASSERT_EQUAL_UINT16(1, pool.size());
    TEST_ASSERT_FALSE(pool.isLive(indexA));  // a's old slot is now dead

    TrackedPayload* c = pool.acquire(3, 2);
    TrackedPayload* d = pool.acquire(4, 3);
    TEST_ASSERT_EQUAL_UINT16(3, pool.size());

    TrackedPayload* e = pool.acquire(5, 4);
    TEST_ASSERT_TRUE(pool.isFull());
    TEST_ASSERT_EQUAL_UINT16(4, pool.size());

    TEST_ASSERT_TRUE(pool.release(c));
    TEST_ASSERT_FALSE(pool.isFull());
    TEST_ASSERT_EQUAL_UINT16(3, pool.size());

    TEST_ASSERT_TRUE(pool.isLive(pool.indexOf(b)));
    TEST_ASSERT_TRUE(pool.isLive(pool.indexOf(d)));
    TEST_ASSERT_TRUE(pool.isLive(pool.indexOf(e)));
}

void test_released_slot_is_available_for_reuse_on_full_pool(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;

    TrackedPayload* items[4];
    for (int i = 0; i < 4; ++i) {
        items[i] = pool.acquire(10 + i, i);
    }
    TEST_ASSERT_TRUE(pool.isFull());

    TEST_ASSERT_TRUE(pool.release(items[2]));
    TEST_ASSERT_FALSE(pool.isFull());

    TrackedPayload* reused = pool.acquire(999, 999);

    TEST_ASSERT_NOT_NULL(reused);
    TEST_ASSERT_TRUE(pool.isFull());
    TEST_ASSERT_EQUAL_UINT16(4, pool.size());
}

void test_indexof_roundtrips_for_live_object_and_kend_for_foreign_pointer(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 4> pool;
    using Pool = ObjectPool<TrackedPayload, 4>;

    TrackedPayload* obj = pool.acquire(7, 0);
    const uint16_t index = pool.indexOf(obj);

    TEST_ASSERT_TRUE(index != Pool::kEnd);
    TEST_ASSERT_EQUAL_PTR(obj, pool.at(index));

    TrackedPayload standalone(1, 1);
    TEST_ASSERT_EQUAL_UINT16(Pool::kEnd, pool.indexOf(&standalone));
    TEST_ASSERT_EQUAL_UINT16(Pool::kEnd, pool.indexOf(nullptr));
}

// =============================================================================
// Multi-word bitmask boundary check (design.md D5): N = 40 spans two
// liveWords_ words (word0 = indices 0-31, word1 = indices 32-39, with bits
// 8-31 of word1 unused padding). This exercises the tail-word mask that
// prevents acquire()/nextLive() from treating word1's unused high bits as
// free slots — the exact off-by-one class the bit math is most at risk of.
// =============================================================================

void test_multiword_bitmask_handles_tail_word_and_boundary_correctly(void) {
    TrackedPayload::resetCounters();
    ObjectPool<TrackedPayload, 40> pool;
    using Pool = ObjectPool<TrackedPayload, 40>;

    TrackedPayload* items[40];
    for (int i = 0; i < 40; ++i) {
        items[i] = pool.acquire(i, i);
        TEST_ASSERT_NOT_NULL(items[i]);
    }
    // All 40 slots (including all 8 valid bits of the tail word) are live:
    // the pool must report full, and a further acquire must return nullptr
    // rather than treating word1's unused bits 8-31 as free.
    TEST_ASSERT_TRUE(pool.isFull());
    TEST_ASSERT_EQUAL_UINT16(40, pool.size());
    TEST_ASSERT_NULL(pool.acquire(999, 999));

    // Release two adjacent-across-the-word-boundary slots (31 in word0, 32
    // in word1) and confirm nextLive() steps cleanly across that boundary.
    TEST_ASSERT_TRUE(pool.release(items[31]));
    TEST_ASSERT_TRUE(pool.release(items[32]));
    TEST_ASSERT_FALSE(pool.isFull());
    TEST_ASSERT_EQUAL_UINT16(38, pool.size());

    TEST_ASSERT_EQUAL_UINT16(30, pool.nextLive(30));
    TEST_ASSERT_EQUAL_UINT16(33, pool.nextLive(31));  // 31 and 32 both dead
    TEST_ASSERT_EQUAL_UINT16(33, pool.nextLive(32));

    // The last valid index (39) must still be reachable and never mistaken
    // for out-of-range or for the kEnd sentinel.
    TEST_ASSERT_TRUE(pool.isLive(39));
    TEST_ASSERT_EQUAL_UINT16(Pool::kEnd, pool.nextLive(40));

    // Reacquiring must reuse one of the two freed slots (31 or 32), not
    // spill into a nonexistent index 40.
    TrackedPayload* reused = pool.acquire(1000, 1000);
    TEST_ASSERT_NOT_NULL(reused);
    const uint16_t reusedIndex = pool.indexOf(reused);
    TEST_ASSERT_TRUE(reusedIndex == 31 || reusedIndex == 32);
    TEST_ASSERT_EQUAL_UINT16(39, pool.size());
}

// =============================================================================
// D8 byte-budget property assertions (not hardcoded byte counts — native_test
// runs 64-bit, where pointer width and alignof(T) differ from ESP32-C3).
// =============================================================================

void test_pool_sizeof_stays_within_declared_budget(void) {
    using Pool = ObjectPool<TrackedPayload, 8>;

    // Bookkeeping per design.md D5: one uint32_t per 32 slots, plus the
    // liveCount_/scanHint_ uint16_t counters.
    constexpr size_t kWordCount = (8 + 31) / 32;
    constexpr size_t declaredBookkeeping = kWordCount * sizeof(uint32_t) + 2 * sizeof(uint16_t);
    const size_t declaredStorage = static_cast<size_t>(Pool::kCapacity) * sizeof(TrackedPayload);

    // Storage may need up to (alignof(T) - 1) bytes of leading padding, and
    // the object as a whole may need up to one machine word of trailing
    // alignment padding — the same "declared members + one machine word"
    // property bound test_gameplay_event_bus.cpp uses for GameplayEventBus.
    const size_t declaredTotal =
        declaredStorage + declaredBookkeeping + (alignof(TrackedPayload) - 1);

    TEST_ASSERT_TRUE(sizeof(Pool) <= declaredTotal + sizeof(void*));
}

void test_at_returns_correctly_aligned_pointer_for_overaligned_t(void) {
    ObjectPool<AlignedPayload, 4> pool;

    AlignedPayload* a = pool.acquire(1);
    AlignedPayload* b = pool.acquire(2);
    AlignedPayload* c = pool.acquire(3);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    const uint16_t indices[3] = {pool.indexOf(a), pool.indexOf(b), pool.indexOf(c)};

    for (int i = 0; i < 3; ++i) {
        AlignedPayload* recovered = pool.at(indices[i]);
        TEST_ASSERT_NOT_NULL(recovered);
        const auto addr = reinterpret_cast<std::uintptr_t>(recovered);
        TEST_ASSERT_EQUAL_UINT32(0u, static_cast<uint32_t>(addr % alignof(AlignedPayload)));
    }
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled
//
// Defined in BOTH flag states, mirroring
// test_gameplay_state_machine.cpp:680/711, because main() RUN_TESTs it
// unconditionally. Defining it only in the #else branch leaves main()
// referencing an undeclared function whenever the flag is on.
// =============================================================================

void test_gameplay_object_pool_zero_cost_when_disabled(void) {
    // With the flag on, the cost must be exactly the slots and nothing per
    // slot beyond the shared bitmask. Doubling N must grow the pool by
    // exactly N * sizeof(T) — proving there is no hidden per-slot header,
    // no vtable, and no indirection. This is the flag-on complement to
    // test_pool_sizeof_stays_within_declared_budget's upper bound.
    using Pool8 = ObjectPool<TrackedPayload, 8>;
    using Pool16 = ObjectPool<TrackedPayload, 16>;

    // Both N values fall in the same single bitmask word ((N + 31) / 32 == 1),
    // so bookkeeping is identical and the delta is pure storage.
    const size_t growth = sizeof(Pool16) - sizeof(Pool8);
    TEST_ASSERT_EQUAL_UINT32(
        static_cast<uint32_t>(8 * sizeof(TrackedPayload)),
        static_cast<uint32_t>(growth));

    // An instantiated pool reserves its slots statically: no heap, no
    // pointer indirection to storage held elsewhere.
    TEST_ASSERT_TRUE(sizeof(Pool8) >= 8 * sizeof(TrackedPayload));
}

#else  // !PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL

void test_gameplay_object_pool_zero_cost_when_disabled(void) {
    // With the flag off, ObjectPool<T, N> is not compiled at all — its
    // entire declaration lives inside the
    // #if PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL guard in
    // include/gameplay/ObjectPool.h. This translation unit compiling and
    // passing without referencing the template IS the "zero bytes reserved"
    // property required by the spec's "Feature-Gated And Zero-Cost When
    // Disabled" requirement — and it trivially subsumes the narrower
    // "an uninstantiated (T, N) pairing costs nothing" scenario: with the
    // flag off, EVERY (T, N) pairing is uninstantiated, and every one of
    // them costs zero code and zero data.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL=0: ObjectPool is not "
        "compiled, zero bytes reserved for any (T, N) pairing.");
}

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL

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

#if PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL
    RUN_TEST(test_acquire_on_nonfull_pool_returns_live_constructed_object);
    RUN_TEST(test_acquire_on_full_pool_returns_null_without_corrupting_live_objects);
    RUN_TEST(test_acquire_never_allocates_dynamically);
    RUN_TEST(test_acquire_constructs_t_with_no_default_constructor_via_forwarded_args);
    RUN_TEST(test_release_destroys_exactly_once_and_frees_slot);
    RUN_TEST(test_release_by_index_destroys_exactly_once_and_frees_slot);
    RUN_TEST(test_releasing_null_foreign_or_already_free_pointer_is_safe_noop);
    RUN_TEST(test_reset_destroys_all_live_slots_in_ascending_index_order);
    RUN_TEST(test_pool_destructor_destroys_remaining_live_slots_on_scope_exit);
    RUN_TEST(test_nextlive_visits_exactly_live_indices_in_ascending_order);
    RUN_TEST(test_nextlive_on_empty_and_fully_live_pool);
    RUN_TEST(test_bookkeeping_correct_across_interleaved_acquire_release);
    RUN_TEST(test_released_slot_is_available_for_reuse_on_full_pool);
    RUN_TEST(test_indexof_roundtrips_for_live_object_and_kend_for_foreign_pointer);
    RUN_TEST(test_multiword_bitmask_handles_tail_word_and_boundary_correctly);
    RUN_TEST(test_pool_sizeof_stays_within_declared_budget);
    RUN_TEST(test_at_returns_correctly_aligned_pointer_for_overaligned_t);
#endif
    RUN_TEST(test_gameplay_object_pool_zero_cost_when_disabled);

    return UNITY_END();
}
