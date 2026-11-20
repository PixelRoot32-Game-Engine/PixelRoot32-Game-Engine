# persist_sd_smoke — microSD adapter on the board (T2)

Board checks for `SdSlotStore` (`include/drivers/esp32/SdStore.h`) on the CYD
(ESP32-2432S028R, classic ESP32, 4 MB). Two parts:

| Command | What it proves |
|---------|----------------|
| `pio test -e smoke` | The engine's shared adapter contract suite (`test/unit/test_persist_contract`), unchanged, against `SdSlotStore` (S3-S6) |
| `pio run -e smoke -t upload` | Mounts the card, prints `sizeof(SdSlotStore)` and the card size, runs one write/query/read/erase round trip |
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
trip. No other path on the card is opened.

## Usage

```bash
pio device list                                               # the CYD is a CH340 (VID:PID 1A86:7523)
pio test -e smoke --upload-port COM4 --test-port COM4         # contract suite on the card
pio run  -e smoke -t upload --upload-port COM4                 # serial report
pio device monitor -b 115200                                  # reset the board to see the run
pio test -e smoke --without-testing                           # build only
```

`pio test` takes the suite from the engine's `test/` folder (`test_dir =
../../test`, `test_filter = unit/test_persist_contract`) and does not build
`src/main.cpp` (`test_build_src = no`). The contract file runs from `setup()`
on Arduino and from `main()` on the host.

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
# done failures=0
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
- **Removing the card after `init()`** is not detected on every operation; a
  failed access answers `IoError`/`NotFound`, and `init()` must run again after
  reinserting.
