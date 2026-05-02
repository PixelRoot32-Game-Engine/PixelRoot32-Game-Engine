# Style guide

This page is a **short entry** for coding conventions. The full rules live in one place:

- **[Coding style](./coding-style.md)** — C++17 usage, namespaces, naming, hot-path rules, exceptions/RTTI policy

**Principles:** deterministic control flow, **no heap allocation in `update()` / `draw()`**, compile with `-fno-exceptions` and `-fno-rtti`, embedded-first (ESP32) with portable native builds.

For gameplay patterns (delta time, feel), see [Gameplay guidelines](./gameplay-guidelines.md). For navigation to all guides, see [Guide index](./index.md).
