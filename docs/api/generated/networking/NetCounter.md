# NetCounter

<Badge type="info" text="Class" />

**Source:** `NetCounter.h`

## Description

Convergent signed accumulator replicated as idempotent operations.

Every node owns one slot and publishes its own deltas. The shared value is
the sum of every used slot, so the same set of applied operations produces
the same value on every peer regardless of arrival order. This is the v1
reference implementation of "deterministic state sync": convergence, not
lockstep.

Wire layout of one operation (5 bytes, little endian):

```cpp
  byte 0      : kCounterOpTag
  bytes 1..4  : int32 delta, sign extended from the int16_t API value
```


Wire layout of one snapshot (2 + slotCount * 11 bytes, little endian):

```cpp
  byte 0      : kCounterSnapshotTag
  byte 1      : slotCount
  bytes 2..   : slotCount entries of 11 bytes each:
                nodeId u8, bootId u16, value i32, cursor u32
```

At the default limits that is 46 bytes, inside the 64-byte payload. The
compile-time assert below rejects a configuration whose table cannot be
exported into one payload.

Duplicate and ordering contract: the counter never rejects an operation for
arriving late or twice. NetSession already suppresses duplicates with its
per-peer sequence window, and addition is commutative, so a second guard here
would only add state. An event fed to onEvent twice is therefore applied
twice, by design; the session is the single owner of effectively-once
delivery. Cursors are only consulted when replaying operations that were
buffered during resync.

Replication contract: add sends one Reliable operation to every online
peer, so the link, not the caller, retries. The local slot is updated before
the send, because the game reads its own input in the same frame; a failed
send is reported but never rolled back.

Budget contract: one fixed slot table sized at compile time and no heap, no
exceptions and no logging. Construction reserves the whole footprint.

Ownership contract: the counter borrows a session that outlives it. It never
owns, initializes or shuts the session or the transport down, and it never
touches the transport directly.

Threading contract: every method must be called from the main loop, next to
NetSession::update. The counter holds no locks and no atomics.

Slice boundary: this class owns operations, snapshot exchange and the short
resync window a node enters when it boots after its peers. A snapshot is an
application Message payload (tag kCounterSnapshotTag) sent with
NetSession::send under NetDelivery::Reliable; the session still owns
retries, dedupe and ACKs.

Resync contract: a node that joins late calls update once per frame. On
NetEventType::PeerJoined the better-established node (higher uptime, or
the lower node id inside kCounterUptimeTieMs) sends a snapshot, and the
younger node enters resync. While resyncing add answers
NetResult::Busy and incoming operations are buffered instead of applied.
A valid snapshot replaces the table, raises revision once, and the
buffer is replayed against the imported cursors. If no snapshot arrives
before kCounterResyncTimeoutMs the node resumes with
isOutOfSync raised; a press made during the window may have been dropped
and the game can use that flag to surface the divergence.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `kMaxSlots` | `static constexpr uint8_t` | Number of fixed slots, local node included. |

## Methods

### `NetResult init(NetSession& session)`

**Description:**

Binds the counter to a session and allocates the local slot.

**Parameters:**

- `session`: Session this counter reads and sends through; it must
       outlive the counter.

**Returns:** Always NetResult::Ok.

The session is assumed to be initialized: this class has no way to ask,
and it only needs the local identity. The local slot is rebuilt with
value 0 at the configured @ref NetSession::localNodeId and

### `void shutdown()`

**Description:**

Clears every slot and detaches from the session.

### `NetResult add(int16_t delta)`

**Description:**

Applies one delta locally and replicates it to every online peer.

**Parameters:**

- `delta`: Signed change; 0 is a no-op.

**Returns:** NetResult::NotInitialized before @ref init, NetResult::Busy while

### `void onEvent(const NetEvent& event)`

**Description:**

Applies one session event: a counter operation, a snapshot, or a
       peer lifecycle change.

**Parameters:**

- `event`: Event just popped from the session queue.

A

### `void update(uint32_t nowMs)`

**Description:**

Advances time-dependent counter state.

**Parameters:**

- `nowMs`: Current time in milliseconds. May wrap; comparisons are
       unsigned or serial-number based.

Stores the local uptime used by the join rule, retries one deferred
snapshot send, and expires an overdue resync. A full reliable queue makes
a snapshot send answer Busy; the peer is remembered and one retry is
attempted per call, so an update never spins and never waits.

Must be called once per frame, before

### `bool isResyncing() const`

**Description:**

Reports whether the node is waiting for a snapshot.

**Returns:** true between entering resync and the import, timeout, or peer
        departure that ends it.

### `bool isOutOfSync() const`

**Description:**

Reports whether a resync gave up without a snapshot.

**Returns:** true after a resync timeout, until the next resync attempt or

### `int32_t value() const`

**Description:**

Returns the converged value.

**Returns:** Sum of every used slot, clamped into the int32 range.

The sum is accumulated in 64 bits so several saturated slots can never
overflow the arithmetic before the final clamp.

### `uint32_t revision() const`

**Description:**

Returns how many real value changes were applied.

**Returns:** Monotonic count of local and remote changes.

Counts changes, not operations: an operation that lands on an already
saturated slot does not bump it, and an ignored operation (zero delta,
malformed payload, full table) does not either. A game can use two
samples to know whether the shared value moved since the last frame.
