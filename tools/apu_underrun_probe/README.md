# apu_underrun_probe — underrun gate (E0 / S15.6)

Starts a real `AudioEngine` with the `ESP32_I2S_AudioBackend`, holds **eight
voices** for `RUN_SECONDS` and reports the backend's own counters.

It is not a CPU benchmark: the render baseline comes from the APU's
`tools/apu_bench_firmware`, which renders into RAM. Here the samples go out through
I2S, so what is measured is whether the producer kept up.

## What it prints

```
# apu_underrun_probe rate=22050 seconds=600 block=512
# chip=ESP32-D0WD-V3 rev=3 mhz=240
# running
REPORT progress rate=22050 block=512 seconds=30 underruns=0 worst_block_us=23221 worst_write_us=17298
...
REPORT final    rate=22050 block=512 seconds=600 underruns=0 worst_block_us=... worst_write_us=...
```

- `underruns`: times the DMA ring was already empty when we came to write. The
  backend's rule is the one that fits this hardware: with a one-block ring a
  **healthy** write blocks for most of the block period, so the symptom of a starved
  consumer is that the write **barely waits** (`writeUs < budget/10`), not that the
  iteration took longer than the block.
- `worst_block_us`: the slowest whole iteration.
- `worst_write_us`: the longest the write waited for space. Close to the block
  period means the producer had plenty of margin; close to zero means the ring had
  drained.

## Voices

Eight, with the same recipe as the host benchmark: `PLAY_EVENT` can only own slots
4–7, so the other four come from `MUSIC_PLAY` with three sub-tracks. That recipe is
verified in `PixelRoot32-APU/tools/apu_bench` (a `UNIT_TEST` probe: 8 voices enabled
before and after 1 100 blocks).

## Usage

```bash
pio run -e smoke          -t upload --upload-port COM4   # 30 s sanity run
pio run -e esp32dev_22050 -t upload --upload-port COM4   # 10 min gate
pio run -e esp32dev_32000 -t upload --upload-port COM4
```

To capture, open the port and reset the board (a manual RTS pulse does not always
reset it; `esptool.py run --port COM4` does). Then:

```bash
pio device monitor -b 115200 > underrun.csv
```

## Pins

`BCLK 26`, `LRCK 25`, `DOUT 22` — the `esp32_dev_ST7789` mapping (MAX98357A). The
gate does not need a speaker: the peripheral consumes the ring at the configured
rate either way.

## What it does not measure

Audible latency or quality. It does not run on the C3 yet either: those envs are
added when that board is attached.
