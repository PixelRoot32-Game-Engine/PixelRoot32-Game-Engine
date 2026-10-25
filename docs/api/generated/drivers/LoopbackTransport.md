# LoopbackTransport

<Badge type="info" text="Class" />

**Source:** `LoopbackTransport.h`

**Inherits from:** [NetTransport](../networking/NetTransport.md)

## Description

Native in-process transport with deterministic fault injection.

Frames are copied directly between attached transports on the same bus and
channel; there is no socket, thread or ESP-NOW dependency. Peer
registration is a documented no-op because an address is resolved against
the attached transports, which also means `addPeer` and `removePeer` always
return NetResult::Ok.

All time comes from the owning bus (LoopbackBus::nowMs), so delay and
fairness behaviour is fully controlled by the test. Received frames and send
results live in fixed arrays; no heap allocation happens after construction.

## Inheritance

[NetTransport](../networking/NetTransport.md) → `LoopbackTransport`

## Properties

| Name | Type | Description |
|------|------|-------------|
| `kRxQueueCapacity` | `static constexpr uint8_t` | Number of received frames buffered before send() reports Busy. |
| `kTxResultCapacity` | `static constexpr uint8_t` | Number of send results buffered before send() reports Busy. |

## Methods

### `NetResult init(const NetTransportConfig& config)`

**Description:**

Attaches to the bus, stores the configuration and marks ready.

**Parameters:**

- `config`: Channel, node label and Wi-Fi ownership.

**Returns:** Ok, InvalidArgument for a channel outside 1..13, or Busy when
        the bus already holds @ref LoopbackBus::kMaxTransports.

Idempotent: re-initializing an attached transport only updates the
stored configuration and channel.

### `void shutdown()`

**Description:**

Detaches, clears both queues and marks the transport not ready.

### `NetResult addPeer(const NetPeerConfig& peer)`

**Description:**

Accepts a peer registration without storing it.

**Parameters:**

- `peer`: Ignored: the loopback resolves addresses directly.

**Returns:** Always NetResult::Ok.

### `NetResult removePeer(const NetAddress& address)`

**Description:**

Accepts a peer removal without storing it.

**Parameters:**

- `address`: Ignored: the loopback resolves addresses directly.

**Returns:** Always NetResult::Ok.

### `NetResult send(const NetAddress& to, const uint8_t* frame, uint16_t length)`

**Description:**

Copies a complete frame to the destination's receive queue.

**Parameters:**

- `to`: Destination address or broadcast address.
- `frame`: Complete frame bytes; may be null only when length is 0.
- `length`: Frame length, at most kNetTransportFrameSize.

**Returns:** Ok, NotInitialized, InvalidArgument, PayloadTooLarge,
        PeerNotFound or Busy.

Broadcast reaches every other attached transport on the same channel.
A unicast send to an attached transport on another channel is accepted
but reported as an unsuccessful result and never delivered. One result
is queued per relevant destination; a dropout produces one unsuccessful
result and duplication still produces exactly one.

### `bool pollRx(NetRxInfo& out)`

**Description:**

Pops the oldest due frame.

**Parameters:**

- `out`: Receives source address, length and frame bytes.

**Returns:** true when a frame was popped; false when none is due.

Entries are scanned in insertion order, so among frames already due the
oldest one wins. A frame becomes due when `bus.nowMs()` reaches its
queued deadline; comparisons tolerate 32-bit wrap.

### `bool pollTxResult(NetTxResult& out)`

**Description:**

Pops the oldest send result.

**Parameters:**

- `out`: Receives the destination and delivery outcome.

**Returns:** true when a result was popped, false when the queue is empty.

### `NetAddress localAddress() const`

**Description:**

Returns the address passed to the constructor.

**Returns:** Local address; never changes after construction.

### `NetResult setChannel(uint8_t channel)`

**Description:**

Sets the channel used for delivery matching.

**Parameters:**

- `channel`: Channel number, 1..13.

**Returns:** Ok on success, InvalidArgument outside 1..13 (the previous
        channel is kept on rejection).

### `bool isReady() const`

**Description:**

Reports whether the transport is initialized.

**Returns:** true between a successful init() and shutdown()/bus reset().
