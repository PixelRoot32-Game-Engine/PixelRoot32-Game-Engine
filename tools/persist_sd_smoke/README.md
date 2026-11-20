# persist_sd_smoke — microSD adapter on the board (T2)

Board checks for `SdSlotStore` (`include/drivers/esp32/SdStore.h`) on the CYD
(ESP32-2432S028R, classic ESP32, 4 MB). Three parts:

| Command | What it proves |
|---------|----------------|
| `pio test -e smoke` | The engine's shared adapter contract suite (`test/unit/test_persist_contract`), unchanged, against `SdSlotStore` (S3-S6) |
| `pio run -e smoke -t upload` | Mounts the card, prints `sizeof(SdSlotStore)` and the card size, runs one write/query/read/erase round trip, then the interactive hot swap |
| `pio test -e smoke --without-testing` | Builds the board test firmware only (no board attached) |

## Hardware

- **Card:** a FAT32-formatted microSD card in the CYD's own slot. Without a
  card the adapter's `init()` returns `Unavailable` and the report says so; the
  suite fails at its first mount assertion.
- **Pins:** the CYD microSD reader is on SPI/VSPI — CS=5, SCK=18, MISO=19,
  MOSI=23 — mounted at 4 MHz. The contract suite takes the same values from
  `PR32_SD_SMOKE_CS` / `PR32_SD_SMOKE_SCK` / `PR32_SD_SMOKE_MISO` /
  `PR32_SD_SMOKE_MOSI`, so another board can be smoked with
  `build_flags = ${env.build_flags} -DPR32_SD_SMOKE_CS=<n> ...`.
- **No partitioning:** unlike the NVS smoke tool this one keeps the default
  partition table and adds no upload scripts. Nothing is flashed outside the
  app image, and the card is never formatted or erased by the adapter.

## Where it writes

The store is rooted at `/pr32smk` with prefix `s`, so on the card it is
`/pr32smk/s/0.slot`, `1.slot`, `2.slot`. `fresh()` erases the three slots
before every contract test; the serial report erases slot 0 after its round
trip, and the hot swap seeds and erases slot 1. No other path on the card is
opened.

## Usage

```bash
pio device list                                               # the CYD is a CH340 (VID:PID 1A86:7523)
pio test -e smoke --upload-port COM4 --test-port COM4         # contract suite on the card
pio run  -e smoke -t upload --upload-port COM4                 # report + hot swap
pio device monitor -b 115200                                  # run it as soon as the upload ends
pio test -e smoke --without-testing                           # build only
```

`pio test` takes the suite from the engine's `test/` folder (`test_dir =
../../test`, `test_filter = unit/test_persist_contract`) and does not build
`src/main.cpp` (`test_build_src = no`). The contract file runs from `setup()`
on Arduino and from `main()` on the host.

## Hot swap (interactive)

The upload ends with a reset, so the firmware is already running when it
finishes: open `pio device monitor` straight away. The report prints without
help, then the hot swap asks for two steps over the same serial port. Booting
with the card already out is not a dead end either: it asks for a card (Enter)
and re-tries `init()` up to `kHotSwapAttempts` times before giving up.

1. It writes a known 64-byte record to slot 1 and prints
   `>>> Take the microSD card OUT of the CYD, then send Enter in this terminal.`
   Pull the card and press Enter in the monitor.
2. With the card out it checks that the call that touches it fails visibly
   (`NotFound`, or `IoError` for a damaged file), that `init()` answers
   `Unavailable` (the failed re-mount path) and that a `write()` on the
   unmounted store answers `Unavailable` too.
3. It prints `>>> Put the card BACK IN, then send Enter in this terminal.`
   Reinsert the card and press Enter.
4. `init()` mounts again (`HOTSWAP card back: init=Ok in N ms uptime=... ms`),
   the seeded record reads back byte for byte, and slot 1 erases. `uptime`
   keeps counting from the boot before the removal, so the report proves the
   recovery happened without a reboot.

Each prompt waits `kHotSwapTimeoutMs` (180 s) and repeats every 15 s, so a
monitor opened mid-wait still shows what to do. Pressing Enter before the card
is really out (or really back) is not a failure: the stage says which state it
still sees and asks again, up to `kHotSwapAttempts` (3) prompts. If nothing
arrives, or the card never changes state, the stage prints `SKIP hot swap: ...`
and the summary counts it in `skipped` instead of `failures`, so an unattended
flash never reports a false failure; everything before the prompt still ran.
The key must be sent by the terminal attached to the port (`pio device monitor`
or the PlatformIO monitor button) — a plain logger that cannot write bytes to
the port leaves the stage in `SKIP`.

## What it prints

```
# persist_sd_smoke
SIZE SdSlotStore=20
INIT root=/pr32smk prefix=s cs=5 sck=18 miso=19 mosi=23 hz=4000000 -> Ok
PASS init() Ok
INIT again=Ok
CARD cardSize=... total=... used=... free=... bytes
SLOT0 write=Ok query=Ok(64) read=Ok(64) erase=Ok query_after=NotFound(0)
PASS slot 0 write/query/read/erase round trip
HOTSWAP seed slot=1 write=Ok
PASS hot swap seed write
>>> Take the microSD card OUT of the CYD, then send Enter in this terminal.
HOTSWAP card out: query=NotFound(0)
PASS hot swap: with the card out an access fails visibly (NotFound/IoError)
HOTSWAP card out: init=Unavailable
PASS hot swap: init() without the card is Unavailable
HOTSWAP card out: write=Unavailable
PASS hot swap: an unmounted store answers Unavailable
>>> Put the card BACK IN, then send Enter in this terminal.
HOTSWAP card back: init=Ok in 231 ms uptime=74210 ms
PASS hot swap: init() mounts again without a reboot
HOTSWAP card back: query=Ok(64) read=Ok(64) same=yes
PASS hot swap: the record survived the reinsert
HOTSWAP card back: erase=Ok query_after=NotFound(0)
PASS hot swap: slots work again after the re-mount
# done failures=0 skipped=0
```

`sizeof(SdSlotStore)` is the object's static RAM: two string pointers, the
limits and a flag. The FATFS driver the Arduino `SD` library links costs
several KB more.

## Known limits

- **Replace on FAT is not atomic.** A write goes to `<file>.tmp`, is flushed
  (`fsync`) and closed, then the old file is removed and `SD.rename` moves the
  new one into place, because FAT cannot rename over an existing destination.
  Power lost inside that window can leave the old record, the new one, a torn
  file the `RecordCodec` envelope rejects, or — in the short gap between the
  removal and the rename — an empty slot.
- **Oversized or damaged files read as `IoError`,** never as an empty slot, so
  damage is visible.
- **Removing the card after `init()` is not detected up front.** The Arduino
  `SD` library caches the card type at mount, so the call that touches the card
  answers `IoError` (or `NotFound` for a missing file). Call `init()` again
  after reinserting: it probes the store directory, releases the dead volume
  and mounts the card again, without a reboot — the hot swap above is the
  hardware check for that path.
