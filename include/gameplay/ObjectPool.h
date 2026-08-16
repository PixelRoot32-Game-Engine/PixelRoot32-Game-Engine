/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

/**
 * @file ObjectPool.h
 * @brief Fixed-capacity, zero-heap object pool with placement-new storage.
 *
 * ## Arena-backed pool storage is NOT supported (design.md D6)
 *
 * `ObjectPool<T, N>` is designed to be a plain member of the owning `Scene`
 * or game object (or a file-scope `static`) — never allocated through
 * `arenaNew`, and never backed by memory obtained from `SceneArena`. This is
 * a structural limitation, not an oversight:
 *
 * - `Scene::resetState()` calls `arena.reset()` unconditionally, and the
 *   arena runs no destructors: it only rewinds a bump offset. A pool placed
 *   in arena memory would therefore never run `~ObjectPool()`, so `~T()`
 *   never runs for any slot that was still live at reset time — a silent
 *   resource leak for any `T` owning a resource.
 * - Worse, a pool whose *storage* lives in arena memory keeps a
 *   `liveWords_` bitmask asserting slots are live while the arena has
 *   already rewound its offset and is about to hand those exact bytes to
 *   the next `arenaNew` caller. Two live objects then alias one address,
 *   and a later `release()`/`reset()` runs `~T()` over memory owned by
 *   something else.
 *
 * There is no placement-site check in C++ that can catch this, and the one
 * enforcement point that could — a `Scene::resetState()` hook — is exactly
 * the engine-side wiring this capability is designed NOT to require. A
 * documented contract with no enforcement point is worse than an
 * unsupported feature stated plainly, so this header states it plainly.
 *
 * The supported reset contract instead: a pool owned by a `Scene` is reset
 * from that scene's `init()` override, *after* the base `Scene::init()` has
 * already run (which runs `resetState()` first):
 *
 * @code
 * void MyScene::init() {
 *     Scene::init();   // clears entities + removes them from CollisionSystem
 *     pool_.reset();   // now safe: no dangling entity references remain
 *     // ... repopulate
 * }
 * @endcode
 *
 * The ordering is "after", not "before": if a pooled `T` is also a
 * registered `Entity`, destructing it before `Scene::init()`'s
 * `resetState()` runs would leave dangling pointers in the scene's entity
 * list for the window until `clearEntities()` runs.
 *
 * Narrow footnote, not a supported mode: if `std::is_trivially_destructible_v<T>`
 * (e.g. a plain POD payload), arena placement degrades to the harmless
 * "the pool's bytes disappear" case, since skipping a trivial `~T()` has no
 * observable effect. This is not offered as a supported configuration
 * because it cannot be enforced at the placement site — a caller who later
 * changes `T` to own a resource gets silent corruption with no diagnostic.
 */

namespace pixelroot32::gameplay {

namespace detail {

#if defined(__GNUC__) || defined(__clang__)

/// Index of the lowest set bit in `bits`. Every call site in ObjectPool
/// guarantees `bits != 0` before calling.
inline int objectPoolCountTrailingZero(uint32_t bits) {
    return __builtin_ctz(bits);
}

#else

/// Portable bit-loop fallback for toolchains without `__builtin_ctz`. Every
/// call site in ObjectPool guarantees `bits != 0` before calling.
inline int objectPoolCountTrailingZero(uint32_t bits) {
    int index = 0;
    while ((bits & 1u) == 0u) {
        bits >>= 1u;
        ++index;
    }
    return index;
}

#endif  // defined(__GNUC__) || defined(__clang__)

}  // namespace detail

/**
 * @class ObjectPool
 * @brief Fixed-capacity, zero-heap slot pool over aligned raw storage.
 *
 * Slots are `alignas(T) unsigned char storage_[N * sizeof(T)]`, constructed
 * in place with placement new and recovered with `std::launder` for
 * standard-correct aliasing (design.md D4). `T` is NOT required to have a
 * default constructor: `acquire()` forwards its arguments to `T`'s
 * constructor. Liveness is tracked by a `uint32_t` bitmask with a scan hint
 * (design.md D5) — `acquire()` finds a free bit with `__builtin_ctz` (or a
 * portable fallback), and `nextLive()` supports the ascending-index
 * iteration pattern both hand-rolled precedents in this codebase already
 * use:
 *
 * @code
 * for (uint16_t i = pool.nextLive(0); i != Pool::kEnd; i = pool.nextLive(i + 1)) {
 *     T& obj = *pool.at(i);
 *     // ...
 * }
 * @endcode
 *
 * Never allocates: no `new`, no `malloc`, no `std::vector`. With
 * `-fno-exceptions`, a constructor cannot fail, so there is no
 * partially-constructed-slot case to unwind.
 *
 * Copy construction and copy assignment are deleted — a pool's slot
 * pointers are identity, not value; copying the raw bytes without
 * re-running every live `T`'s copy constructor would alias two pools over
 * one set of resources.
 *
 * See this file's header-level doc comment for why arena-backed pool
 * storage is NOT supported (design.md D6).
 */
template <typename T, uint16_t N>
class ObjectPool {
    static_assert(N >= 1, "ObjectPool capacity must be at least 1");

public:
    /// Fixed slot capacity of this pool instantiation.
    static constexpr uint16_t kCapacity = N;

    /// nextLive()/indexOf() end-of-iteration / not-found sentinel.
    static constexpr uint16_t kEnd = 0xFFFFu;

    ObjectPool() = default;

    /// Destructs every remaining live slot via reset().
    ~ObjectPool() { reset(); }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    /**
     * @brief Constructs a `T` in a free slot, forwarding `args` to `T`'s
     *        constructor.
     * @return A pointer to the newly constructed, live object, or `nullptr`
     *         when the pool is full — every existing live slot is left
     *         unmodified in that case.
     *
     * Never allocates (no `new`, no `malloc`). `T` does NOT need a default
     * constructor; only a constructor matching `args` is required.
     */
    template <typename... Args>
    T* acquire(Args&&... args) {
        if (liveCount_ >= N) {
            return nullptr;
        }
        for (uint16_t scanned = 0; scanned < kWordCount; ++scanned) {
            const uint16_t wordIndex = static_cast<uint16_t>((scanHint_ + scanned) % kWordCount);
            const uint32_t freeBits = (~liveWords_[wordIndex]) & wordMask(wordIndex);
            if (freeBits != 0u) {
                const uint32_t bit =
                    static_cast<uint32_t>(detail::objectPoolCountTrailingZero(freeBits));
                const uint16_t index = static_cast<uint16_t>(wordIndex * 32u + bit);

                // Placement new's own return value is the correct pointer to
                // use immediately — no std::launder needed on this path
                // (design.md D4). Laundering is only required when a T* is
                // later recovered from the raw bytes (at()/indexOf()/nextLive()).
                T* slot = ::new (static_cast<void*>(rawSlot(index))) T(std::forward<Args>(args)...);

                liveWords_[wordIndex] |= (uint32_t{1} << bit);
                ++liveCount_;
                scanHint_ = wordIndex;
                return slot;
            }
        }
        // Unreachable when liveCount_ < N and bookkeeping is consistent;
        // kept as a defensive fallback rather than an assert.
        return nullptr;
    }

    /**
     * @brief Runs `~T()` on the slot holding `object` and frees it.
     * @return `true` if a live slot was released; `false` (safe no-op, no
     *         bookkeeping change, `~T()` NOT invoked again) for `nullptr`, a
     *         pointer foreign to this pool, or a slot that is already free.
     */
    bool release(T* object) {
        if (object == nullptr) {
            return false;
        }
        const uint16_t index = indexOf(object);
        if (index == kEnd) {
            return false;
        }
        return releaseAt(index);
    }

    /**
     * @brief Runs `~T()` on the slot at `index` and frees it.
     * @return `true` if the slot was live and is now freed; `false` (safe
     *         no-op) for an out-of-range index or a slot that is already
     *         free.
     */
    bool releaseAt(uint16_t index) {
        if (index >= N || !isLive(index)) {
            return false;
        }
        std::launder(reinterpret_cast<T*>(rawSlot(index)))->~T();
        clearBit(index);
        --liveCount_;

        const uint16_t wordIndex = static_cast<uint16_t>(index / 32u);
        if (wordIndex < scanHint_) {
            scanHint_ = wordIndex;
        }
        return true;
    }

    /**
     * @brief Runs `~T()` on every live slot, ascending index order, then
     *        clears all bookkeeping. Safe to call on an empty pool.
     */
    void reset() {
        for (uint16_t i = nextLive(0); i != kEnd; i = nextLive(static_cast<uint16_t>(i + 1))) {
            std::launder(reinterpret_cast<T*>(rawSlot(i)))->~T();
        }
        for (uint16_t w = 0; w < kWordCount; ++w) {
            liveWords_[w] = 0u;
        }
        liveCount_ = 0;
        scanHint_ = 0;
    }

    /** @brief Fixed slot capacity (same value as kCapacity). */
    uint16_t capacity() const { return N; }

    /** @brief Number of slots currently live. */
    uint16_t size() const { return liveCount_; }

    /** @brief True when size() == capacity(). */
    bool isFull() const { return liveCount_ == N; }

    /** @brief True when `index` is in range and currently holds a live object. */
    bool isLive(uint16_t index) const {
        if (index >= N) {
            return false;
        }
        const uint16_t wordIndex = static_cast<uint16_t>(index / 32u);
        const uint32_t bit = static_cast<uint32_t>(index % 32u);
        return (liveWords_[wordIndex] & (uint32_t{1} << bit)) != 0u;
    }

    /**
     * @brief Recovers a pointer to the live object at `index`.
     * @return A `std::launder`-corrected `T*`, or `nullptr` if `index` is
     *         out of range or dead.
     */
    T* at(uint16_t index) {
        if (!isLive(index)) {
            return nullptr;
        }
        return std::launder(reinterpret_cast<T*>(rawSlot(index)));
    }

    /**
     * @brief Finds the slot index owning `object`.
     * @return The slot index if `object` was returned by this pool's
     *         `acquire()` and is still live; `kEnd` for a null, foreign, or
     *         no-longer-live pointer.
     */
    uint16_t indexOf(const T* object) const {
        if (object == nullptr) {
            return kEnd;
        }
        const auto objAddr = reinterpret_cast<std::uintptr_t>(object);
        const auto baseAddr = reinterpret_cast<std::uintptr_t>(storage_);
        const auto totalBytes = static_cast<std::uintptr_t>(N) * sizeof(T);
        if (objAddr < baseAddr || objAddr >= baseAddr + totalBytes) {
            return kEnd;
        }
        const std::uintptr_t offset = objAddr - baseAddr;
        if (offset % sizeof(T) != 0u) {
            return kEnd;
        }
        const uint16_t index = static_cast<uint16_t>(offset / sizeof(T));
        return isLive(index) ? index : kEnd;
    }

    /**
     * @brief Returns the first live index `>= from`, or `kEnd` when none
     *        remains.
     *
     * Masks off bits below `from` in the first word examined and skips
     * whole zero words, so iterating with
     * `for (i = nextLive(0); i != kEnd; i = nextLive(i + 1))` visits exactly
     * the live indices, in ascending order, each exactly once.
     */
    uint16_t nextLive(uint16_t from) const {
        if (from >= N) {
            return kEnd;
        }
        uint16_t wordIndex = static_cast<uint16_t>(from / 32u);
        const uint32_t bitOffset = static_cast<uint32_t>(from % 32u);
        uint32_t word = liveWords_[wordIndex] & (0xFFFFFFFFu << bitOffset);

        while (true) {
            if (word != 0u) {
                const uint32_t bit =
                    static_cast<uint32_t>(detail::objectPoolCountTrailingZero(word));
                return static_cast<uint16_t>(wordIndex * 32u + bit);
            }
            ++wordIndex;
            if (wordIndex >= kWordCount) {
                return kEnd;
            }
            word = liveWords_[wordIndex];
        }
    }

private:
    static constexpr uint16_t kWordCount = static_cast<uint16_t>((N + 31u) / 32u);

    /// Mask of the bits within `liveWords_[wordIndex]` that correspond to a
    /// real slot. Always all-ones except for the last word when N is not a
    /// multiple of 32, where the high bits beyond N-1 have no backing slot
    /// and must never be treated as "free" by acquire()/nextLive().
    static constexpr uint32_t wordMask(uint16_t wordIndex) {
        const uint32_t remaining =
            static_cast<uint32_t>(N) - static_cast<uint32_t>(wordIndex) * 32u;
        if (remaining >= 32u) {
            return 0xFFFFFFFFu;
        }
        return (remaining == 0u) ? 0u : ((uint32_t{1} << remaining) - 1u);
    }

    unsigned char* rawSlot(uint16_t index) {
        return storage_ + static_cast<size_t>(index) * sizeof(T);
    }
    const unsigned char* rawSlot(uint16_t index) const {
        return storage_ + static_cast<size_t>(index) * sizeof(T);
    }

    void clearBit(uint16_t index) {
        const uint16_t wordIndex = static_cast<uint16_t>(index / 32u);
        const uint32_t bit = static_cast<uint32_t>(index % 32u);
        liveWords_[wordIndex] &= ~(uint32_t{1} << bit);
    }

    alignas(T) unsigned char storage_[static_cast<size_t>(N) * sizeof(T)]{};
    uint32_t liveWords_[kWordCount]{};
    uint16_t liveCount_ = 0;
    uint16_t scanHint_ = 0;
};

}  // namespace pixelroot32::gameplay

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL
