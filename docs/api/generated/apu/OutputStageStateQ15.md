# OutputStageStateQ15

<Badge type="info" text="Struct" />

**Source:** `ApuCore.h`

## Description

Block-local snapshot of the integer output stage.

The stage is the only reader and writer of its own state, and it runs
on every sample, so the render loop copies it out of the class once
per block and writes it back once. Read through `this`, the ten
recursion words plus the coefficients, the trim and the three flags
cost twenty-two member loads and stores per sample for six multiplies
of arithmetic; held in a local, the compiler is free to keep the words
that stay live across iterations in registers.

It holds no new state and changes no arithmetic: every field is a
verbatim copy of a member below, and `captureOutputStageState` /
`restoreOutputStageState` are the only two places that move between
them. That mechanical copy is what makes the hoist bit-exact — a
member added to the stage must be added to this struct and to both
helpers, never handled inline at a call site.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `chainActive` | `bool` | `outputFilterActive_` |
| `lpfActive` | `bool` | `outputLpfActive_` |
| `legacyHpfEnabled` | `bool` | `hpfEnabled_` |
| `legacyRQ15` | `int32_t` | `hpfRQ15_`, the historical pole |
| `hpf1RQ15` | `int32_t` | `outHpf1RQ15_` |
| `hpf2RQ15` | `int32_t` | `outHpf2RQ15_` |
| `lpfB0Q15` | `int32_t` | `outLpfB0Q15_` |
| `lpfB1Q15` | `int32_t` | `outLpfB1Q15_` |
| `lpfB2Q15` | `int32_t` | `outLpfB2Q15_` |
| `lpfA1Q15` | `int32_t` | `outLpfA1Q15_` |
| `lpfA2Q15` | `int32_t` | `outLpfA2Q15_` |
| `trimQ15` | `int32_t` | `outTrimQ15_` |
| `legacyPrevInQ15` | `int32_t` | `hpfPrevInQ15` |
| `legacyPrevOutQ15` | `int32_t` | `hpfPrevOutQ15` |
| `hpf1PrevInQ15` | `int32_t` | `outHpf1PrevInQ15_` |
| `hpf1PrevOutQ15` | `int32_t` | `outHpf1PrevOutQ15_` |
| `hpf2PrevInQ15` | `int32_t` | `outHpf2PrevInQ15_` |
| `hpf2PrevOutQ15` | `int32_t` | `outHpf2PrevOutQ15_` |
| `lpfX1Q15` | `int32_t` | `outLpfX1Q15_` |
| `lpfX2Q15` | `int32_t` | `outLpfX2Q15_` |
| `lpfY1Q15` | `int32_t` | `outLpfY1Q15_` |
| `lpfY2Q15` | `int32_t` | `outLpfY2Q15_` |
