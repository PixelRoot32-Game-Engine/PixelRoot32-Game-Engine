# ESP32EspNowTransport

<Badge type="info" text="Class" />

**Source:** `ESP32EspNowTransport.h`

**Inherits from:** [NetTransport](../networking/NetTransport.md)

## Description

ESP-NOW link backend for ESP32 targets.

Brings up a station interface without a router, initializes ESP-NOW on one
fixed channel and maps the raw ESP-NOW error codes onto NetResult.
Received frames and send results are staged by the two ESP-NOW callbacks into
fixed-capacity rings, so the Wi-Fi task never blocks the main loop and the
main loop never waits on the Wi-Fi task.

Single owner: ESP-NOW callbacks are process-global C function pointers, so
only one instance can own the radio at a time. A second instance receives
NetResult::Busy from init() until the owner shuts down. All
methods must be called from the main loop; the callbacks touch nothing but
their ring and a drop counter.

The rings are single-producer/single-consumer: the Wi-Fi task is the only
producer and the main loop is the only consumer. A full ring drops the newest
entry and counts it, matching the session's own backpressure policy, because
a late frame is worth less than a frame the peer is still waiting on.

v1 has no encryption: every peer is registered unencrypted and
NetPeerConfig::encrypted is ignored, so traffic is only as private as
the radio's coverage. Add an LMK path before trusting this on a shared band.

## Inheritance

[NetTransport](../networking/NetTransport.md) → `ESP32EspNowTransport`

## Properties

| Name | Type | Description |
|------|------|-------------|
| `kRxRingCapacity` | `static constexpr uint8_t` | Received frames staged by the callback before new ones are dropped. |
| `kTxResultRingCapacity` | `static constexpr uint8_t` | Send results staged by the callback before new ones are dropped. |

## Methods

### `NetResult init(const NetTransportConfig& config)`

**Description:**

Claims the radio, brings Wi-Fi up and starts ESP-NOW.

**Parameters:**

- `config`: Channel, node label and Wi-Fi ownership.

**Returns:** Ok when the link is usable; InvalidArgument for a channel outside
        1..13; Busy when another instance owns the radio; TransportError
        when a Wi-Fi, ESP-NOW or MAC step fails.

Idempotent for the same instance: the radio stays up and only a changed
channel is applied. A failed call unwinds everything it started, so a
retry begins from a clean state. When `manageWifi` is set the transport
brings the station up without joining an access point, because an
association would move the channel and drop the link.

### `void shutdown()`

**Description:**

Unregisters the callbacks, deinitializes ESP-NOW and clears rings.

### `NetResult addPeer(const NetPeerConfig& peer)`

**Description:**

Registers a peer address with ESP-NOW.

**Parameters:**

- `peer`: Address and label of the peer; the label is not sent here.

**Returns:** Ok when the peer is registered or already existed,
        NotInitialized when the link is down,
        PeerTableFull when the ESP-NOW peer table is exhausted,
        InvalidArgument for an unusable address, TransportError for any
        other failure.

The peer follows the interface channel (`channel = 0`), so a later

### `NetResult removePeer(const NetAddress& address)`

**Description:**

Removes a peer address from ESP-NOW.

**Parameters:**

- `address`: Peer MAC to forget.

**Returns:** Ok when the address is no longer registered, including when it
        never was; NotInitialized when the link is down.

### `NetResult send(const NetAddress& to, const uint8_t* frame, uint16_t length)`

**Description:**

Hands one complete frame to ESP-NOW.

**Parameters:**

- `to`: Destination address or the broadcast address.
- `frame`: Frame bytes, header included; may be null only when
       `length` is 0.
- `length`: Frame length in bytes, at most kNetTransportFrameSize.

**Returns:** Ok when the radio accepted the frame, NotInitialized,
        InvalidArgument for a null frame with bytes, PayloadTooLarge for
        a frame above the transport ceiling, Busy when the radio is out
        of buffers, or PeerNotFound when `to` was not registered.

Never blocks: the delivery outcome of an accepted frame arrives through

### `bool pollRx(NetRxInfo& out)`

**Description:**

Pops the oldest staged frame.

**Parameters:**

- `out`: Receives the source address, length and frame bytes.

**Returns:** true when a frame was copied, false when the ring is empty.

### `bool pollTxResult(NetTxResult& out)`

**Description:**

Pops the oldest staged send result.

**Parameters:**

- `out`: Receives the destination and whether ESP-NOW delivered it.

**Returns:** true when a result was copied, false when the ring is empty.

### `NetAddress localAddress() const`

**Description:**

Returns the station MAC read during init().

**Returns:** Local MAC, or an all-zero address before a successful init().

### `NetResult setChannel(uint8_t channel)`

**Description:**

Changes the Wi-Fi channel every peer follows.

**Parameters:**

- `channel`: Channel number, 1..13.

**Returns:** Ok on success, InvalidArgument outside 1..13, NotInitialized
        before init(), TransportError when the radio rejects the change.

The previous channel is kept when the change is rejected, so the value
reported by a later query still describes the live link.

### `bool isReady() const`

**Description:**

Reports whether this instance owns a usable link.

**Returns:** true between a successful init() and shutdown().
