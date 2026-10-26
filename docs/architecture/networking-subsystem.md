# Networking Subsystem (ESP-NOW)

The optional `pixelroot32::networking` module gives a game a peer-to-peer link
without a router, an access point or an IP address. It ships behind
`PIXELROOT32_ENABLE_NETWORKING`, defaults to `0`, and contributes no symbols or
Engine bytes when it is off.

The module has two transports: a real ESP-NOW backend for the ESP32 family and
an in-process loopback for native builds and deterministic tests. Above them,
`NetSession` owns discovery, the peer table, reliability and the event queue;
`NetCounter` is the reference implementation of a shared value that converges
from idempotent operations.

> **Status:** the module is experimental for one minor release. The wire
> protocol carries a version byte so it can evolve.
>
> **Guarantee, stated once:** the module guarantees **convergence**, not
> lockstep. The same set of applied operations produces the same value on every
> peer, independent of arrival order and duplicates. It does not promise
> identical frame-by-frame simulation, and the engine has no fixed-tick loop to
> build one on. This is the same scope correction the physics documentation
> records: determinism is per build, not across hardware or targets (issue
> [#244](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/issues/244),
> [physics-subsystem.md](physics-subsystem.md)).

---

## 1. Layers and ownership

```text
Scene / game code
    onNetworkEvent(NetEvent)
            ^
Engine::update()        drain + dispatch, only when the flag and a session are set
            |
NetSession              peers, discovery, reliability, dedupe, stats
            |
NetTransport            abstract frame contract
    |-- ESP32EspNowTransport   drivers/esp32  (real radio, not unit-testable)
    `-- LoopbackTransport      drivers/native (in-process, deterministic)
```

| Layer | Responsibility | Must not |
|-------|----------------|----------|
| `NetTransport` | Send and receive complete frames, register peers, own the channel and local address | Parse the wire header, sequence messages, ACK, retry or touch game state |
| `ESP32EspNowTransport` | Station and ESP-NOW setup, peer registration, RX/TX callbacks, SPSC staging rings, error mapping | Touch session or scene state from a callback |
| `LoopbackTransport` | In-process bus, virtual clock, fault injection | Depend on ESP32 headers or the system clock |
| `NetSession` | Discovery, peer table, reliability, dedupe, RX event queue, stats, timeouts | Allocate, block or call the scene |
| `NetCounter` | Convergent signed accumulator, snapshots, resync window | Own the session queue or the transport |
| `Engine` | Pump one borrowed session and dispatch drained events to the current scene | Own, initialize or shut down the transport or session |
| `Scene` | Handle `onNetworkEvent` | Be called from the Wi-Fi task |

### Ownership

The application owns the transport, the session and the counter. `Engine` only
borrows the session through `setNetworkSession(&session)`, the same shape as
`setTouchManager()`. It never keeps more than the raw pointer, never initializes
it and never shuts it down. Passing `nullptr` disables networking entirely.

The transport must be initialized before `NetSession::init()`: a session on a
dead link returns `NetResult::NotInitialized` instead of half-working.
`NetSession::shutdown()` sends a best-effort BYE and clears session state, but
deliberately leaves the transport running so the application stays in control
of the link lifetime.

### Engine and Scene dispatch

With a session registered, every `Engine::update()` does:

1. update input,
2. call `session.update(nowMs)` once,
3. drain the session queue with `pollEvent()` and call
   `Scene::onNetworkEvent()` for each event,
4. update the current scene,
5. dispatch touch, as before.

Dispatch happens **before** `Scene::update()` so a scene can react to network
input and draw the result in the same frame.

Events are delivered to whichever scene is current **at drain time**. A scene
swap does not clear the queue: a reliable message may already have been ACKed to
the sender, and silently discarding it would break the reliability contract.
Scenes that do not care inherit the default no-op hook and pay only for the
virtual slot.

---

## 2. Transport contract

A transport moves **complete frames**, never an application payload on its own.
The session frames before sending and the transport never re-frames.

| Contract | Value |
|----------|-------|
| Header | 16 bytes |
| Default application payload | 64 bytes (`PIXELROOT32_NET_MAX_PAYLOAD`) |
| Default configured frame | 80 bytes = 16 + 64 |
| Hard application ceiling | `kNetMaxAppPayload` = 250 - 16 = **234 bytes** |
| Largest ESP-NOW frame | 250 bytes |

`kNetTransportFrameSize` is `NetHeaderSize + NetMaxPayload`, and a
`static_assert` fails the build if it exceeds 250. `NetMaxPayload + 16 <= 250`
is asserted again in `platforms::config`. A larger payload is rejected at
`NetSession::send()` with `NetResult::PayloadTooLarge`; it never reaches the
wire.

### `NetTransport`

Every method is called from the main loop. `send()` never blocks: `Ok` means the
link accepted the frame, and the delivery outcome arrives later through
`pollTxResult()` in FIFO order. `pollRx()` and `pollTxResult()` never block and
never allocate.

`NetTransportConfig` carries the channel (1..13), the application label
(`nodeId == 0` derives it from the local MAC) and `manageWifi`. The transport
owns peer addresses and the link; it does not know about sequence numbers, ACKs
or game messages.

### ESP32 backend

`ESP32EspNowTransport` brings the station interface up without joining an
access point, because an association would move the channel and drop the link.
It registers the broadcast address for HELLO, registers unicast peers as they
are discovered, and keeps every peer on the interface channel so `setChannel()`
moves them without re-registration.

ESP-NOW callbacks run in the Wi-Fi task. They copy a frame into a 4-entry RX ring
and a send result into a 4-entry TX ring, both single-producer/single-consumer
with atomic head and tail indices, and increment a drop counter. They never log,
allocate, parse a header or reach session state. A full ring drops the newest
entry, matching the session's own drop-newest policy. `ESP_ERR_ESPNOW_NO_MEM`
maps to `NetResult::Busy`, so a momentarily full radio does not lose a reliable
message.

ESP-NOW callbacks are process-global, so exactly one instance may own the radio.
A second `init()` returns `NetResult::Busy` until the owner shuts down.
`shutdown()` unregisters callbacks before clearing the rings, deinitializes
ESP-NOW, and only turns Wi-Fi off when this transport brought it up
(`manageWifi`).

### Native backend

`LoopbackTransport` copies frames directly between transports attached to the
same `LoopbackBus`, on the same channel. There is no socket, thread or ESP32
header in the native path. The bus is the simulation's clock and its fault
injector: it holds the current `nowMs`, one xorshift32 generator and a fixed list
of at most 8 transports. Time never advances on its own, so a test moves the
clock explicitly.

`LoopbackFaults` injects, in this order per destination, `lossPercent`, then
`duplicatePercent`, then `reorderPercent`, plus `delayMs`, `sendBusy` and a
`seed`. Reinstalling the same profile restarts the same deterministic sequence.
Peer registration is a documented no-op because an address is resolved against
the attached transports; `addPeer()` and `removePeer()` always return `Ok`.
Broadcast reaches every other attached transport on the same channel. A unicast
send to a transport on another channel is accepted but reported as an
unsuccessful result and never delivered.

---

## 3. Wire format

All multi-byte fields are little-endian and encoded field by field. No C++
struct is copied to the wire, so padding and host endianness cannot leak. The
magic is the one field that is not a numeric little-endian value: bytes 0 and 1
are the ASCII characters `P` and `R`, read as `0x5052`.

| Offset | Size | Field |
|--------|------|-------|
| 0 | 2 | Magic `0x5052` ("PR") |
| 2 | 1 | Protocol version (1) |
| 3 | 1 | Type: Data (1), Hello (2), Bye (3), Ack (4), Snapshot (5) |
| 4 | 1 | Flags: bit 0 reliable; all other bits reserved and must be zero |
| 5 | 1 | Source `nodeId` |
| 6 | 2 | Source `bootId` |
| 8 | 4 | Sequence (reliable Data only; 0 is the "no ACK" sentinel) |
| 12 | 2 | Reserved ack field (0 in v1) |
| 14 | 2 | Payload length |
| 16 | N | Payload |

The decoder validates in a fixed order: short frame, magic, version, type,
flags, payload ceiling, and finally declared length against the real frame
length. A rejected frame yields exactly one reason through `NetParseResult`.

### ACK payload rule

The header's 16-bit ack field is reserved and always `0` in v1. An ACK carries
the acknowledged **32-bit** sequence as a 4-byte little-endian payload. The
header field is too narrow: truncating the sequence to 16 bits would let a stale
ACK alias a newer pending message after 65,536 sends. An ACK has no application
payload and is never queued or retried; the peer's own retransmission is what
recovers a lost ACK.

### Control frames

| Type | Payload | Notes |
|------|---------|-------|
| Hello | 4-byte sender uptime | `nodeId` and `bootId` come from the header; the source MAC comes from the link |
| Bye | none | Best effort on shutdown |
| Ack | 4-byte sequence | See above |
| Snapshot | reserved | `NetPacketType::Snapshot` is defined but not used by v1; the session counts one and ignores it |

The counter's snapshot is **not** the wire `Snapshot` type. It travels as an
application Data message with an application tag, so the session still owns
retries, dedupe and ACKs (see section 6).

---

## 4. Reliability

Reliable delivery is opt-in per message (`NetDelivery::Reliable`). Unreliable
messages are fire-and-forget: no sequence, no ACK, no retry and no dedupe.

```text
send(reliable) -> Queued -> Sent -> ACK received -> Done
                              |
                              +-> timeout -> Retry (retries < maxRetries) -> Sent
                              |
                              +-> retries == maxRetries -> Failed + SendFailed event
```

- Each peer has a monotonic `uint32_t` sequence. Zero is skipped on wrap because
  it is the wire sentinel.
- The session encodes the frame once and keeps the **whole encoded frame** in a
  fixed pending queue (8 entries by default). A retry is byte-identical to the
  first transmission, including its sequence, and never re-encodes against a
  sequence that has already advanced.
- An ACK removes the pending entry. The first transmission also yields a clean
  RTT sample; a retried frame would include the timeout and does not.
- After `ackTimeoutMs` (20 ms default) a sent entry retries. After `maxRetries`
  (3 default) retransmissions it is removed, `txFailed` is incremented and a
  `NetEventType::SendFailed` event is queued for the application.
- An ACK for an unknown or already-retired sequence is ignored, not an error.
- All time comparisons use unsigned subtraction (`now - sentAt >= timeout`), so
  the 32-bit millisecond wrap is handled without special cases.

### Dedupe window

Each peer keeps `rxHighestSeq` and a 64-bit window of accepted older sequences.
Both use serial-number arithmetic, not plain numeric comparison:

- A sequence ahead of `rxHighestSeq` by a small forward distance is new. If the
  gap is 64 or wider, every older bit is stale and the window is cleared;
  otherwise the window shifts forward. Bit 0 always names the newest sequence.
- At or below the newest sequence, only the last 64 are remembered. An age below
  64 whose bit is clear is accepted and its bit is set; an age of 64 or more, or
  a set bit, is a duplicate.
- A duplicate is **not delivered again** but is **ACKed again**, because the
  original ACK may be the frame that was lost. `rxDuplicates` counts it.
- A new `bootId` resets that peer's window and fails pending messages for it:
  they belong to the old boot's sequence space and can never be acknowledged.

Reliable delivery is **effectively-once, not globally ordered**. Games that need
order must carry their own logical clock or use the versioned counter.

### Backpressure

A full reliable pending queue makes `send()` return `NetResult::Busy` before a
sequence is assigned, so a retry by the caller leaves no gap on the wire. A busy
**transport** is different: a reliable send is accepted and returns `Ok`, the
entry is marked unsent, and `retryPending()` attempts it on a later update under
the same sequence. An unsent entry cannot consume the retry budget because
there is nothing to retry yet. An unreliable send that hits transport `Busy` is
reported to the caller and nothing is queued, because there is no state to hold
it.

### Stats

`NetStats` is monotonic for the lifetime of the session: `txMessages`,
`rxMessages`, `txRetries`, `txFailed`, `rxDropped`, `rxDuplicates`, `txBusy` and
`lastRttMs`. Rates come from differencing two snapshots.

---

## 5. Discovery and lifecycle

- A node broadcasts HELLO on the first `update()` after `init()` and then once
  per `helloIntervalMs` (1 s default). The payload is its uptime.
- `shutdown()` sends one best-effort BYE.
- `nodeId` is an application label. `0` asks the session to derive it from the
  local MAC: the last MAC byte, with `0` and `255` mapped to `1` (both are
  reserved markers). `255` is the broadcast marker.
- `bootId` is a per-boot generation. `0` asks the session to generate a non-zero
  value: ESP32 uses the hardware RNG, native mixes the wall clock, a
  process-local counter and the MAC. An explicit boot id wins. A generated value
  is kept for the object's whole lifetime, because a re-init reconfigures a
  running session; it is not a reboot.
- An unknown source MAC with discovery on allocates a slot, registers the peer
  with the transport and emits `PeerJoined`. A known peer refreshes `lastSeen`
  and uptime. A new boot id resets dedupe, preserves the slot index.
- Silence for `peerTimeoutMs` (3 s default) marks the peer offline and emits
  `PeerLeft`. The slot is never freed: it stays allocated and keeps its index.
- Any valid frame from an offline peer revives it, emits `PeerJoined` again and
  then delivers the message. The second join is the counter's resync trigger.
- Peer identity is the MAC. `nodeId` is only a label. A collision is **not**
  detected by the session; two nodes with the same label confuse application
  routing and the counter, so assign unique ids.
- With `discovery = false`, unknown HELLOs are dropped and only pre-seeded
  peers are accepted. `addPeer()` stores a slot marked offline, registers the
  address with the transport and emits no event; a locked deployment seeds both
  sides before traffic starts.
- A full peer table drops a new HELLO and counts the drop; `addPeer()` returns
  `PeerTableFull`. The session never evicts a peer that may still be exchanging
  reliable traffic.
- Broadcast is internal to discovery: HELLO and BYE are the only broadcast
  frames. The application sends unicast to a peer slot; there is no
  application-level broadcast API in v1.

---

## 6. Counter and resync

`NetCounter` is the v1 reference implementation of "deterministic state sync"
for one signed value that every peer mutates:

```text
value = sum(slot[i].value)
slot[i] = sum of deltas applied from node i
```

Each node owns one slot. An operation is idempotent and keyed by
`(nodeId, bootId, sequence)`. Addition is commutative, so arrival order does not
matter, and duplicate suppression is already the session's job.

### Operation wire layout (5 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 1 | `kCounterOpTag` (1) |
| 1 | 4 | `int32` delta, sign-extended from the `int16_t` API value |

### Snapshot wire layout (2 + 11 * slotCount bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 1 | `kCounterSnapshotTag` (2) |
| 1 | 1 | Slot count |
| 2 + 11*i | 1 | `nodeId` |
| 3 + 11*i | 2 | `bootId` |
| 5 + 11*i | 4 | `value` (`int32`) |
| 9 + 11*i | 4 | `cursor` (`uint32`) |

At the default four slots that is 46 bytes, inside the 64-byte payload. A
`static_assert` rejects a configuration whose snapshot cannot fit in one
payload.

### Uptime rule

A node that boots after the others must not start from zero. On
`PeerJoined`, both sides compare the local clock with the peer's advertised
uptime:

1. If the local uptime exceeds the peer's by more than 1 s
   (`kCounterUptimeTieMs`), the local node sends a reliable snapshot.
2. If the peer's uptime exceeds the local one by more than 1 s, the local node
   enters `Resyncing` and waits for a snapshot.
3. If the difference is within 1 s (simultaneous boot), the lower `nodeId`
   sends and the higher one waits.

The rule selects exactly one sender on both sides, deterministically.

### Snapshot and resync

A snapshot is sent as `NetDelivery::Reliable` with the application snapshot tag,
so the session still owns retries and dedupe. A valid snapshot replaces the
whole slot table (a state transfer, not a merge), raises `revision()` once, ends
resync and replays operations that arrived while the snapshot was in flight.

The resync buffer holds up to 4 events (`kCounterResyncBufferCapacity`) in
arrival order, drop-newest, so a flood cannot evict an operation that arrived
before the snapshot. Replay is the only path with a cursor check, because the
snapshot may already include an operation that was buffered before it landed.

While `isResyncing()` is true, `add()` answers `NetResult::Busy`: applying a
delta now would be lost when the table is replaced. The game may drop the input
that produced the call or queue it and retry after the flag clears.

If no snapshot arrives within `kCounterResyncTimeoutMs` (3 s, matching the peer
timeout), the node resumes with `isOutOfSync()` raised. The value stays usable,
but it may have diverged, and a press made during the window may have been
dropped. A `PeerLeft` for the awaited peer cancels the resync without raising the
flag, because the next join starts a fresh attempt.

### Explicit snapshot request

A power-cut reboot can be faster than the survivor's 3 s peer timeout. The
survivor still considers the rebooted node online, so it never runs its own join
path and never sends a snapshot. The younger node therefore sends an explicit
1-byte request (`kCounterSnapshotRequestTag`, 3) when it enters resync. The
survivor answers with a snapshot; without the request the younger node would
wait out the full timeout even though the survivor is reachable.

### Convergence, not lockstep

The guarantee is that the same set of applied operations yields the same value
on every peer, independent of arrival order and duplicates. Two peers may show
different values for a few frames while an operation or a snapshot is in
flight. The module cannot promise identical frame-by-frame simulation, and the
engine does not have a fixed-tick loop. This is the same corrected scope the
physics subsystem documents: determinism is per build, not across hardware or
targets (issue
[#244](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/issues/244),
[physics-subsystem.md](physics-subsystem.md)).

`NetCounter` is deliberately not a generic serializer. Arbitrary object
replication would require a serialization layer the engine does not have, and
would multiply the API surface without serving the reference use case.

---

## 7. Threading

| Context | Allowed |
|---------|---------|
| Wi-Fi task (ESP-NOW callbacks) | Copy a frame into the transport ring, push the send result, increment a drop counter. Nothing else. |
| Main loop (`Engine::update` -> `NetSession::update`) | Drain the transport, validate, dedupe, ACK, emit events, run timers, send retries |
| Application (`Scene::onNetworkEvent`, `NetCounter::add`) | Read events, mutate game state, enqueue sends |

`NetSession`'s event queue is deliberately not atomic, like
`GameplayEventBus`: the transport publishes frames only through `pollRx()`, so
only the main loop touches it. The transport staging rings are the only atomic
state, and they are strictly one producer and one consumer. No scene code may
run from a callback.

The native path is entirely single-threaded: the bus clock is advanced by the
caller, never by a background thread.

---

## 8. Memory

Every table and queue is a fixed array sized at compile time. Nothing allocates
after construction and `init()`. At the defaults the module's own static
footprint is budgeted at about 2 KB:

| Item | Estimate |
|------|----------|
| RX event queue (8 x 76 B `NetEvent`) | ~608 B |
| Pending reliable queue (8 entries, each holding a complete frame) | ~672 B |
| Peer table (4 slots) | ~128 B |
| Session config, stats and state | ~80 B |
| Transport RX staging (4 entries per ESP32 transport) | ~288 B |
| Counter (4 slots plus state) | ~56 B |
| **Static total** | **~1.8 KB, about 2 KB** |

`NetEvent` is 76 bytes at the default payload: one byte each for type, peer
slot and node id, four for the sequence, two for the length, and the inline
64-byte payload, with alignment padding. The hard design gate is 2.5 KB of
module-owned static RAM, pinned by `static_assert`s on the main types. Larger
payloads and peer counts raise the cost linearly: every queue entry carries the
payload inline, and every peer slot is reserved for the session lifetime.

These figures exclude the IDF Wi-Fi and ESP-NOW stacks and the ESP-NOW peer
entries allocated by the IDF. Those are real on hardware and are measured, not
budgeted.

### Example measurements

`examples/esp_now` reports, on clean builds, the same totals for both C3
environments because only two constants differ:

| Environment | RAM | Flash |
|-------------|-----|-------|
| `esp32c3_node_up` | 42,492 B (13.0% of 327,680) | 749,824 B (57.2% of 1,310,720) |
| `esp32c3_node_down` | 42,492 B (13.0% of 327,680) | 749,824 B (57.2% of 1,310,720) |

Those are **example-wide** figures. They include the Arduino core, the Wi-Fi and
ESP-NOW stacks, U8G2, the display buffers and the Engine, not just this module.
Do not read them as the module's cost.

---

## 9. Limitations

| ID | Limitation |
|----|------------|
| L1 | **No encryption or authentication.** v1 registers every peer unencrypted and ignores `NetPeerConfig::encrypted`. Traffic is only as private as the radio's coverage. LMK provisioning is planned for v2. |
| L2 | **No fragmentation.** One ESP-NOW frame is at most 250 bytes and the hard application ceiling is 234 bytes. Larger data needs application-level chunking. |
| L3 | **One channel.** Both peers must use the same channel (1..13). Joining an access point can move the radio to the AP's channel and silently drop the link. |
| L4 | **Peer ceilings.** ESP-NOW allows 20 peers and 6 encrypted peers; the default table is 4. The encrypted ceiling does not apply in v1 because encryption is not implemented. |
| L5 | **No hardware in CI.** RF behavior, latency, heap and frame time are validated manually. The loopback covers protocol and logic only. |
| L6 | **No persistence.** If both nodes lose power, the shared value resets to 0. A single reboot is restored by a live peer. |
| L7 | **Discovery trusts HELLO.** With discovery on, any sender in range can join the peer table. Use `discovery = false` plus pre-seeded peers for locked deployments. |
| L8 | **Arduino core range.** Arduino-ESP32 2.0.17 and 3.3.12 are verified. A compile-time shim covers the 3.x send and receive callback signatures; the CI examples matrix builds the 3.x branch. |
| L9 | **One ESP-NOW instance per process.** The callbacks are process-global; a second transport receives `Busy` from `init()` until the owner shuts down. |
| L10 | **Not IP.** No routing, no internet, no OTA synergy. ESP-NOW is a link-layer protocol. |
| L11 | **Node id collisions are not detected.** The MAC is the identity; the label is the application's responsibility. |
| L12 | **Drop-newest queues.** A full RX event queue drops the newest event and counts it; a full staging ring does the same. A full reliable pending queue makes `send()` return `Busy`. |
| L13 | **No generic serialization.** `NetCounter` is the reference state-sync helper, not a serializer for arbitrary game state. |
| L14 | **Unordered reliability.** Reliable messages are effectively-once, not ordered. Order-sensitive games need their own clock. |
| L15 | **Experimental for one minor release.** The protocol version byte allows the wire format to evolve, but the API may change before it stabilizes. |

---

## 10. Where to look

| What | Where |
|------|-------|
| Types, events, results | [`include/networking/NetTypes.h`](../../include/networking/NetTypes.h) |
| Wire constants and codec | [`include/networking/NetProtocol.h`](../../include/networking/NetProtocol.h) |
| Transport contract | [`include/networking/NetTransport.h`](../../include/networking/NetTransport.h) |
| Session and peer table | [`include/networking/NetSession.h`](../../include/networking/NetSession.h) |
| Convergent counter | [`include/networking/NetCounter.h`](../../include/networking/NetCounter.h) |
| ESP-NOW backend | [`include/drivers/esp32/ESP32EspNowTransport.h`](../../include/drivers/esp32/ESP32EspNowTransport.h) |
| Native loopback | [`include/drivers/native/LoopbackTransport.h`](../../include/drivers/native/LoopbackTransport.h) |
| Protocol tests | [`test/unit/test_net_protocol/`](../../test/unit/test_net_protocol/) |
| Discovery and reliability tests | [`test/unit/test_net_session_discovery/`](../../test/unit/test_net_session_discovery/), [`test/unit/test_net_reliability/`](../../test/unit/test_net_reliability/) |
| Counter and resync tests | [`test/unit/test_net_counter/`](../../test/unit/test_net_counter/), [`test/unit/test_net_counter_resync/`](../../test/unit/test_net_counter_resync/) |
| Engine dispatch tests | [`test/unit/test_net_engine_dispatch/`](../../test/unit/test_net_engine_dispatch/) |
| Two-board demo | [`examples/esp_now/README.md`](../../examples/esp_now/README.md) |
| Task-oriented guide | [Networking guide](../guide/networking.md) |
| Conceptual API | [Networking API](../api/networking.md) |
| Generated API | [Generated networking pages](../api/generated/networking/NetSession.md) |
| Build flags | [Configuration](../api/config.md#networking-limits) |
