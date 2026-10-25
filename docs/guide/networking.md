# Networking (ESP-NOW)

The `pixelroot32::networking` module gives a game a router-free peer-to-peer
link: discovery, per-message reliability, and a convergent shared counter.
ESP-NOW ships with the Arduino ESP32 core, so there is no extra library to add.

This page is task-oriented. For the wire format, threading and memory budget,
see the [Networking subsystem](../architecture/networking-subsystem.md)
(authoritative). For method-level detail, see the
[Networking API](../api/networking.md) and the headers.

> **Scope:** the module guarantees convergence, not lockstep. The same applied
> operations produce the same value on every peer, whatever the arrival order.
> It does not make two boards draw identical frames. See
> [Convergence, not lockstep](#convergence-not-lockstep).

---

## 1. Enable the module

```ini
build_flags =
    -D PIXELROOT32_ENABLE_NETWORKING=1
```

- Default is `0`. With the flag off the module contributes no symbols, no
  `Engine` bytes and no `Scene` vtable slot.
- ESP-NOW and `WiFi` come from the Arduino ESP32 core. No `lib_deps` entry is
  required.
- The module needs a transport. ESP32 targets use the real radio; native builds
  use `LoopbackTransport`, which has no radio and no sockets. Any other target
  fails the build with a named `#error` when the flag is on.
- The default application payload is 64 bytes and the peer table holds 4 peers.
  Override with `PIXELROOT32_NET_MAX_PAYLOAD` and
  `PIXELROOT32_NET_MAX_PEERS`; the full list is in
  [Configuration](../api/config.md#networking-limits).

Enable it for the whole library and every translation unit that includes the
engine, not only for one source file: the flag changes header contents.

---

## 2. Setup

### ESP32 composition

```cpp
#include <core/Engine.h>
#include <drivers/esp32/ESP32EspNowTransport.h>
#include <networking/NetCounter.h>
#include <networking/NetSession.h>

using namespace pixelroot32::networking;

namespace {
ESP32EspNowTransport transport;
NetSession session(transport);
NetCounter counter;
GameScene scene;
}  // namespace

void setup() {
    engine.init();

    NetTransportConfig transportConfig;
    transportConfig.channel = 1;
    transportConfig.nodeId = 1;          // 0 derives the label from the MAC
    transportConfig.manageWifi = true;
    if (transport.init(transportConfig) != NetResult::Ok) {
        // No radio: report it and keep the game running without networking.
    }

    NetSessionConfig sessionConfig;
    sessionConfig.channel = 1;           // must match the peer
    sessionConfig.nodeId = 1;            // must be unique across peers
    sessionConfig.discovery = true;
    if (session.init(sessionConfig) != NetResult::Ok) {
        // Bad configuration or a transport that is not ready.
    }

    if (counter.init(session) != NetResult::Ok) {
        // The counter assumes an initialized session.
    }

    engine.setScene(&scene);
    engine.setNetworkSession(&session);  // Engine borrows; it never owns
}
```

Order matters:

1. Bring the transport up first. `NetSession::init()` returns
   `NotInitialized` on a dead link.
2. Start the session. It applies the channel to the transport.
3. Bind the counter to the session.
4. Register the session with the Engine. From then on, every
   `Engine::update()` pumps it once and dispatches drained events.

`engine.setNetworkSession(nullptr)` disables dispatch without shutting
anything down. The application stays responsible for `shutdown()` of the
counter, the session and the transport.

### Scene hook and counter

```cpp
class GameScene : public pixelroot32::core::Scene {
public:
    void update(unsigned long deltaTime) override {
        (void)deltaTime;
        const uint32_t nowMs = static_cast<uint32_t>(millis());
        counter.update(nowMs);           // once per frame

        if (engine.getInputManager().isButtonPressed(kApplyButton)) {
            const NetResult result = counter.add(1);
            // Busy means a snapshot is being imported: drop or queue this press.
            (void)result;
        }
    }

#if PIXELROOT32_ENABLE_NETWORKING
    void onNetworkEvent(const NetEvent& event) override {
        counter.onEvent(event);          // operations, snapshots and lifecycle
        if (event.type == NetEventType::SendFailed) {
            // A reliable message gave up after its retries.
        }
    }
#endif
};
```

The Engine calls `onNetworkEvent()` before `Scene::update()`, on whichever scene
is current when the session queue is drained. A scene swap does not discard
queued events; they are delivered to the scene that is current at drain time.

### `update()` before `send()`

`NetSession::send()` stamps a reliable pending entry with the time of the last
`session.update()`. Call `session.update()` once per frame **before** any send,
or the first ACK timeout is measured from a stale timestamp (zero, before the
first update).

When the session is registered with the Engine, the Engine already does this
before `Scene::update()`, so a `send()` from the scene is safe. If you pump a
session yourself, keep the order explicit:

```cpp
counter.update(nowMs);     // counter clock first
session.update(nowMs);     // drain transport, send HELLO, run timers
NetEvent event;
while (session.pollEvent(event)) {
    counter.onEvent(event);
}
```

With Engine-managed dispatch the counter's stored uptime is at most one frame
old. That is far inside the 1 s uptime tie window the resync rule uses, so the
example drives `counter.update()` from `Scene::update()` and still converges.

---

## 3. Channel

Both nodes must use the **same Wi-Fi channel, 1..13**. ESP-NOW is not IP: there
is no router, no access point association, no DHCP and no socket.

- Do not connect either board to a router. Joining an AP can move the radio to
  the AP's channel and silently drop the link.
- `NetSessionConfig::channel` is applied to the transport by
  `NetSession::init()`; `ESP32EspNowTransport::setChannel()` can move it later.
- A channel mismatch is the most common "the boards do not see each other"
  cause. Both peers keep running and show no peer.

The example fixes channel 1 in one place. Its build-time `#error` guards the
node/delta pairing, not the channel, so a channel mismatch is still a runtime
symptom to diagnose.

---

## 4. Reliability

Choose the contract per message:

| Delivery | Behaviour |
|----------|-----------|
| `NetDelivery::Unreliable` | Fire-and-forget. No sequence, no ACK, no retry, no dedupe. |
| `NetDelivery::Reliable` | Gets a sequence, is ACKed and retried up to `maxRetries`; duplicates are suppressed. |

`send()` returning `Ok` means the **session accepted** the message, not that the
peer received it. A reliable message may still be waiting for transport room,
and a delivery failure arrives later as `NetEventType::SendFailed` with the peer
slot and sequence.

| Result | Meaning |
|--------|---------|
| `Ok` | Accepted. Reliable may be queued; unreliable was handed to the transport. |
| `Busy` | Reliable pending queue is full, or an unreliable send found no transport room. |
| `PeerNotFound` | The slot exists but the peer is offline. Data never creates a peer. |
| `PayloadTooLarge` | Above the configured payload ceiling. |
| `InvalidArgument` | Unused or out-of-range slot, or null data with a non-zero length. |
| `NotInitialized` | The session is not initialized. |

Reliable delivery is **effectively-once, not ordered**. If order matters, carry
a logical clock in the payload.

---

## 5. The convergent counter

`NetCounter` is a signed accumulator shared by every peer:

```text
value = sum(slot[i].value)
```

Each node owns one slot and publishes its own deltas. An operation carries
`{nodeId, bootId, sequence, delta}` and is applied once per sequence by the
session's dedupe window. Addition is commutative, so arrival order does not
matter.

| Call | What it does |
|------|--------------|
| `counter.init(session)` | Binds the counter and allocates the local slot. |
| `counter.add(delta)` | Applies locally first, then replicates one reliable operation to every online peer. |
| `counter.onEvent(event)` | Routes a drained `NetEvent` into the counter. Call it for every event, not only `Message`. |
| `counter.update(nowMs)` | Advances the uptime, deferred snapshot send and resync timeout. Call once per frame. |
| `counter.value()` | The converged `int32_t` value. |
| `counter.revision()` | Count of real value changes; useful to know whether to redraw. |
| `counter.isResyncing()` | True while waiting for a snapshot. |
| `counter.isOutOfSync()` | True after a resync timed out without a snapshot. |

### `add()` and resync

- `add()` returns `Busy` while `isResyncing()` is true. The local table is about
  to be replaced, so applying a delta now would be lost. Drop the input or queue
  it and retry after the flag clears.
- A send failure is reported but never rolled back: the local slot is updated
  before the send, so the local node never disagrees with itself.
- `value()` saturates at the `int32_t` limits instead of wrapping, so a
  saturated counter never flips sign.
- **Display clamping is the game's job.** The example clamps the drawn number to
  `[-999, 999]`; the accumulator stays `int32_t`.
- A node that boots after its peer enters resync on join. The better-established
  node sends a snapshot; the younger one waits, buffers incoming operations,
  replaces its table and replays them. If no snapshot arrives within 3 s, it
  resumes with `isOutOfSync()` raised and the value may have diverged.
- A power-cut reboot can be faster than the survivor's 3 s peer timeout. The
  younger node sends an explicit snapshot request so the survivor, which still
  thinks it is online, answers anyway.

### Convergence, not lockstep

Two peers may show different values for a few frames while an operation or a
snapshot is in flight. The module guarantees that the same applied operations
produce the same value, not that both screens change on the same frame. This is
the same scope correction recorded for physics: determinism is per build, not
across hardware or targets (issue #244, see
[physics-subsystem.md](../architecture/physics-subsystem.md)).

---

## 6. Native loopback

`LoopbackTransport` simulates the link inside one process, so the protocol,
discovery, retries and the counter can be exercised without hardware.

```cpp
#include <drivers/native/LoopbackTransport.h>

LoopbackBus bus;                       // owns the virtual clock and faults
NetAddress addressA{};
NetAddress addressB{};
addressA.bytes[5] = 1;
addressB.bytes[5] = 2;

LoopbackTransport transportA(bus, addressA);
LoopbackTransport transportB(bus, addressB);
NetSession sessionA(transportA);
NetSession sessionB(transportB);
NetCounter counterA;
NetCounter counterB;

// In the frame loop:
bus.setNowMs(nowMs);
```

What it simulates:

- Frames copied directly between attached transports on the same channel.
- A virtual clock advanced only by `bus.setNowMs()`.
- Faults through `LoopbackFaults`: `lossPercent`, `duplicatePercent`,
  `reorderPercent`, `delayMs`, `sendBusy` and a deterministic `seed`. The order
  per destination is loss, then duplication, then reordering.
- Up to 8 transports per bus.
- Broadcast to every other attached transport on the channel.
- Peer registration is a no-op; addresses are resolved against the bus.

What it does not simulate: RF range, interference, channel switching, real
timing jitter, or the Wi-Fi stack. It proves logic, not radio behavior.

**One Engine pumps one session.** The example registers the primary session
with the Engine and drives the second simulated node in `Scene::update()`
(counter update, session update, poll, counter route). That is a simulation
detail: two real boards each run their own Engine and pump their own session.

---

## 7. Troubleshooting

### No peer: the peer count stays 0

Check, in this order:

1. **Channel.** Both nodes must be on the same channel, 1..13. An AP
   association can move one of them.
2. **Discovery.** If `discovery = false`, every peer address must be seeded with
   `addPeer()` on both sides before traffic starts. Unknown HELLOs are dropped.
3. **MAC.** On native, the `NetAddress` passed to `LoopbackTransport` must be
   unique and must match the other side's seeded address. On ESP32, the MAC is
   the radio's station address; discovery should find it automatically.
4. **Peer table.** The default table holds 4 peers; a new HELLO beyond that is
   dropped and counted, and `addPeer()` returns `PeerTableFull`.
5. **Range.** A few metres indoors is typical for a C3 with a small antenna.

### Resync never finishes, or `isOutOfSync()` is true

The node is waiting for a snapshot and its peer is not answering. Confirm the
peer is alive and on the same channel. A 3 s timeout ends the wait and raises
`isOutOfSync()`; the value is usable but may have diverged. Restarting the one
node while the other stays up restores it. If both rebooted, the value resets to
0: the module has no persistence.

### `send()` returns `Busy`

- A reliable send returns `Busy` when all pending entries (8 by default) are
  awaiting an ACK. Wait for ACKs, reduce the burst, or raise
  `PIXELROOT32_NET_TX_QUEUE_CAPACITY`.
- An unreliable send returns `Busy` when the transport has no room. There is no
  queue behind it, so the frame is not sent.
- `counter.add()` returns `Busy` while resyncing, independently of the queue.

A full RX event queue drops the **newest** event and increments `rxDropped`; it
does not block the loop.

### Two nodes disagree about the role or the value

`nodeId` is an application label, not an identity. Two nodes with the same label
confuse application routing and the counter; the session does not detect the
collision. Assign unique ids (the example uses 1 and 2) and rebuild both nodes
when you change one.

### The Wi-Fi stack uses more RAM than expected

Enabling networking pulls in the Wi-Fi and ESP-NOW stacks. The module's own
static state is about 2 KB at defaults, but the stacks are much larger. On a
single-core C3, disable audio, particles, physics and the UI system if the game
does not need them, as `examples/esp_now` does. Its example-wide C3 figure is
42,492 bytes of RAM, including the Arduino core and the Wi-Fi stack, not just
this module. See the
[memory section](../architecture/networking-subsystem.md#8-memory).

---

## 8. Security note

v1 has **no encryption and no authentication**. Every peer is registered
unencrypted, and `NetPeerConfig::encrypted` is reserved and ignored. Anyone in
radio range can observe frames and, with discovery on and a free slot, can send
a HELLO and be added to the peer table.

- Do not use this link for secrets or credentials.
- For a locked deployment, set `discovery = false` and pre-seed the exact peer
  addresses on both sides. That removes the open join path but does not encrypt
  the traffic.
- LMK provisioning and encryption are planned for v2; the current encrypted-peer
  hardware ceiling (6) does not apply yet.

---

## 9. Limits

| Limit | Default | Range / ceiling |
|-------|---------|-----------------|
| Application payload | 64 B | Hard ceiling 234 B (250-byte frame minus 16-byte header) |
| Configured frame | 80 B | At most 250 B |
| Peer table | 4 | 1..20 (ESP-NOW hardware limit); 6 encrypted, unused in v1 |
| RX event queue | 8 | Power of two, >= 2 |
| Pending reliable messages | 8 | Power of two, >= 2 |
| ACK timeout | 20 ms | Configurable |
| Retries before `SendFailed` | 3 | At least 1 |
| HELLO interval | 1 s | Configurable |
| Peer timeout | 3 s | Configurable |
| Channel | 1 | 1..13, same on both nodes |
| Counter resync timeout | 3 s | Fixed by `kCounterResyncTimeoutMs` |
| Counter uptime tie window | 1 s | Fixed by `kCounterUptimeTieMs` |
| Resync operation buffer | 4 events | Fixed by `kCounterResyncBufferCapacity` |

---

## 10. Verification

Run the engine suites with the module compiled in, from the repository root:

```bash
pio test -e native_test_networking --verbose
```

This is the same command the CI workflow runs in its
`Run Tests (networking enabled)` step. `native_test` stays the flag-off
contract and does not compile the networking assertions.

Build the demo from `examples/esp_now`:

```bash
pio run -e native
pio run -e esp32c3_node_up
pio run -e esp32c3_node_down
```

The two C3 environments are the two roles: node 1 adds, node 2 subtracts.

## 11. Next steps

- [ESP-NOW example](../../examples/esp_now/README.md) - two C3 boards and a
  native simulation, with wiring, controls and measured build sizes.
- [Networking subsystem](../architecture/networking-subsystem.md) - wire format,
  reliability, discovery, threading, memory and limitations.
- [Networking API](../api/networking.md) - conceptual reference for
  `NetTransport`, `NetSession`, `NetCounter` and the event types.
- [Configuration](../api/config.md#networking-limits) - every
  `PIXELROOT32_NET_*` flag.
- [Testing](testing.md) - the `native_test_networking` environment and the
  `test_net_*` suites.
