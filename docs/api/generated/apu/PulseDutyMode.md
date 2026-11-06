# PulseDutyMode

<Badge type="info" text="Enum" />

**Source:** `AudioTypes.h`

## Description

Whether a PULSE note keeps the continuous duty generator (E3/A2).

`Nes` and `Famicom` snap a static duty to the nearest of the four NES
patterns (12.5 / 25 / 50 / 75 %). Rounding 30 % to 25 % is what the
hardware can express, but it makes a value the tool can already author
unsayable under a profile.

`Continuous` is the note winning inside the duty domain, exactly like an
animated duty (`dutySteps` / `dutySweep`) already does: both resolve to
the same `255` sentinel and neither pins the slot, so the next note on
that slot gets the profile's step again.

PULSE only. `Legacy` answers the continuous generator for every duty, so
`Continuous` is inert there.

Declared here for the same reason as `NoisePeriodMode`, and likewise not
stored on `AudioChannel`.
