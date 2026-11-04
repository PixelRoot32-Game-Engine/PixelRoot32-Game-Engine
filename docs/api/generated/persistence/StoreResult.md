# StoreResult

<Badge type="info" text="Enum" />

**Source:** `StoreTypes.h`

## Description

Outcome of every persistence operation.

Every store, adapter and codec call reports one of these values: the module
never answers with a bare bool and never throws, so a caller can always tell
a missing record from a full medium or a damaged one.
