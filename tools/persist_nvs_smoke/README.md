# persist_nvs_smoke — NVS adapter on the board (T6)

Board checks for `Esp32NvsKeyValueStore` / `Esp32NvsSlotStore`
(`include/drivers/esp32/Esp32NvsStore.h`) on the CYD (ESP32-2432S028, classic
ESP32, 4 MB), the board that carries DOOM. Two parts:

| Command | What it proves |
|---------|----------------|
| `pio test -e smoke` | The engine's shared adapter contract suite (`test/unit/test_persist_contract`), unchanged, against both NVS adapters (S14) |
| `pio run -e smoke -t upload` | `Preferences` compatibility (S13), type replacement on the real IDF, write cost, `nvs_get_stats` |
| `pio run -e smoke` and `pio run -e smoke_nostores` | Static RAM of the adapter (N3), from the two images' `.dram0.data` + `.dram0.bss` |

## Safety for the DOOM board

- **Partition table:** `partitions_wad.csv` is a copy of the DOOM demo's table.
  Flashing with the same table leaves the `nvs` partition and the 3.4 MB `wad`
  container where they are.
- **No write into `nvs` on upload:** the Arduino build script always flashes
  `boot_app0.bin` (8 KB of `otadata`) at `0xE000`. The DOOM table has no
  `otadata`, and `0xE000..0xF000` is the last page of `nvs`, so that image would
  erase an NVS page. `keep_nvs.py` drops it; the upload writes only
  `0x1000` (bootloader), `0x8000` (table) and `0x10000` (app).
- **Namespaces:** the firmware writes only to `pr32pref` and `pr32smk`, the
  contract suite only to `pr32ctk` and `pr32cts`, and each erases only its own
  namespaces (`nvs_erase_all` on its own handle). Nothing opens `doom`, erases
  the partition or erases flash. A namespace keeps its one NVS entry after
  `nvs_erase_all`; only a partition erase would remove it.
- **Afterwards** the board runs this firmware, not DOOM: reflash the DOOM app
  from the demo project with the environment you ship (for example
  `esp32dev_ILI9341`). The `wad` container is not touched and need not be
  reflashed. Note that the demo's own uploads still include `boot_app0.bin` at
  `0xE000`, the last page of `nvs`.

## Usage

```bash
pio device list                                   # the CYD is a CH340 (VID:PID 1A86:7523)
pio test -e smoke --upload-port COM4 --test-port COM4
pio run  -e smoke -t upload --upload-port COM4
pio device monitor -b 115200                      # reset the board to see the run
pio run  -e smoke_nostores                        # build only, for N3
```

`pio test` takes the suite from the engine's `test/` folder (`test_dir =
../../test`, `test_filter = unit/test_persist_contract`) and does not build
`src/main.cpp` (`test_build_src = no`). The contract file runs from `setup()`
on Arduino and from `main()` on the host.

## What it prints

```
# persist_nvs_smoke stores=1
# chip=ESP32-D0WD-V3 rev=3 mhz=240 idf=v4.4.7-dirty arduino=2.0.17
STATS before err=ESP_OK used=9 free=747 total=756 namespaces=5
SIZE Esp32NvsKeyValueStore=24 Esp32NvsSlotStore=28
PASS S13 Preferences.putBytes -> getBlob byte-identical
PASS S13 setBlob -> Preferences.getBytes byte-identical
IDF set_i32(t)=ESP_OK
IDF set_blob(t) on i32 key=ESP_OK
...
TIME setBlob  8 B us min=... mean=... max=... | commit us min=... mean=... max=... | n=50 failures=0
TIME setBlob 64 B us ...
STATS after err=ESP_OK used=9 free=747 total=756 namespaces=5
# done failures=0
```

- `S13`: 8 bytes (the size of DOOM's `cfg` record) written with
  `Preferences::putBytes` and read with `getBlob`, and 12 bytes the other way.
- `IDF`: raw `nvs_*` calls on one key under two types - the behaviour the
  adapter's one-value-per-key rule is built on.
- `TYPE`: the adapter's own last-write-wins check across types.
- `TIME`: `setBlob` and `commit()` separately, 50 writes of a changing blob per
  size. The set is where the flash write happens (NVS has no write-back
  cache); `commit()` is nearly free. Every write stalls flash-resident code on
  both cores for its duration.

## Results (2026-11-02, CYD, Arduino-ESP32 2.0.17 / IDF 4.4.7)

| Check | Result |
|-------|--------|
| Contract suite on the board | 67/67 (18 KV + 13 slot per adapter, Memory and NVS, + 5 memory hooks) |
| S13, `Preferences` -> `getBlob` | PASS, byte-identical, 8 B |
| S13, `setBlob` -> `Preferences` | PASS, byte-identical, 12 B |
| Type replacement (`setI32` then `setBlob`, `getI32`) | NotFound, blob intact |
| `setBlob` 8 B | min 1,657 us, mean 4,055 us, max 13,046 us |
| `setBlob` 64 B | min 3,053 us, mean 4,277 us, max 13,622 us |
| `commit()` | 6..21 us (mean 12 us) |
| `nvs_get_stats` before / after | used 9 / 9, free 747 / 747 of 756 entries |
| Static RAM (N3) | `.dram0.data` +0 B, `.dram0.bss` +48 B for two KV stores (24 B each); flash +2,760 B text, +680 B rodata |

Raw IDF 4.4.7 behaviour: after `nvs_set_i32(k)` then `nvs_set_blob(k)` both
succeed and both items stay; `nvs_get_i32(k)` returns the old integer,
`nvs_get_u32(k)` and the blob length query return `ESP_ERR_NVS_NOT_FOUND`
(not `TYPE_MISMATCH`), and `nvs_erase_key(k)` removes one item per call
(three calls for three types). An empty blob (`size 0`) is accepted, with a
null pointer too.
