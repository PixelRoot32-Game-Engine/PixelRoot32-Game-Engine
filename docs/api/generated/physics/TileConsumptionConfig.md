# TileConsumptionConfig

<Badge type="info" text="Struct" />

**Source:** `TileConsumptionHelper.h`

## Description

Configuration for tile consumption operations.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `updateTilemap` | `bool` | Update tilemap runtimeMask to hide consumed tiles |
| `logConsumption` | `bool` | Log consumption events for debugging |
| `validateCoordinates` | `bool` | Validate tile coordinates before consumption |
| `requiredHits` | `uint8_t` | Number of hits required before the tile can be consumed. 1 = single-shot default; >1 requires multiple applyHit() calls. The engine does NOT store per-tile hit state — the caller owns the counter. |
