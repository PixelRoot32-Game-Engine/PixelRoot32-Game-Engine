# NetSessionConfig

<Badge type="info" text="Struct" />

**Source:** `NetSession.h`

## Description

Values the application supplies once, when the session starts.

The defaults come from `platforms::config`, so an application that is happy
with the build-time limits only has to override the fields it cares about.
Two fields have sentinel semantics: `nodeId == 0` derives the label from the
local MAC, and `bootId == 0` asks the session to generate a per-boot value.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `channel` | `uint8_t` | Wi-Fi channel shared by every peer, 1..13. |
| `nodeId` | `uint8_t` | Application label; 0 derives it from the MAC. |
| `bootId` | `uint16_t` | Boot generation; 0 generates one per boot. |
| `discovery` | `bool` | false disables HELLO and requires seeded peers. |
| `helloIntervalMs` | `uint16_t` | HELLO period. |
| `peerTimeoutMs` | `uint16_t` | Offline threshold. |
| `ackTimeoutMs` | `uint16_t` | Reliable ACK timeout. |
| `maxRetries` | `uint8_t` | Retries before SendFailed. |
