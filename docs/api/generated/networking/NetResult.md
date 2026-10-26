# NetResult

<Badge type="info" text="Enum" />

**Source:** `NetTypes.h`

## Description

Every fallible networking entry point returns one of these.

A dedicated enum (instead of a bool plus out-parameter) keeps transport,
session and application failures distinguishable without exceptions, which
this engine builds out.
