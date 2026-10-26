# NetAddress

<Badge type="info" text="Struct" />

**Source:** `NetTypes.h`

## Description

Fixed-size peer address, trivially copyable and free of constructors.

A 6-byte MAC with no methods that own resources: the transport hands it
around by value and the peer table stores it inline, so the module never
needs a heap allocation to identify a peer.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `bytes` | `uint8_t` | Raw MAC bytes, in transmission order. |
