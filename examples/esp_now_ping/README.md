# esp_now_ping

Two ESP32-C3 boards measuring an application round-trip over ESP-NOW.

One board sends a tagged **ping**, the peer replies with a tagged **pong**
automatically, and the sender displays the interval between its own timestamp
and the pong's arrival. `native` simulates both boards in one window, keys `1`
and `2` driving one panel each.

This is the minimal custom-message example: everything the
[networking guide](../../docs/guide/networking.md) shows in
[Sending your own messages](../../docs/guide/networking.md#sending-your-own-messages)
as a complete, compilable project.

## What it teaches

1. **A tagged application message.** `NetSession` moves opaque bytes; the first
   byte says what the rest means. This demo owns tags 16 and 17 and leaves the
   module's tags 1-3 alone.

2. **A pure codec.** [`PingPongProtocol.h`](src/PingPongProtocol.h) is a header
   with no Arduino, no display and no global state. It encodes and decodes the
   7-byte record, and the native unit tests include it directly.

3. **One reliable send and one automatic reply.** Both directions use
   `NetDelivery::Reliable`. The receiving board answers inside
   `Scene::onNetworkEvent`; there is no button press and no timer on the pong
   path.

4. **The Engine's event pipeline.** The scene registers its session with
   `engine.setNetworkSession()` and the Engine calls
   `Scene::onNetworkEvent()` for every message it drains. The scene never polls
   the primary session itself.

5. **An application RTT.** The ping carries the sender's `millis()` value; the
   pong echoes it; the sender subtracts. It measures the whole application
   round trip, not the radio's retry timer.

## Requirements (build flags)

| Flag | Value | Why |
| ---- | ----- | --- |
| `PIXELROOT32_ENABLE_NETWORKING` | `1` | Compiles the networking module. Off by default. |
| `PIXELROOT32_ENABLE_AUDIO` | `0` | Off - the demo is silent, and a single-core C3 pays for every task it does not need. |
| `PIXELROOT32_ENABLE_PARTICLES` | `0` | Off |
| `PIXELROOT32_ENABLE_PHYSICS` | `0` | Off |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | `0` | Off - `Renderer::drawText` works without it |
| `PIXELROOT32_USE_U8G2` | defined | Selects `U8G2_Drawer` for the 1-bit SSD1306 panel |
| `PHYSICAL_DISPLAY_WIDTH` / `HEIGHT` | `128` / `64` | The SSD1306 controller's framebuffer |
| `LOGICAL_WIDTH` / `HEIGHT` | `72` / `40` | The window the Beetle's panel actually shows |
| `X_OFF_SET` / `Y_OFF_SET` | `28` / `24` | Where that window sits inside the framebuffer |
| `ESP_NOW_NODE_ID` | `1` or `2` | Application label in HELLO |

The short flag list is the lesson: a monochrome panel has no palette, sprite
blit or tilemap fast path to justify turning any of them on.

`ESP_NOW_NODE_ID` is compile-time, not a runtime menu.
`src/platforms/esp32_c3.h` rejects any other value with an `#error`. Both
boards run identical code and both reply automatically, so the id only names
the sender; it does not decide who may ping.

## The 7-byte wire layout

Every ping and every pong is exactly `kPingPongPayloadSize` (7) bytes, little
endian:

| Byte | Field | Encoding |
| ---- | ----- | -------- |
| 0 | tag | `16` ping, `17` pong |
| 1-2 | sequence | `uint16`, little endian |
| 3-6 | stampMs | `uint32`, little endian |

The same `PingPongMessage` struct is used in both directions, so a pong can
echo the ping's sequence and stamp without a second schema. A ping and a pong
with the same fields differ only in byte 0.

**Tag registry.** `NetCounter` owns module tags 1, 2 and 3. This example starts
at 16 and uses 16 and 17; the dice example uses 10-12. A game should keep its
own tag enumerator in one place and never reuse a number.

Decoding is exact: a payload is rejected unless its length is exactly 7 and its
tag is the expected one. The null and length checks run before the first read,
so a short or foreign frame is never read past its declared bytes.

## Code walkthrough

### `sendPing()`

```cpp
// First online peer; pressing with nobody online does nothing.
if (session.peerInfo(id, info) && info.online) { chosen = id; }

++nextSequence;
if (nextSequence == 0) { ++nextSequence; }      // 0 is not a valid app id

PingPongMessage message{nextSequence, nowMs};
uint8_t payload[kPingPongPayloadSize] = {};
const uint16_t length = encodePing(payload, sizeof(payload), message);

if (session.send(chosen, NetDelivery::Reliable, payload, length) ==
    NetResult::Ok) {
    ++pingsSentCount;
} else {
    ++failureCount;
}
```

`Ok` means the session accepted the message, not that the radio delivered it.
A give-up after the retries arrives later as `SendFailed`.

### `onEvent()`

```cpp
case NetEventType::Message:
    if (event.payload[0] == kPingTag) {
        if (decodePing(event.payload, event.length, message)) {
            uint8_t reply[kPingPongPayloadSize] = {};
            const uint16_t length = encodePong(reply, sizeof(reply), message);
            session.send(event.peerId, NetDelivery::Reliable, reply, length);
            ++pongsSentCount;          // Ok only; otherwise ++failureCount
        }
    } else if (event.payload[0] == kPongTag) {
        if (decodePong(event.payload, event.length, message)) {
            lastRttValue = elapsedMs(millis(), message.stampMs);
            hasRttValue = true;
            ++pongsReceivedCount;
        }
    }
    break;

case NetEventType::SendFailed:
    ++failureCount;                    // a reliable send gave up
    break;
```

`PeerJoined` and `PeerLeft` need no protocol action: replies always address
`event.peerId` from the message that triggered them, and the panel reads
`peerCount()` live. Any other tag or length is ignored.

## Hardware

Two identical DFRobot Beetle ESP32-C3 boards, exactly as in
[mono_oled](../mono_oled/), [esp_now](../esp_now/) and
[esp_now_dice](../esp_now_dice/):

| Signal | Pin |
| ------ | --- |
| I2C SDA | 5 |
| I2C SCL | 6 |
| OLED reset | none (`255`) |
| Button | 3 |

The OLED is an SSD1306 wired as a 128x64 controller; the 0.42" glass exposes a
72x40 window at offset (28, 24). The engine draws in 72x40 coordinates and the
driver applies the offset.

## Roles and flashing

Both environments build the same `src/main.cpp`. Only `ESP_NOW_NODE_ID` differs.

| Role | Environment | Node id | Panel |
| ---- | ----------- | ------- | ----- |
| A | `esp32c3_ping_a` | 1 | `TX` / `RX` / `RTT` of board A |
| B | `esp32c3_ping_b` | 2 | same, independently |

Flash one board with each environment:

```bash
cd examples/esp_now_ping

# List the boards first; the two ports are usually different.
pio device list

pio run -e esp32c3_ping_a --target upload --upload-port COM5
pio run -e esp32c3_ping_b --target upload --upload-port COM6
```

On Linux/macOS, use the device paths instead of `COM5`/`COM6`
(`/dev/ttyUSB0`, `/dev/ttyACM0`, ...). If only one board is attached, omit
`--upload-port` and let PlatformIO pick it.

There is no wrong-role failure in this demo: either board can ping and either
board replies. Distinct ids are still the intended configuration, because the
label is what a larger game would route by.

## Controls

| Input | Native | ESP32-C3 | Action |
| ----- | ------ | -------- | ------ |
| Button 0 | `1` | GPIO 3 | Send one ping from this board |
| Button 1 | `2` | *(none)* | Send one ping from the simulated second board |

A press with no peer online does nothing. A press with a peer online sends one
reliable ping; the reply is automatic.

## The display

Each 72x40 panel has three text lines at font size 1:

- **Top** (`y = 0`): `TX n` at the left is the pings this board sent;
  `RX n` right-aligned is the pongs it received.
- **Middle** (`y = 12`): `RTT 12ms` after a measurement, `RTT --` before the
  first one.
- **Bottom** (`y = 31`): `PEER 1` when the peer is online, `OFFLINE` otherwise.

When a failure exists, the bottom line becomes `ERR n` **alone**. The obvious
`PEER 1 ERR 2` is 13 glyphs (78 px) and does not fit 72 px, so the failure
takes the line instead of being appended to the peer text; the peer count is
already implied by whether pings are being answered.

Counters are clamped to three digits (999) for drawing, and RTT to `9999`,
because a long soak would otherwise widen a line past the panel. The counters
themselves stay `uint32_t`. When nothing visible changes, the scene skips the
frame via `shouldRedrawFramebuffer()`, because a still U8G2 buffer looks exactly
like a hung program.

## Reading the RTT

The stamp travels verbatim. It is **not** a synchronized clock: the pong echoes
the number the ping carried, and the sender subtracts it from its own
`millis()`. Both values come from the same 32-bit millisecond clock, so the
result is a valid interval and stays correct across the 32-bit wrap
(`elapsedMs()` uses unsigned subtraction).

It measures the whole application round trip: ping encode and send, radio
delivery, the peer's decode and reply, and the pong's return. It includes the
peer's frame time between receipt and reply, so it is not a pure radio
measurement. A reliable retry on either leg lengthens it rather than losing the
sample.

## Limits

- **Reliable only, on purpose.** Both messages must arrive exactly once for the
  measurement to mean anything. A continuous heartbeat where the next sample
  supersedes the last would be `NetDelivery::Unreliable` instead.
- **One ping at a time.** The demo keeps one outstanding measurement per board
  and does not match a pong to a specific sequence; a stale pong would still be
  accepted. That is enough for a demo, not for a protocol.
- **No encryption.** v1 registers peers unencrypted. Anyone in radio range can
  observe or inject frames.
- **Channel 1, no router.** Both boards must use channel 1 and must not join an
  access point; an association can move the radio and drop the link.
- **No fragmentation.** One ESP-NOW frame is at most 250 bytes and the default
  application payload is 64 bytes. A ping uses 7.
- **First online peer only.** With more than two boards, a press targets the
  first online slot and the reply goes back to that sender.

## Build

```bash
cd examples/esp_now_ping

pio run -e native            # SDL2 simulator, two panels
pio run -e esp32c3_ping_a    # board A firmware
pio run -e esp32c3_ping_b    # board B firmware
```

Measured on clean builds. Both C3 environments report the same image size
because only one constant differs:

| Environment | RAM | Flash |
| ----------- | --- | ----- |
| `esp32c3_ping_a` | 42,172 bytes (12.9% of 327,680) | 747,816 bytes (57.1% of 1,310,720) |
| `esp32c3_ping_b` | 42,172 bytes (12.9% of 327,680) | 747,816 bytes (57.1% of 1,310,720) |
| `native` | host build, no size gate | host build, no size gate |

The codec is covered by
[`test/unit/test_esp_now_ping_protocol/`](../../test/unit/test_esp_now_ping_protocol/test_esp_now_ping_protocol.cpp):

```bash
pio test -e native_test -f "unit/test_esp_now_ping_protocol"
```

The `native` window opens two panels side by side (144x40 logical, 432x120
physical). One Engine pumps one session; `PingPongScene::update` pumps the
second simulated board itself and hands the loopback bus the same `millis()`
value the engine uses. That is a simulation detail: two real boards each run
their own Engine.

## Troubleshooting

### `OFFLINE` on both panels

The boards are not discovering each other. Check, in this order:

1. Both boards are powered and running this demo, not a different firmware.
2. Both use channel 1.
3. No router or AP association is configured on either board.
4. The boards are within radio range (a few metres indoors is typical).

`OFFLINE` should become `PEER 1` within about a second of the second board
booting.

### A press does nothing

`TX` does not move and no error appears. That is the documented no-peer path:
the board had no online peer at the moment of the press. Wait for `PEER 1`,
then press again.

### `TX` increases but `RX` stays at `0`

The ping left this board, but no pong came back. Either the peer never received
it (its panel would show `OFFLINE` or fail its own pings) or the return path is
failing. Confirm both boards are on channel 1 and within range.

### `ERR n` appears

At least one send was rejected (`Busy`, `PeerNotFound`, `Unsupported`) or a
reliable message exhausted its retries and arrived as `SendFailed`. A single
error during hand-over or a burst of presses is normal. A permanently growing
count means the link is not delivering; check the peer and the channel. The
bottom line shows the count alone because it does not fit beside `PEER`.

### The screen stops changing

Expected. The scene redraws only when a drawn counter, RTT, failure count or
peer count changes. An idle but healthy pair shows a still `RTT --` or the last
measurement forever, and that is not a hang.

### The two panels show different RTTs

Expected. Each board measures its own round trip, and the two boards do not
share a clock. Compare the order of magnitude, not the digits.

## Documentation links

- [`PingPongProtocol.h`](src/PingPongProtocol.h) - the wire contract: the two
  tags, the 7-byte layout and the wrap-safe interval.
- [`NetSession`](../../include/networking/NetSession.h) - discovery, reliability,
  peer lifecycle and the event queue.
- [`LoopbackTransport`](../../include/drivers/native/LoopbackTransport.h) - the
  native bus and its fault profiles.
- [`ESP32EspNowTransport`](../../include/drivers/esp32/ESP32EspNowTransport.h) -
  the radio backend.
- [`Engine`](../../include/core/Engine.h) - the networking pipeline and
  `setNetworkSession`.
- [Networking guide](../../docs/guide/networking.md) - setup, tags, delivery
  contracts, the ship checklist and the examples that go further.
- [Examples catalogue](../README.md)
