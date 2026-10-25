# API Reference: Networking Module

> **Source of truth:**
> - `include/networking/NetTypes.h`
> - `include/networking/NetProtocol.h`
> - `include/networking/NetTransport.h`
> - `include/networking/NetSession.h`
> - `include/networking/NetCounter.h`
> - `include/drivers/esp32/ESP32EspNowTransport.h`
> - `include/drivers/native/LoopbackTransport.h`

## Overview

The networking module is an optional peer-to-peer layer over ESP-NOW. It is
compiled only when `PIXELROOT32_ENABLE_NETWORKING=1` (default `0`) and lives in
`pixelroot32::networking`.

The public surface has three parts:

- `NetTransport` and its two implementations move complete frames.
- `NetSession` turns frames into peers, reliable delivery, dedupe and
  application events.
- `NetCounter` is the reference convergent state-sync helper.

`Engine` does not own any of them. It borrows a `NetSession*` through
`Engine::setNetworkSession()` and pumps it once per `update()`. See the
[Networking guide](../guide/networking.md) for composition and the
[Networking subsystem](../architecture/networking-subsystem.md) for the wire
format and guarantees.

> **Scope:** convergence, not lockstep. The same applied operations produce the
> same value on every peer regardless of order; frame-identical simulation is
> out of scope. See issue #244 and
> [physics-subsystem.md](../architecture/physics-subsystem.md).

## `NetTransport`

Abstract frame-level transport. Every method is called from the main loop;
`send()` and the two poll methods never block and never allocate.

| Method | Contract |
|--------|----------|
| `init(NetTransportConfig)` | Bring the link up. Idempotent. |
| `shutdown()` | Take the link down. Safe twice and after a failed init. |
| `addPeer(NetPeerConfig)` | Register a peer address with the link. |
| `removePeer(NetAddress)` | Forget a peer address. |
| `send(to, frame, length)` | Send one complete frame. `Ok` means accepted; the result arrives through `pollTxResult()`. |
| `pollRx(NetRxInfo&)` | Pop one complete received frame, if any. |
| `pollTxResult(NetTxResult&)` | Pop one send result, FIFO. |
| `localAddress()` | This transport's link address. |
| `setChannel(uint8_t)` | Change the channel, 1..13. |
| `isReady()` | True between a successful `init()` and `shutdown()`. |

`NetTransportConfig`:

| Field | Default | Meaning |
|-------|---------|---------|
| `channel` | `1` | Wi-Fi channel, 1..13 |
| `nodeId` | `0` | Application label; `0` derives it from the MAC |
| `manageWifi` | `true` | `false` leaves Wi-Fi ownership to the application |

`NetPeerConfig` carries the peer `address`, its `nodeId` label and a reserved
`encrypted` flag (ignored in v1).

`NetRxInfo` is one complete frame plus its source address. `NetTxResult`
reports the destination and whether the link delivered the frame.

### Backends

- `ESP32EspNowTransport` (`include/drivers/esp32/`) - real ESP-NOW, station
  without an access point, 4-entry atomic RX/TX staging rings, one owner per
  process. A second `init()` returns `Busy`.
- `LoopbackTransport` (`include/drivers/native/`) - in-process bus with a
  virtual clock and injectable loss, duplication, reordering, delay and
  send-busy. `LoopbackBus` owns the clock, faults and up to 8 transports.

## `NetSessionConfig` and `NetSession`

`NetSessionConfig` supplies the immutable session configuration. Defaults come
from `platforms::config`, so only the fields a game cares about need setting.

| Field | Default | Meaning |
|-------|---------|---------|
| `channel` | `1` | Shared channel, 1..13 |
| `nodeId` | `0` | Label; `0` derives it from the MAC |
| `bootId` | `0` | Per-boot generation; `0` generates one |
| `discovery` | `true` | `false` disables HELLO and requires seeded peers |
| `helloIntervalMs` | `1000` | HELLO period |
| `peerTimeoutMs` | `3000` | Silence before a peer is offline |
| `ackTimeoutMs` | `20` | Reliable ACK timeout |
| `maxRetries` | `3` | Retransmissions before `SendFailed` |

`NetSession` is a fixed-capacity object: peers, pending reliable frames and
events are all compile-time arrays. It never allocates, blocks or calls the
scene. All timers come from the `nowMs` argument of `update()`, never from an
internal clock.

| Method | Contract |
|--------|----------|
| `NetSession(NetTransport&)` | Binds to a transport that must outlive the session. The transport is not touched until `init()`. |
| `init(config)` | Requires a ready transport; applies the channel; validates intervals, timeouts and retries. Returns `NotInitialized`, `InvalidArgument`, or the transport's channel rejection. |
| `shutdown()` | Sends a best-effort BYE and clears peers and events. Leaves the transport running. |
| `update(nowMs)` | Sends HELLO on schedule, consumes TX results, drains at most 8 frames, retries and expires peers. No-op before `init()` or after `shutdown()`. |
| `send(peerId, delivery, data, length)` | Queues one application payload. See the return table below. |
| `addPeer(NetPeerConfig)` | Seeds a peer without discovery. Stores it offline; emits no event. |
| `pollEvent(NetEvent&)` | Pops the oldest session event. FIFO. |
| `peerInfo(peerId, NetPeerInfo&)` | Copies one peer slot snapshot. |
| `peerCount()` | Number of currently online peers. |
| `stats()` | Monotonic `NetStats` counters. |
| `pendingCount()` | Reliable messages queued or still awaiting an ACK. |
| `localNodeId()` / `localBootId()` | Effective identity after derivation. |

### `NetEvent` and `NetEventType`

| `NetEventType` | Meaning | Useful fields |
|----------------|---------|---------------|
| `Message` | Application payload from a peer | `peerId`, `nodeId`, `sequence`, `length`, `payload` |
| `PeerJoined` | A peer became reachable, including a revive after timeout | `peerId`, `nodeId` |
| `PeerLeft` | A peer timed out or sent BYE | `peerId`, `nodeId` |
| `SendFailed` | A reliable message exhausted its retries | `peerId`, `nodeId`, `sequence` |

`NetEvent` holds the payload inline at `platforms::config::NetMaxPayload`, so
delivering an event never allocates. At the default limits it is 76 bytes.

`NetPeerInfo` reports the peer address, label, boot generation, last-seen time,
advertised uptime and online state. Per-peer RTT is not tracked; only the most
recent measurement is in `NetStats::lastRttMs`.

## `NetCounter`

Convergent signed accumulator replicated as idempotent operations. The shared
value is the sum of every used slot, so the same applied operations produce the
same value independent of arrival order. Each operation is
`{nodeId, bootId, sequence, delta}` and is applied once per sequence by the
session's dedupe window.

| Method | Contract |
|--------|----------|
| `init(NetSession&)` | Binds the counter and rebuilds the local slot. Always `Ok`. |
| `shutdown()` | Clears slots and detaches. Safe twice. |
| `add(int16_t delta)` | Applies locally first, then sends one reliable operation to every online peer. Returns `NotInitialized`, `Busy` while resyncing, `Ok`, or the first send error. Never rolls back. |
| `onEvent(const NetEvent&)` | Routes a drained event into the counter. Handles operations, snapshots, joins and leaves. |
| `update(nowMs)` | Stores uptime, retries one deferred snapshot send, expires a resync. Call once per frame. |
| `isResyncing()` | True while waiting for a snapshot. |
| `isOutOfSync()` | True after a resync timeout until the next attempt or `init()`. |
| `value()` | Converged `int32_t`, saturated rather than wrapped. |
| `revision()` | Monotonic count of real value changes. |

`add()` returns `Busy` while resyncing. A snapshot replaces the whole slot table,
raises `revision()` once, ends resync and replays operations buffered during the
wait. If no snapshot arrives within `kCounterResyncTimeoutMs`, the node resumes
with `isOutOfSync()` raised. The game owns display clamping; the accumulator
stays `int32_t`.

## `NetResult`

Every fallible entry point returns one of these. The meaning of `Ok` is
operation-specific; in particular, a reliable `send()` returning `Ok` means
"accepted", not "delivered".

| Value | Typical meaning |
|-------|-----------------|
| `Ok` | Accepted / completed for this call. Read the method's contract. |
| `NotInitialized` | The session or transport is not initialized. |
| `InvalidArgument` | Null data with a non-zero length, an unused slot, or an out-of-range channel/timeout. |
| `PayloadTooLarge` | Payload or frame above its configured ceiling. |
| `PeerNotFound` | The peer slot exists but the peer is offline, or the address is unknown to the link. |
| `PeerTableFull` | The peer table has no free slot. |
| `NoMemory` | Reserved for a future backend. The current fixed tables report `Busy` or `PeerTableFull` instead. |
| `Busy` | The reliable pending queue is full, the transport has no room, or another ESP-NOW owner holds the radio. |
| `TransportError` | The link rejected the operation for a non-transient reason. |
| `Unsupported` | Reserved. No current transport returns it. |

## Key constants

| Constant | Header | Value | Meaning |
|----------|--------|-------|---------|
| `kNetAddressSize` | `NetTypes.h` | `6` | MAC length |
| `kNetMagic` | `NetProtocol.h` | `0x5052` | "PR" in reading order |
| `kNetHeaderSize` | `NetProtocol.h` | `16` | Wire header bytes |
| `kNetMaxFrameSize` | `NetProtocol.h` | `250` | ESP-NOW frame limit |
| `kNetMaxAppPayload` | `NetProtocol.h` | `234` | 250 minus the 16-byte header |
| `kNetTransportFrameSize` | `NetTransport.h` | `80` at defaults | Header plus configured payload |
| `kNetFlagReliable` | `NetProtocol.h` | `0x01` | Reliable flag bit; all other bits reserved |
| `kCounterOpTag` | `NetCounter.h` | `1` | Counter operation payload tag |
| `kCounterOpPayloadSize` | `NetCounter.h` | `5` | Tag plus `int32` delta |
| `kCounterSnapshotTag` | `NetCounter.h` | `2` | Counter snapshot payload tag |
| `kCounterSnapshotRequestTag` | `NetCounter.h` | `3` | Bare snapshot request tag |
| `kCounterSnapshotRequestSize` | `NetCounter.h` | `1` | Request payload size |
| `kCounterUptimeTieMs` | `NetCounter.h` | `1000` | Simultaneous-boot window |
| `kCounterResyncTimeoutMs` | `NetCounter.h` | `3000` | Snapshot wait before `outOfSync` |
| `kCounterResyncBufferCapacity` | `NetCounter.h` | `4` | Operations buffered during resync |

The counter's snapshot is an application Data payload with
`kCounterSnapshotTag`, not the reserved wire `NetPacketType::Snapshot`. Its
layout is `2 + 11 * slotCount` bytes: tag, slot count, then 11-byte entries of
`nodeId` (u8), `bootId` (u16), `value` (i32) and `cursor` (u32), all
little-endian.

## Public headers

| Header | Contents |
|--------|----------|
| `include/networking/NetTypes.h` | `NetAddress`, `NetResult`, `NetDelivery`, `NetEvent`, `NetEventType`, `NetPeerInfo`, `NetStats` |
| `include/networking/NetProtocol.h` | Wire constants, `NetPacketType`, `NetPacketHeader`, `NetParseResult`, encode/decode |
| `include/networking/NetTransport.h` | `NetTransport`, `NetTransportConfig`, `NetPeerConfig`, `NetRxInfo`, `NetTxResult`, frame constants |
| `include/networking/NetSession.h` | `NetSession`, `NetSessionConfig` |
| `include/networking/NetCounter.h` | `NetCounter` and the counter tags/limits |
| `include/drivers/esp32/ESP32EspNowTransport.h` | ESP-NOW backend (guarded by `ESP32`) |
| `include/drivers/native/LoopbackTransport.h` | Native bus and loopback backend (guarded by `PLATFORM_NATIVE`) |

The headers are the source of truth for signatures, defaults and exact
wording. The generated pages restate them:

- [NetSession](generated/networking/NetSession.md)
- [NetCounter](generated/networking/NetCounter.md)
- [NetTransport](generated/networking/NetTransport.md)
- [NetEvent](generated/networking/NetEvent.md)
- [NetResult](generated/networking/NetResult.md)
- [LoopbackTransport](generated/drivers/LoopbackTransport.md)
- [ESP32EspNowTransport](generated/drivers/ESP32EspNowTransport.md)

## Related documentation

- [Networking guide](../guide/networking.md) - setup, channel, reliability,
  counter and troubleshooting.
- [Networking subsystem](../architecture/networking-subsystem.md) - wire format,
  threading, memory and limitations.
- [Configuration](config.md#networking-limits) - `PIXELROOT32_NET_*` flags.
- [Platform compatibility](../guide/platform-compatibility.md) - target matrix.
- [API index](index.md) - all modules.
