# SoundProfile

<Badge type="info" text="Enum" />

**Source:** `AudioTypes.h`

## Description

Named character sets applied to every voice.

`Legacy` is the frozen compatibility contract: it reproduces the 2.0.0
sound. `Nes` and `Famicom` select profile-owned character (linear
envelope, weighted mix, unipolar noise, stepped duty) and differ only in
the output stage: `Nes` runs HPF 90 Hz -> HPF 440 Hz -> LPF 14 kHz (the
front-loading NES), `Famicom` HPF 37 Hz -> LPF 14 kHz (the Famicom's
single coupling high-pass). The profile table in `docs/ABI.md` says what
each one owns.

`Hybrid` is the 2.2.x name of value 2. It is kept as a deprecated alias
of `Famicom` so existing code keeps compiling (with a warning); the wire
value stays 2, so a serialised `Hybrid` selects `Famicom`.
