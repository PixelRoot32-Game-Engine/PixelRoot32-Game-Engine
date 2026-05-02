# SPIClass

<Badge type="info" text="Class" />

**Source:** `MockSPI.h`

## Description

Mocks the Arduino SPI class for native platform.

Provides a dummy implementation of SPI methods to allow compilation
of drivers that depend on SPI.h.

## Methods

### `void begin()`

### `void end()`

### `uint8_t transfer(uint8_t data)`

**Description:**

Mocks data transfer. Returns input data (loopback).
