# Engine Philosophy

PixelRoot32 is not the product of a traditional electronics expert or a large engineering team.

It was born from curiosity, experimentation, and a deep love for retro games from the 90s.

The journey started with a simple Raspberry Pi Zero project (PiGRRL Zero), exploring how far limited hardware could go. From there, the challenge evolved: rendering small games, experimenting with engines like Godot under constraints, and eventually discovering the ESP32 as a surprisingly capable platform for game development.

Coming from a mobile development background with limited experience in C++, this project represents a leap into a different domain. That leap was made possible by how accessible knowledge has become today—especially through AI-assisted tools that lower the barrier to learning and building complex systems. PixelRoot32, in many ways, reflects that shift: the ability to explore new fields driven by curiosity rather than formal specialization.

PixelRoot32 exists as a personal exploration of what is possible when passion meets constraints.

---

## Core Principles

* **Constraints drive design**Limited hardware is not a limitation, but a creative constraint that shapes better, simpler systems.
* **Determinism over convenience**The engine favors predictable behavior over abstractions that hide complexity.
* **No hidden costs**No runtime allocations, no implicit work — everything is explicit and under control.
* **Simplicity over complexity**Systems are designed to be understandable, hackable, and transparent.
* **Learning by building**PixelRoot32 is as much a learning journey as it is a game engine.
* **Retro-inspired, not retro-limited**Inspired by 90s games, but built with modern tools and ideas.

---

## Philosophy in Practice

This philosophy translates into concrete decisions:

* No exceptions or RTTI
* Fixed memory strategies and object pooling
* Explicit ownership (`std::unique_ptr`)
* Integer-friendly rendering pipelines
* Compile-time modular systems
* Minimal dependencies
  
---

## Acknowledgements

PixelRoot32 was heavily inspired by the work of nbourre and the ESP32 Game Engine project:[GitHub - nbourre/ESP32-Game-Engine · GitHub](https://github.com/nbourre/ESP32-Game-Engine)

This project builds upon that inspiration with a focus on structure, determinism, and long-term maintainability.
