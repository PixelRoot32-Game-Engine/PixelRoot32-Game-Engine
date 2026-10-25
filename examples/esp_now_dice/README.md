# esp_now_dice

Two ESP32-C3 boards playing a best-of-forever dice duel over ESP-NOW.

One board per player, one button per board, one shared score. Each round both
boards commit to a secret seed before seeing the other's, then reveal, derive
the same two dice and score the higher one. `native` simulates both boards in
one window, keys `1` and `2` driving one panel each.

> **Temporary prototype.** This example lives here for the 1.13.0 cycle and
> moves to
> [**PixelRoot32-Demo-Projects**](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects)
> afterwards. It is a complete two-player game built on top of the networking
> module, which is the one thing `examples/` is not supposed to keep.

## What it shows

1. **A commit/reveal exchange over a lossy radio.** A round is two reliable
   messages each way: a commitment to a secret seed, then the seed itself. The
   board that pressed second cannot pick its seed after seeing the first
   board's, because the first commitment is already on the wire.

2. **Deterministic derivation, not transmission.** Neither board sends its
   dice. Both compute them from the two revealed seeds and the round number, so
   the result cannot disagree with itself and the wire stays at 7 bytes per
   message. The derivation is in
   [`DiceProtocol.h`](src/DiceProtocol.h), a pure header shared with the native
   unit tests.

3. **A shared score with a sign convention.** The score lives in `NetCounter`
   and always holds `A - B`, where A is the lower node id. A's wins add `+1`,
   B's wins add `-1`. Both boards converge on the same number because the
   operation is replicated, so one round produces exactly one delta on the wire.

4. **One button, five phases.** Start a round, advance past a result, reset the
   match: the same press means whatever the current phase needs.

5. **The Engine's composition boundary again.** The Engine borrows a
   `NetSession`; it does not own the transport or the session. This example
   shows a second consumer of the same event stream: every event reaches both
   the counter and the duel.

## Rules

- A round starts when a player presses. The other player's commit may arrive
  first and is buffered, so **either** player can press first.
- Both boards exchange `Commit` (round + 32-bit commitment) and then `Reveal`
  (round + 32-bit seed) with `NetDelivery::Reliable`.
- Each board derives **two dice** from `(seedA, seedB, round)`, where A's seed
  is always the first input. A shows the first die, B the second, so the pair is
  identical on both panels with the columns swapped.
- **Higher die wins the round.** The winner moves the shared score by `+1` (A)
  or `-1` (B). A **tie scores nothing** and is replayed by pressing again.
- The match ends at a shared score of **+3 or -3**: three rounds ahead wins.
- A press on the match-over screen sends `Reset` and starts a rematch: round
  counter, peer cache and shared score all go back to zero on both boards.

A round that cannot finish is abandoned, never half-scored. Three seconds in
`Committing` or `Revealing` expires it as `TIMEOUT`; a conflicting commitment or
a seed that does not match its commitment is `CHEAT`; the peer leaving drops the
round back to `Idle`.

## Requirements (build flags)

| Flag | Value | Why |
| ---- | ----- | --- |
| `PIXELROOT32_ENABLE_NETWORKING` | `1` | Compiles the networking module. Off by default. |
| `PIXELROOT32_ENABLE_AUDIO` | `0` | Off - the duel is silent, and a single-core C3 pays for every task it does not need. |
| `PIXELROOT32_ENABLE_PARTICLES` | `0` | Off |
| `PIXELROOT32_ENABLE_PHYSICS` | `0` | Off |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | `0` | Off - `Renderer::drawText` works without it |
| `PIXELROOT32_USE_U8G2` | defined | Selects `U8G2_Drawer` for the 1-bit SSD1306 panel |
| `PHYSICAL_DISPLAY_WIDTH` / `HEIGHT` | `128` / `64` | The SSD1306 controller's framebuffer |
| `LOGICAL_WIDTH` / `HEIGHT` | `72` / `40` | The window the Beetle's panel actually shows |
| `X_OFF_SET` / `Y_OFF_SET` | `28` / `24` | Where that window sits inside the framebuffer |
| `ESP_NOW_NODE_ID` | `1` or `2` | Application label; `1` is A, `2` is B |

The short flag list is part of the lesson: the game needs no audio, physics,
particles or UI, and a monochrome panel has no palette, sprite blit or tilemap
fast path to justify turning any of them on. A whole die is at most six filled
rectangles and one outline.

`ESP_NOW_NODE_ID` is a compile-time value, not a runtime menu.
`src/platforms/esp32_c3.h` rejects any other value with an `#error`, because two
boards that both claim to be node 1 would have no canonical seed order and
therefore no way to agree on the dice.

## Hardware

Two identical DFRobot Beetle ESP32-C3 boards, exactly as in
[mono_oled](../mono_oled/) and [esp_now](../esp_now/):

| Signal | Pin |
| ------ | --- |
| I2C SDA | 5 |
| I2C SCL | 6 |
| OLED reset | none (`255`) |
| Button | 3 |

The OLED is an SSD1306 wired as a 128x64 controller; the 0.42" glass exposes a
72x40 window at offset (28, 24). The engine draws in 72x40 coordinates and the
driver applies the offset.

## The two roles

Both environments build the same `src/main.cpp`. Only `ESP_NOW_NODE_ID` differs.

| Role | Environment | Node id | Scores | Panel |
| ---- | ----------- | ------- | ------ | ----- |
| A | `esp32c3_node_a` | 1 | `+1` per round won | `YOU +n` when ahead |
| B | `esp32c3_node_b` | 2 | `-1` per round won | `YOU +n` when ahead |

A is the canonical first seed: both boards pass A's seed to the dice derivation
before B's, so the pair is the same on both sides. A is also the node that wins
the `NetCounter` uptime tie-break when both boards boot at the same time.

Flash one board with each environment:

```bash
cd examples/esp_now_dice

# List the boards first; the two ports are usually different.
pio device list

pio run -e esp32c3_node_a --target upload --upload-port COM5
pio run -e esp32c3_node_b --target upload --upload-port COM6
```

On Linux/macOS, use the device paths instead of `COM5`/`COM6`
(`/dev/ttyUSB0`, `/dev/ttyACM0`, ...). If only one board is attached, omit
`--upload-port` and let PlatformIO pick it.

A board flashed with the wrong environment still runs, but both boards will
claim the same role and the dice will not agree. Reflash it; there is no runtime
switch. The panel gives no direct hint about which role it holds - the score
wording is deliberately from the local perspective on both - so check the
environment you flashed, not the screen.

## Controls

| Input | Native | ESP32-C3 | Action |
| ----- | ------ | -------- | ------ |
| Button 0 | `1` | GPIO 3 | Press **this** board |
| Button 1 | `2` | *(none)* | Press the simulated **other** board |

One press means one of four things, depending on the phase:

| Phase | Press does |
| ----- | ---------- |
| `Idle` with a peer | Start a round (or join the peer's pending one) |
| `Idle` with no peer | Nothing at all - a lone board cannot score |
| `Committing` / `Revealing` | Nothing - the exchange is in flight |
| `Result` | Clear the result; a round that reached +3/-3 moves to match over |
| `MatchOver` | Send `Reset` to the peer and start a rematch |

## The display

Each 72x40 panel shows:

- **Top left**, the score from this board's point of view: `YOU +2` when you lead
  by two, `RIV +2` when the other board leads by two, `EVEN` at 0. The shared
  counter is always `A - B`, so board B negates it before showing it.
- **Top right**, the round number, `R3`.
- **Middle**, two 15x15 dice at `y = 11`: yours on the left, the peer's on the
  right. Before a round resolves both are empty boxes, so the panel shows where
  the dice will land instead of looking blank.
- **Bottom**, one status word:

| Word | Meaning |
| ---- | ------- |
| `PRESS` | Idle, peer online: a press starts a round |
| `OFFLINE` | Idle, no peer: nothing to play against |
| `WAIT` | Committing or revealing |
| `WIN` / `LOSE` / `TIE` | The round resolved; a press continues |
| `MATCH` | Someone reached +3/-3; a press starts a rematch |
| `CHEAT` | A commitment or reveal failed verification; the round was abandoned |
| `TIMEOUT` | The round ran past three seconds; the round was abandoned |
| `SYNC` | A score snapshot is being imported after a reboot |

Priority is `CHEAT` > `TIMEOUT` > `SYNC` > the phase word: a failed
verification is the one state the player has to act on, and both abort reasons
outrank the phase they are technically still in.

When nothing visible changes the scene skips the frame via
`shouldRedrawFramebuffer()`, because a still U8G2 buffer looks exactly like a
hung program. The scene compares phase, round, both dice, the score, the peer
count and every status flag - not the whole struct, whose padding bytes are not
initialised.

## Channel requirement

Both boards must use Wi-Fi channel 1. The demo fixes it in
`DiceDuelScene.cpp` (`kChannel = 1`) and never joins an access point.

- Do **not** connect either board to a router. Joining an AP can move the radio
  to the AP's channel and silently drop the link.
- Both boards must be rebuilt if you change `kChannel`; there is no runtime
  channel selector.
- A channel mismatch is the most common "the boards do not find each other"
  cause. Both panels keep showing `OFFLINE` and no round can start.

## Native simulation

`native` builds one Engine, one scene, two `LoopbackTransport` instances on one
`LoopbackBus`, two sessions, two counters and two duels. The window is two 72x40
panels side by side (144x40 logical, 432x120 physical).

- `1` presses panel A, `2` presses panel B.
- A round needs one press from each side, in either order.

One Engine pumps and dispatches exactly one session. The scene registers the
primary session with the Engine and, in `Scene::update`, pumps the secondary one
itself (session update, event drain, counter and duel). That is a **simulation
detail, not an engine limitation**: two real boards each run their own Engine
and each Engine pumps its own session.

There is no `SDL_Delay` in the scene. The engine loop owns timing; the scene
hands the loopback bus the same `millis()` value the engine uses. Both native
seed bases are fixed constants, so the same key sequence always produces the
same dice - which makes a bug reproducible and hides nothing behind randomness.

The loopback is **lossless**: no radio, no interference, no retries to watch. It
proves the protocol, the scoring and the drawing, not the RF behaviour. Native
also cannot show a timeout unless you stop pressing.

## Score and resync behaviour

The value is `sum(slot[i].value)`, where a slot is one node's accumulated
deltas. A round is only scored by the board that won it, so a round produces
exactly one replicated operation.

- A rebooted board enters `SYNC` on join and waits for the peer's snapshot
  before it can agree on the score.
- The survivor sends the snapshot because its uptime is higher; a simultaneous
  boot is broken by the lower node id. Either way both boards end up on the same
  score and the match continues from where it was.
- A round that resolves *inside* the `SYNC` window can lose its point:
  `NetCounter::add` answers `Busy` while a snapshot is pending, and the duel's
  scoring call is fire-and-forget, so the operation is dropped rather than
  queued. On a healthy link the snapshot lands within a few frames and the
  window is far shorter than a round, so this is visible only if the link is
  also slow.
- If no snapshot arrives within the peer timeout, the board exits `SYNC` and
  resumes with its own value, which may have diverged. The panel shows the phase
  word again; restarting both boards is the simplest recovery.
- If **both** boards lose power, the score is gone: the counter has no storage
  of its own. Both boot at 0, which converges trivially.

## Limitations

- **No encryption.** v1 registers peers unencrypted. Anyone in radio range can
  observe or inject frames.
- **The commitment is not cryptographic.** `makeCommit()` is a 32-bit mix of the
  seed and the round. It stops a peer from *changing its mind* after seeing your
  commitment, which is what the game needs; it does not stop a determined peer
  from searching the 32-bit seed space for a seed that yields a favourable
  commitment. Do not read it as a fair-play guarantee against a modified
  firmware, and do not reuse the pattern for anything that needs real
  commitments.
- **No persistence.** See the resync section: a single reboot is restored by the
  live peer, a simultaneous cold boot starts from 0.
- **Two players only.** A third board may join the radio, but the duel only ever
  talks to the first online peer it caches at the start of a round and ignores
  messages from every other peer.
- **No fragmentation.** One ESP-NOW frame is at most 250 bytes and the default
  application payload is 64 bytes. A duel message uses 7.

## Build

```bash
cd examples/esp_now_dice

pio run -e native            # SDL2 simulator, two panels
pio run -e esp32c3_node_a    # node A firmware
pio run -e esp32c3_node_b    # node B firmware
```

Measured on clean builds (`esp32c3_node_a` and `esp32c3_node_b` report the same
image size because only one constant differs):

| Environment | RAM | Flash |
| ----------- | --- | ----- |
| `esp32c3_node_a` | 42,572 bytes (13.0% of 327,680) | 752,270 bytes (57.4% of 1,310,720) |
| `esp32c3_node_b` | 42,572 bytes (13.0% of 327,680) | 752,270 bytes (57.4% of 1,310,720) |
| `native` | host build, no size gate | host build, no size gate |

## Troubleshooting

### Both panels show `OFFLINE` and nothing happens

The boards are not discovering each other. Check, in this order:

1. Both boards are powered and running this demo, not a different firmware.
2. Both use channel 1.
3. No router or AP association is configured on either board.
4. The boards are within radio range (a few metres indoors is typical).

`OFFLINE` should become `PRESS` within about a second of the second board
booting.

### One panel says `PRESS` and the other `OFFLINE`

Only one direction is arriving: the boards are on different channels, or one
radio failed to come up. `PRESS` is driven by the local peer table, so an
asymmetric result is a link problem, not a game problem.

### A press does nothing

Check the status word. `WAIT` means the exchange is in flight; `OFFLINE` means
there is no peer; `MATCH` means the match is over and the press should reset it.

### `CHEAT` appears

A commitment conflicted, or a revealed seed did not reproduce the commitment the
peer published. The round was abandoned without a score. This is a protocol
failure or tampered firmware - a clean link does not produce it. Press again; if
it repeats, one board is running different code.

### `TIMEOUT` appears

Three seconds passed in `Committing` or `Revealing`. Usually one board is gone or
one direction of the link is dead. Confirm `PRESS` on both panels, then press
again.

### `SYNC` never clears

The board is waiting for a score snapshot and its peer is not answering. Confirm
the peer is alive and on the same channel. If no snapshot arrives within the peer
timeout the wait ends on its own, and the score may then be wrong.

### The two panels show different numbers

The score is replicated, so a few frames of skew while an operation is in flight
are normal. If they stay different, one board missed the round: the score is a
sum of operations, not a snapshot per round, so press again and watch whether
the next round moves both. Restarting both boards is the reliable recovery.

### The dice are the same on both panels

Expected: both columns hold the same pair, and A's die is on the left while B's
die is on the right. If the two panels show the pair *swapped* (left die of one
equals right die of the other and vice versa) that is still correct - it is the
mirror image of the same pair.

## Hardware validation checklist

Run this on two physical Beetle ESP32-C3 boards and record the result here.

- [ ] Cold boot both boards: each shows `OFFLINE` then `PRESS` within 2 s.
- [ ] 10 rounds pressing A first: scores and dice agree on both panels every
      round; no round is scored twice.
- [ ] 10 rounds pressing B first: the pending commitment is adopted, no round
      times out, and the result matches the A-first runs.
- [ ] Force a tie and press again: `TIE` on both panels, neither score moves,
      the replay round resolves normally.
- [ ] Reach `MATCH` from +3: both panels show `MATCH` (the loser within one
      frame of the winner); one press resets both to `EVEN` and round `R0`.
- [ ] Reach `MATCH` from -3: same, with B ahead.
- [ ] Reboot the losing board mid-match: it shows `SYNC`, then restores the
      score and continues; no round is counted twice and the peer does not
      change its own score.
- [ ] Power the peer off mid-round: the remaining board drops out of `WAIT`
      without `TIMEOUT` and returns to `OFFLINE`.
- [ ] Power the peer off for 30 s and back on: it rejoins, resyncs and the next
      round resolves.
- [ ] Channel mismatch (one board rebuilt on channel 6): no link, both panels
      `OFFLINE`, no phantom scores.
- [ ] 30-minute soak with one press per side per round: the scores stay equal,
      free heap is stable within 1 KB and neither board reboots.

## Documentation links

- [`DiceProtocol.h`](src/DiceProtocol.h) - the wire contract: commitment,
  verification and the dice derivation.
- [`DiceDuel.h`](src/DiceDuel.h) - the round state machine, the payload tags and
  the timeout.
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
