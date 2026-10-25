# NetTransportConfig

<Badge type="info" text="Struct" />

**Source:** `NetTransport.h`

## Description

Per-transport initialization values.

Mirrors the session configuration rather than owning it: the transport is
told which channel to use and which label its node advertises, and it does
not interpret either beyond link setup.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `channel` | `uint8_t` | Wi-Fi channel, 1..13 on real hardware. |
| `nodeId` | `uint8_t` | Application label; 0 means derive from the MAC. |
| `manageWifi` | `bool` | false leaves Wi-Fi ownership to the application. |
