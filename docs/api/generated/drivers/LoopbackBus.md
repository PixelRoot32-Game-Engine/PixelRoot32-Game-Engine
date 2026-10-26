# LoopbackBus

<Badge type="info" text="Class" />

**Source:** `LoopbackTransport.h`

## Description

In-process bus shared by a set of loopback transports.

The bus is the simulation's clock and its fault injector. It owns a small
fixed list of attached transports, the current `nowMs` value and one
xorshift32 generator, so a script replayed on a fresh bus produces the same
decisions byte for byte. It allocates nothing and has no static state: every
object on the bus is owned by the caller.

The bus must outlive every transport attached to it.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `kMaxTransports` | `static constexpr uint8_t` | Maximum number of transports that can share one bus. |
