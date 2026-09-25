# DialogEvent

<Badge type="info" text="Struct" />

**Source:** `DialogTypes.h`

## Description

The single payload type delivered to DialogEventFn for every kind of dialog event.

One shared shape for all four DialogEventType values, rather than a
union or a type per event, keeps the runner's synchronous callback
interface a single function pointer with no std::function and no
heap-allocated event objects.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `type` | `DialogEventType` | Which event this is; decides which fields below apply. |
| `line` | `LineId` | Line the event concerns; kNoLine on Ended after a bad id. |
| `choice` | `ChoiceId` | kNoChoice except on ChoiceConfirmed. |
| `tag` | `uint16_t` | Line tag, or choice tag on ChoiceConfirmed. Never interpreted. |
