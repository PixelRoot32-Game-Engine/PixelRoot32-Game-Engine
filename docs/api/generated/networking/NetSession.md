# NetSession

<Badge type="info" text="Class" />

**Source:** `NetSession.h`

## Description

Fixed-capacity networking session: discovery, reliability and lifecycle.

The session turns the transport's raw frames into peer-table state and
application events. It owns discovery (HELLO), shutdown notification (BYE),
the peer table, the offline timeout, the unreliable data path, and reliable
delivery with its ACK, retry, duplicate window and give-up event. Reliable
sends survive a busy transport: an accepted message stays in the pending
queue and is transmitted on a later update without consuming a new
sequence. The sync layer is a separate helper (NetCounter) built on this
session; the session never interprets application payloads.

Ownership contract: the session never owns, initializes or shuts down the
transport. init requires a transport that is already ready, applies the
configured channel to it, and reports NetResult::NotInitialized
otherwise. shutdown sends a best-effort BYE and clears session state,
but deliberately leaves the transport running so the application stays in
control of the link lifetime.

Budget contract: every table and queue is a fixed array sized at compile
time. No method allocates, blocks or uses exceptions; construction reserves
the whole footprint.

Threading contract: every method must be called from the main loop. The
receive queue is deliberately not atomic, like GameplayEventBus, because the
transport publishes frames only through NetTransport::pollRx.

Time contract: the session has no clock of its own. All timers and
timestamps come from the `nowMs` argument of update, so a native test
can drive a whole peer lifecycle deterministically.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `kMaxPeers` | `static constexpr uint8_t` | Number of fixed peer slots, including offline ones. |

## Methods

### `explicit NetSession(NetTransport& transportRef)`

**Description:**

Binds the session to a transport.

**Parameters:**

- `transportRef`: Transport the session reads and writes frames on.

The reference must outlive the session. The transport is not touched
until

### `NetResult init(const NetSessionConfig& newConfig)`

**Description:**

Starts the session on an already initialized transport.

**Parameters:**

- `newConfig`: Channel, identity and timer configuration.

**Returns:** Ok on success, NotInitialized when the transport is not ready,
        InvalidArgument for a channel outside 1..13 or a zero interval,
        timeout or retry limit, or the transport's rejection of the
        channel.

Configuration is validated before the transport is touched, so a
rejected configuration leaves the link's channel and this session
uninitialized. Registers the broadcast address with the transport when
discovery is on. Peers registered through @ref addPeer survive a re-init,
because they are configuration rather than live link state; pending
events are cleared.

### `void shutdown()`

**Description:**

Sends a best-effort BYE and clears every peer and event.

### `void update(uint32_t nowMs)`

**Description:**

Advances discovery, drains the transport and expires silent peers.

**Parameters:**

- `nowMs`: Current time in milliseconds. May wrap; all comparisons are
       unsigned-difference comparisons.

No-op before

### `NetResult send(uint8_t peerId, NetDelivery delivery, const void* data, uint16_t length)`

**Description:**

Queues one application payload for a peer.

**Parameters:**

- `peerId`: Peer slot index, as reported by a PeerJoined event.
- `delivery`: Delivery contract requested for this message.
- `data`: Payload bytes; may be null only when `length` is 0.
- `length`: Payload size in bytes, at most NetMaxPayload.

**Returns:** Ok when the session accepted the message, NotInitialized before

### `NetResult addPeer(const NetPeerConfig& peer)`

**Description:**

Adds a peer address without waiting for discovery.

**Parameters:**

- `peer`: Address and application label to register.

**Returns:** InvalidArgument for the zero or broadcast address, Ok when the
        address was stored or already present, PeerTableFull when every
        slot is taken.

The slot is stored but marked offline, and no event is emitted: this
path pre-seeds a locked deployment or pairs with `discovery = false`.
The address is also registered with the transport.

### `bool pollEvent(NetEvent& out)`

**Description:**

Pops the oldest pending session event.

**Parameters:**

- `out`: Receives the event when one is queued.

**Returns:** true when an event was written, false when the queue is empty.

FIFO. PeerJoined and PeerLeft carry the peer slot and its application
label; Message events carry a copy of one Data payload, its slot, the
sender's label and the wire sequence.

### `bool peerInfo(uint8_t peerId, NetPeerInfo& out) const`

**Description:**

Copies one peer slot out of the table.

**Parameters:**

- `peerId`: Slot index, as reported by a PeerJoined or PeerLeft event.
- `out`: Receives the slot snapshot.

**Returns:** true when the slot exists and is in use; false for an
        out-of-range index or a never-used slot.

### `uint8_t peerCount() const`

**Description:**

Counts the peers currently considered online.

**Returns:** Number of slots marked online, at most NetMaxPeers.

### `const NetStats& stats() const`

**Description:**

Returns the session's traffic counters.

**Returns:** Reference to the live counters; only ever increases.

### `uint8_t pendingCount() const`

**Description:**

Counts the reliable messages still waiting for an ACK.

**Returns:** Number of occupied pending entries, at most NetTxQueueCapacity.

Diagnostic for tests and games: a full queue makes @ref send answer
Busy even when the transport itself would accept the frame.

### `uint8_t localNodeId() const`

**Description:**

Returns the effective local application label.

**Returns:** The configured nodeId, or the value derived from the local MAC.

### `uint16_t localBootId() const`

**Description:**

Returns the effective boot generation.

**Returns:** The configured bootId, or the generated per-boot value.
