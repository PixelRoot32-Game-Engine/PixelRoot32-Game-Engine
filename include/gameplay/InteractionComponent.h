/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

namespace pixelroot32::core { class Actor; }

namespace pixelroot32::gameplay {

/**
 * @struct InteractionComponent
 * @brief Opt-in interaction hooks for a game actor — composition, not inheritance.
 *
 * D3's core decision: no new virtual methods and no new members on `Actor`
 * itself. A game actor that wants enter/exit/interact semantics holds one of
 * these as a member and registers it with an `InteractionTracker`:
 *
 * @code
 * tracker.registerActor(this, &interaction);   // after Scene::addEntity assigned entityId
 * @endcode
 *
 * Every actor that does NOT opt in pays exactly zero bytes for this
 * capability — `Actor` (include/core/Actor.h) is untouched by this file.
 *
 * `onInteract()` is manual-call only. The engine never auto-fires it from
 * enter/exit signals, physics contact resolution, or scene lifecycle events
 * — only explicit game code calling `invokeInteract()` triggers it, because
 * the engine has no facing-direction or action-binding concept to key an
 * automatic invocation on.
 */
struct InteractionComponent {
    using TriggerFn  = void (*)(void* owner, core::Actor* other);
    using InteractFn = void (*)(void* owner, core::Actor* instigator);

    void*      owner      = nullptr;   ///< Usually the game actor's `this`.
    TriggerFn  onEnter    = nullptr;   ///< Called once when a tracked pair begins contact.
    TriggerFn  onExit     = nullptr;   ///< Called once when a tracked pair's contact ends.
    InteractFn onInteract = nullptr;   ///< Manual-call only — never invoked by the engine.

    /**
     * @brief Invokes the interact callback. Manual-call only.
     *
     * The engine NEVER calls this. Game code calls it explicitly (e.g. after
     * a spatial-area-query finds a candidate and the player presses an
     * interact button).
     * @param instigator The actor initiating the interaction (e.g. the player).
     */
    void invokeInteract(core::Actor* instigator) {
        if (onInteract) onInteract(owner, instigator);
    }
};

}
