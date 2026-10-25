# PendingTx

<Badge type="info" text="Struct" />

**Source:** `NetSession.h`

## Description

One reliable frame waiting for its ACK.

The whole encoded frame is stored, not the application payload, so a
retry is byte-identical to the first transmission and never re-encodes
against a sequence that has already advanced. `sent` separates an entry
the transport has never accepted from one that is awaiting an ACK: an
unsent entry is attempted on every update and cannot consume the retry
budget, because it has nothing to retry yet.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `sequence` | `uint32_t` | Sequence carried by @ref frame. |
| `sentAtMs` | `uint32_t` | Session time of the last transmission. |
| `frameLength` | `uint16_t` | Valid bytes in @ref frame. |
| `retries` | `uint8_t` | Retransmissions already performed. |
| `peerId` | `uint8_t` | Destination slot index. |
| `sent` | `bool` | true once the transport accepted the frame. |
| `frame` | `uint8_t` | Encoded frame copy. |
