# Room

<Badge type="info" text="Struct" />

**Source:** `RoomGraph.h`

## Description

POD describing a single room: camera rect + optional tile window
       + fixed-size connection list.

A Room is always copyable and trivially constructible. All fields have
sensible zero/default initializers so a default-constructed Room is safe
to inspect (hasTileWindow == false, no connections).

Connections are stored by direction index (0 = Up, 1 = Down, 2 = Left,
      3 = Right). An unused slot holds INVALID_ROOM (0xFFFF).

::: tip
Connections are stored by direction index (0 = Up, 1 = Down, 2 = Left,
      3 = Right). An unused slot holds INVALID_ROOM (0xFFFF).
:::

## Properties

| Name | Type | Description |
|------|------|-------------|
| `cameraMinX` | `math::Scalar` | Left camera bound (world units) |
| `cameraMinY` | `math::Scalar` | Top camera bound (world units) |
| `cameraMaxX` | `math::Scalar` | Right camera bound (world units) |
| `cameraMaxY` | `math::Scalar` | Bottom camera bound (world units) |
| `tileOriginCol` | `int16_t` | Tile window origin column (-1 = unused) |
| `tileOriginRow` | `int16_t` | Tile window origin row    (-1 = unused) |
| `tileCols` | `int16_t` | Tile window width in tiles |
| `tileRows` | `int16_t` | Tile window height in tiles |
| `hasTileWindow` | `bool` | True if tile window fields are valid |
| `connections_` | `int` | Target room indices by direction (Up/Down/Left/Right) |
| `connectionCount_` | `uint8_t` | Number of valid connections |
| `constexpr` | `static` | Sentinel for unused connection slots |
