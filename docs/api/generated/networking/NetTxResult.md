# NetTxResult

<Badge type="info" text="Struct" />

**Source:** `NetTransport.h`

## Description

Outcome of one accepted send, reported asynchronously.

One result is queued per delivered destination. `success` is false when the
link (or a fault injector) accepted the send but did not deliver it.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `to` | `NetAddress` | Destination that this result refers to. |
| `success` | `bool` | true when the frame was handed to the link. |
