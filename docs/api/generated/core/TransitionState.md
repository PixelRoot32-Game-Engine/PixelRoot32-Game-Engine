# TransitionState

<Badge type="info" text="Enum" />

**Source:** `SceneManager.h`

## Description

State machine for scene transitions.

Idle → FadingOut → SceneSwap → FadingIn → Idle.
During FadingOut/FadingIn the current scene's update() is skipped
(input blocking). Draw() still runs so the framebuffer has content
for the transition effect to post-process.
