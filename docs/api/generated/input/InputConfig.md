# InputConfig

<Badge type="info" text="Struct" />

**Source:** `InputConfig.h`

## Description

Configuration structure for the InputManager.

Defines the mapping between logical inputs and physical pins (ESP32) 
or keyboard keys (Native/SDL2).

Uses variadic arguments to allow flexible configuration of input count.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `buttonNames` | `std::vector<uint8_t>` | Array of button mappings (scancodes) for Native. |
| `inputPins` | `std::vector<int>` | Array of GPIO pin numbers for ESP32. |
| `count` | `int` | Total number of configured inputs. |
