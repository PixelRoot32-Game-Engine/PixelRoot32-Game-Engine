# esp_now

Two ESP32-C3 boards sharing one convergent counter over ESP-NOW.

This is the hardware demo for the optional `pixelroot32::networking` module: a
peer-to-peer link with no router, no IP address and no access point. One board
adds, the other subtracts, and both displays show the same value.

`native` simulates both boards in one window, so the protocol and the drawing
can be exercised without hardware.

## What it shows

1. **ESP-NOW is not IP.** Both boards bring the station interface up without
   joining an access point. Discovery is a broadcast HELLO; there is no DHCP, no
   socket and no routing. The link works because both radios sit on the same
   Wi-Fi channel.

2. **Reliable delivery with duplicate suppression.** A press is applied locally
   first and then replicated as one reliable operation. A retry or a duplicated
   frame is suppressed by the session's per-peer sequence window, so a press
   counts exactly once even when the radio is busy.

3. **A convergent counter, not lockstep.** The shared value is the sum of every
   node's own slot. Addition is commutative, so arrival order does not matter.
   The demo proves convergence, not frame-identical simulation: the two boards
   may draw a different value for a few frames while an operation is in flight.

4. **Reboot resync.** A node that boots after its peer does not restart at zero.
   On join, the better-established node sends a snapshot and the younger node
   waits for it before accepting input. The display shows `SYNC` during that
   window and `!` if the snapshot never arrived.

5. **The Engine's composition boundary.** The Engine borrows a `NetSession`; it
   does not own the transport or the session. This example shows the whole
   composition: bring the transport up, start the session, register the pointer,
   and forward drained events to the counter.

## Requirements (build flags)

| Flag | Value | Why |
| ---- | ----- | --- |
| `PIXELROOT32_ENABLE_NETWORKING` | `1` | Compiles the networking module. Off by default. |
| `PIXELROOT32_ENABLE_AUDIO` | `0` | Off - the demo has no sound, and a single-core C3 pays for every task it does not need. |
| `PIXELROOT32_ENABLE_PARTICLES` | `0` | Off |
| `PIXELROOT32_ENABLE_PHYSICS` | `0` | Off |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | `0` | Off - `Renderer::drawText` works without it |
| `PIXELROOT32_USE_U8G2` | defined | Selects `U8G2_Drawer` for the 1-bit SSD1306 panel |
| `PHYSICAL_DISPLAY_WIDTH` / `HEIGHT` | `128` / `64` | The SSD1306 controller's framebuffer |
| `LOGICAL_WIDTH` / `HEIGHT` | `72` / `40` | The window the Beetle's panel actually shows |
| `X_OFF_SET` / `Y_OFF_SET` | `28` / `24` | Where that window sits inside the framebuffer |
| `ESP_NOW_NODE_ID` | `1` or `2` | Application label; also the resync tie-breaker |
| `ESP_NOW_DELTA` | `1` or `-1` | What one local press adds: node 1 increments, node 2 decrements |

The short flag list is part of the lesson: this demo needs no audio, physics,
particles or UI, and a monochrome panel has no palette, sprite blit or tilemap
fast path to justify turning any of them on.

`ESP_NOW_NODE_ID` and `ESP_NOW_DELTA` are compile-time values, not a runtime
menu. `src/platforms/esp32_c3.h` rejects any node id other than 1 or 2, any
delta other than 1 or -1, and a mismatched pair, with an `#error`.

## Hardware

Two identical DFRobot Beetle ESP32-C3 boards, matching
[mono_oled](../mono_oled/):

| Signal | Pin |
| ------ | --- |
| I2C SDA | 5 |
| I2C SCL | 6 |
| OLED reset | none (`255`) |
| Button | 3 |

The OLED is an SSD1306 wired as a 128x64 controller; the 0.42" glass exposes a
72x40 window at offset (28, 24). The engine draws in 72x40 coordinates and the
driver applies the offset, exactly as in `mono_oled`.

## The two roles

Both environments build the same `src/main.cpp`. Only the two flags differ.

| Role | Environment | Node id | Delta | Panel |
| ---- | ----------- | ------- | ----- | ----- |
| Up | `esp32c3_node_up` | 1 | `+1` | `N1 +` |
| Down | `esp32c3_node_down` | 2 | `-1` | `N2 -` |

Flash one board with each environment:

```bash
cd examples/esp_now

# List the boards first; the two ports are usually different.
pio device list

pio run -e esp32c3_node_up   --target upload --upload-port COM5
pio run -e esp32c3_node_down --target upload --upload-port COM6
```

On Linux/macOS, use the device paths instead of `COM5`/`COM6`
(`/dev/ttyUSB0`, `/dev/ttyACM0`, ...). If only one board is attached, omit
`--upload-port` and let PlatformIO pick it.

A board flashed with the wrong environment still runs, but it will show the
wrong role and both boards will claim the same increment direction. Reflash it;
there is no runtime switch.

## Controls

| Input | Native | ESP32-C3 | Action |
| ----- | ------ | -------- | ------ |
| Button 0 | `1` | GPIO 3 | Apply the local role delta |
| Button 1 | `2` | *(none)* | Apply `-1` to the simulated second board |

On hardware, one press adds `ESP_NOW_DELTA` to the local node and replicates it
to every online peer. Both displays then show the converged value.

## The display

Each 72x40 panel shows:

- the local role (`N1 +` or `N2 -`, or `A +` / `B -` on native),
- the shared value, clamped to `[-999, 999]` for display only,
- the online peer count (`PEER 0` or `PEER 1`),
- a state marker in the top-right corner:
  - `SYNC` while the node is waiting for a snapshot,
  - `!` when a resync timed out and the value may have diverged,
  - otherwise the online peer count.

The internal accumulator is `int32_t` and saturates at its own limits; only the
drawn number is clamped. When nothing visible changes the scene skips the frame
via `shouldRedrawFramebuffer()`, because a still U8G2 buffer looks exactly like
a hung program.

## Channel requirement

Both boards must use Wi-Fi channel 1. The demo fixes it in
`EspNowDemoScene.cpp` (`kChannel = 1`) and never joins an access point.

- Do **not** connect either board to a router. Joining an AP can move the radio
  to the AP's channel and silently drop the link.
- Both boards must be rebuilt if you change `kChannel`; there is no runtime
  channel selector.
- A channel mismatch is the most common "the boards do not see each other"
  cause. The panels keep showing `PEER 0` and no value travels.

## Limitations

- **No encryption.** v1 registers peers unencrypted. Anyone in radio range can
  observe or inject frames. Do not use this for secrets. The module has a
  `discovery = false` + pre-seeded peer mode for locked deployments; this demo
  uses discovery so you can flash two boards and watch them find each other.
- **No persistence.** If both boards lose power, the shared value is lost and
  both boot at 0. A single reboot is restored by the live peer; a simultaneous
  cold boot converges trivially at 0.
- **No guaranteed ordering.** Operations are commutative, so order does not
  matter for this counter. A game that needs ordered events must carry its own
  logical clock.
- **No fragmentation.** One ESP-NOW frame is at most 250 bytes and the default
  application payload is 64 bytes. The counter uses 5 bytes per operation and
  46 bytes for a snapshot at the default peer limit.

## Native simulation

`native` builds one Engine, one scene, two `LoopbackTransport` instances on one
`LoopbackBus`, two sessions and two counters. The window is two 72x40 panels
side by side (144x40 logical, 432x120 physical).

- `1` applies `+1` to panel A.
- `2` applies `-1` to panel B.
- Both panels converge, exactly as the two boards do.

The loopback is **lossless**: it has no radio, no channel interference and no
retries to watch. It proves discovery, the counter and the drawing, not the RF
behaviour. Add a fault profile to `LoopbackBus` (loss, duplicate, reorder,
delay) if you want to exercise retries in native.

One Engine pumps and dispatches exactly one session. The scene registers the
primary session with the Engine and, in `Scene::update`, pumps the secondary
session itself (update, poll, counter). That is a **simulation detail, not an
engine limitation**: two real boards each run their own Engine and each Engine
pumps its own session.

There is no `SDL_Delay` in the scene. The engine loop owns timing; the scene
hands the loopback bus the same `millis()` value the engine uses.

## Counter and resync behaviour

The value is `sum(slot[i].value)`, where a slot is one node's accumulated
deltas. An operation carries `{nodeId, bootId, sequence, delta}` and is applied
once per sequence by the session's dedupe window.

- A press applies locally first, then replicates. A failing link never makes the
  local board disagree with itself.
- A rebooted node enters `SYNC` on join and rejects local input (`NetResult::Busy`)
  until the peer's snapshot lands. The survivor sends the snapshot because its
  uptime is higher; a simultaneous boot is broken by the lower node id.
- If no snapshot arrives within the peer timeout, the node exits `SYNC` with
  `!` raised. The value is usable, but it may have diverged. Restarting both
  boards is the simplest recovery.
- A snapshot replaces the whole slot table and replays operations that arrived
  during the wait, so an in-flight press is neither lost nor counted twice.

## Build

```bash
cd examples/esp_now

pio run -e native              # SDL2 simulator, two panels
pio run -e esp32c3_node_up     # node 1 firmware
pio run -e esp32c3_node_down   # node 2 firmware
```

Measured on clean builds (`esp32c3_node_up` and `esp32c3_node_down` report the
same image size because only two constants differ):

| Environment | RAM | Flash |
| ----------- | --- | ----- |
| `esp32c3_node_up` | 42,492 bytes (13.0% of 327,680) | 749,824 bytes (57.2% of 1,310,720) |
| `esp32c3_node_down` | 42,492 bytes (13.0% of 327,680) | 749,824 bytes (57.2% of 1,310,720) |
| `native` | host build, no size gate | host build, no size gate |

## Troubleshooting

### Both boards show the same role (`N1 +` twice, or `+` on both)

Both were flashed with the same environment. `ESP_NOW_NODE_ID` is compiled in,
so reflash one board with the other environment and confirm the header changes
before pressing anything.

### The panels show `PEER 0` and the value never moves

The boards are not discovering each other. Check, in this order:

1. Both boards are powered and running the demo, not a different firmware.
2. Both use channel 1. A board that joined an access point may have moved.
3. No router or AP association is configured on either board.
4. The boards are within radio range (a few metres indoors is typical).

`PEER` should become `1` within about a second of the second board booting.

### The panel shows `NO LINK`

Transport initialization failed: the radio did not come up, or another owner
already holds ESP-NOW. Check the serial monitor at 115200 baud, then power-cycle
the board. On a C3 this is usually a boot-time failure, not a protocol problem.

### `SYNC` never clears

The node is waiting for a snapshot and its peer is not answering. Confirm the
peer is alive and on the same channel. If the peer was power-cut so recently
that it never emitted `PeerLeft`, the rebooted node sends an explicit snapshot
request; if that is lost too, the wait ends in `!`.

### `!` appears

A resync timed out. The node kept its own value and resumed local input, but the
two boards may now disagree. Restart both, or restart one while the other stays
up so the survivor restores it.

### The two boards show different values for a moment

Expected. Convergence is not lockstep. The value becomes equal once the reliable
operation is delivered and acknowledged; a few frames of skew are normal.

## Hardware validation checklist (N14)

Run this on two physical Beetle ESP32-C3 boards and record the results in this
README and the architecture page.

- [ ] Cold boot: both panels show `0` within 2 s of link establishment.
- [ ] Node 1 pressed 10 times: both show `10`.
- [ ] Node 2 pressed 3 times: both show `7`.
- [ ] Node 1 pressed 100 times from zero: both show `+100` (`100`); no press is
      applied twice.
- [ ] Node 2 pressed 100 times: both move by `-100` from the same starting value.
- [ ] 100 rapid presses: counts match on both panels; no press is applied twice;
      note retries, suppressed duplicates and drops.
- [ ] Reboot node 1 while node 2 runs: node 1 restores `7` within 3 s and does
      not double-count its own previous contributions.
- [ ] Power off node 2 for 30 s: node 1 shows `PEER 0`; power node 2 on and
      confirm it rejoins and resyncs without drift.
- [ ] Walk out of range and return: the peer goes offline and comes back with no
      double count.
- [ ] Channel mismatch: force node 1 to channel 1 and node 2 to channel 6;
      confirm no link, a clear offline state and no phantom values.
- [ ] 10-minute soak at one press per second, alternating boards: final values
      are equal, free heap is stable within 1 KB and neither board reboots.
- [ ] Record press-to-remote-redraw latency for 20 presses; target p95 <= 150 ms
      and max <= 500 ms.

## Documentation links

- [`NetSession`](../../include/networking/NetSession.h) - discovery, reliability,
  peer lifecycle and the event queue.
- [`NetCounter`](../../include/networking/NetCounter.h) - operations, snapshots
  and resync.
- [`LoopbackTransport`](../../include/drivers/native/LoopbackTransport.h) - the
  native bus and its fault profiles.
- [`ESP32EspNowTransport`](../../include/drivers/esp32/ESP32EspNowTransport.h) -
  the radio backend.
- [`Engine`](../../include/core/Engine.h) - the networking pipeline and
  `setNetworkSession`.
- [Examples catalogue](../README.md)
