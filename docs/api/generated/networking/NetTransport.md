# NetTransport

<Badge type="info" text="Class" />

**Source:** `NetTransport.h`

## Description

Abstract frame-level transport used by the networking session.

The transport moves complete frames between peers. It owns peer addresses,
the channel and the link state, and nothing else: it does not parse the
wire header, sequence messages, ACK, retry or touch game state.

Threading contract: every method may be called only from the main loop. A
real link backend may stage received frames from its own callback, but it
must only publish them through pollRx so the session drains them from
the main loop. pollRx and pollTxResult never block and never
allocate; they return false when nothing is pending.

## Methods

### `virtual NetResult init(const NetTransportConfig& config)`

**Description:**

Brings the link up and applies the transport configuration.

**Parameters:**

- `config`: Channel, node label and Wi-Fi ownership.

**Returns:** Ok on success, or a NetResult describing why the link is down.

Must be idempotent: a second call with the same configuration leaves an
already initialized transport usable.

### `virtual void shutdown()`

**Description:**

Takes the link down and releases its resources.

### `virtual NetResult addPeer(const NetPeerConfig& peer)`

**Description:**

Registers a peer address with the link layer.

**Parameters:**

- `peer`: Address and label of the peer.

**Returns:** Ok when the peer is known to the transport.

### `virtual NetResult removePeer(const NetAddress& address)`

**Description:**

Removes a previously registered peer.

**Parameters:**

- `address`: Peer MAC to forget.

**Returns:** Ok when the address is no longer registered.

### `virtual NetResult send(const NetAddress& to, const uint8_t* frame, uint16_t length)`

**Description:**

Sends one complete frame.

**Parameters:**

- `to`: Destination address, or the broadcast address.
- `frame`: Frame bytes, header included; may be null only when
       `length` is 0.
- `length`: Frame length in bytes, at most

**Returns:** Ok when the link accepted the send (its result arrives through

### `virtual bool pollRx(NetRxInfo& out)`

**Description:**

Pops one received frame, if any.

**Parameters:**

- `out`: Receives the source address, length and frame bytes.

**Returns:** true when a frame was written to `out`; false when none is due.

Non-blocking. The caller owns the copy for as long as it needs it.

### `virtual bool pollTxResult(NetTxResult& out)`

**Description:**

Pops one send result, if any.

**Parameters:**

- `out`: Receives the destination and whether delivery succeeded.

**Returns:** true when a result was written to `out`; false when none is due.

Non-blocking and strictly FIFO: results are reported in the order the
sends were accepted.

### `virtual NetAddress localAddress() const`

**Description:**

Returns this transport's own link-layer address.

**Returns:** Local MAC used as the source of every outgoing frame.

### `virtual NetResult setChannel(uint8_t channel)`

**Description:**

Changes the Wi-Fi channel used by the link.

**Parameters:**

- `channel`: Channel number, 1..13.

**Returns:** Ok on success, InvalidArgument for a channel outside 1..13.

Both peers must share a channel for traffic to be delivered.

### `virtual bool isReady() const`

**Description:**

Reports whether the link is initialized and usable.

**Returns:** true between a successful init() and shutdown().
