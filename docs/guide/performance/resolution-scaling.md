# Resolution scaling

PixelRoot32 separates **logical** resolution (what your game draws at) from **physical** resolution (the actual display), so you can target low pixel counts for performance while filling modern panels.

## When to use it

- Ship gameplay at 128×128 or 160×144 but drive a 240×240 TFT.
- Keep UI and physics in logical space; only the final blit scales up.

## Configure `DisplayConfig`

Set `logicalWidth` / `logicalHeight` for the render buffer and `physicalWidth` / `physicalHeight` for the panel. The renderer and input pipeline map between the two according to your driver setup.

See [DisplayConfig / Engine](../../api/core.md#engine) and the architecture deep dive [Resolution scaling](../../architecture/resolution-scaling.md) for implementation details, ESP32 considerations, and coordinate mapping.

## Related

- [Rendering guide](../rendering.md)
- [Platform configuration](../platform-config.md)
- [ESP32 performance](./esp32-performance.md)
