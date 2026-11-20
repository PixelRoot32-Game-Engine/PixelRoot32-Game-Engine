# SdConfig

<Badge type="info" text="Struct" />

**Source:** `SdConfig.h`

## Description

Hardware configuration for an SD card over SPI.

Plain data, passed to an SD adapter's init(): the adapter owns no pin
constants and no board assumptions, so the same adapter serves any board
that names its own bus and pins. Callers name the SPI peripheral and pins;
-1 means "use the bus default" for that signal.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `spi` | `SPIClass*` | SPI peripheral/bus to use (e.g. `&VSPI`, `&HSPI`, `&SPI`). |
| `sck` | `int` | SCK pin; -1 = bus default. |
| `miso` | `int` | MISO pin; -1 = bus default. |
| `mosi` | `int` | MOSI pin; -1 = bus default. |
| `cs` | `int` | Chip-select pin; required on most boards. |
| `frequencyHz` | `uint32_t` | SPI clock in Hz; conservative default of 4 MHz. |
