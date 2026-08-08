# ObjectPool

<Badge type="info" text="Class" />

**Source:** `ObjectPool.h`

## Description

Fixed-capacity, zero-heap slot pool over aligned raw storage.

Slots are `alignas(T) unsigned char storage_[N * sizeof(T)]`, constructed
in place with placement new and recovered with `std::launder` for
standard-correct aliasing (design.md D4). `T` is NOT required to have a
default constructor: `acquire()` forwards its arguments to `T`'s
constructor. Liveness is tracked by a `uint32_t` bitmask with a scan hint
(design.md D5) — `acquire()` finds a free bit with `__builtin_ctz` (or a
portable fallback), and `nextLive()` supports the ascending-index
iteration pattern both hand-rolled precedents in this codebase already
use:


```cpp
for (uint16_t i = pool.nextLive(0); i != Pool::kEnd; i = pool.nextLive(i + 1)) {
    T& obj = *pool.at(i);
    // ...
}
```


Never allocates: no `new`, no `malloc`, no `std::vector`. With
`-fno-exceptions`, a constructor cannot fail, so there is no
partially-constructed-slot case to unwind.

Copy construction and copy assignment are deleted — a pool's slot
pointers are identity, not value; copying the raw bytes without
re-running every live `T`'s copy constructor would alias two pools over
one set of resources.

See this file's header-level doc comment for why arena-backed pool
storage is NOT supported (design.md D6).

## Methods

### `T* acquire(Args&&... args)`

**Description:**

Constructs a `T` in a free slot, forwarding `args` to `T`'s
       constructor.

**Returns:** A pointer to the newly constructed, live object, or `nullptr`
        when the pool is full — every existing live slot is left
        unmodified in that case.

Never allocates (no `new`, no `malloc`). `T` does NOT need a default
constructor; only a constructor matching `args` is required.

### `bool release(T* object)`

**Description:**

Runs `~T()` on the slot holding `object` and frees it.

**Returns:** `true` if a live slot was released; `false` (safe no-op, no
        bookkeeping change, `~T()` NOT invoked again) for `nullptr`, a
        pointer foreign to this pool, or a slot that is already free.

### `bool releaseAt(uint16_t index)`

**Description:**

Runs `~T()` on the slot at `index` and frees it.

**Returns:** `true` if the slot was live and is now freed; `false` (safe
        no-op) for an out-of-range index or a slot that is already
        free.

### `void reset()`

**Description:**

Runs `~T()` on every live slot, ascending index order, then
       clears all bookkeeping. Safe to call on an empty pool.

### `uint16_t capacity() const`

**Description:**

Fixed slot capacity (same value as kCapacity).

### `uint16_t size() const`

**Description:**

Number of slots currently live.

### `bool isFull() const`

**Description:**

True when size() == capacity().

### `bool isLive(uint16_t index) const`

**Description:**

True when `index` is in range and currently holds a live object.

### `T* at(uint16_t index)`

**Description:**

Recovers a pointer to the live object at `index`.

**Returns:** A `std::launder`-corrected `T*`, or `nullptr` if `index` is
        out of range or dead.

### `uint16_t indexOf(const T* object) const`

**Description:**

Finds the slot index owning `object`.

**Returns:** The slot index if `object` was returned by this pool's
        `acquire()` and is still live; `kEnd` for a null, foreign, or
        no-longer-live pointer.

### `uint16_t nextLive(uint16_t from) const`

**Description:**

Returns the first live index `>= from`, or `kEnd` when none
       remains.
