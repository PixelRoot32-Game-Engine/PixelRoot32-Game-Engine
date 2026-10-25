# NetPeerConfig

<Badge type="info" text="Struct" />

**Source:** `NetTransport.h`

## Description

One peer registration handed to the transport.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `address` | `NetAddress` | Peer MAC, the link-layer identity. |
| `nodeId` | `uint8_t` | Application label advertised by that peer. |
| `encrypted` | `bool` | Reserved for v2; the loopback ignores it. |
