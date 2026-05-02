# LimitRect

<Badge type="info" text="Struct" />

**Source:** `PhysicsActor.h`

## Description

Defines a rectangular boundary for actor movement.

Used to constrain an actor within a specific area of the world.
Values of -1 indicate no limit on that side.

## Methods

### `int width() const`

**Description:**

Constructs a new LimitRect.

**Parameters:**

- `l`: Left limit.
- `t`: Top limit.
- `r`: Right limit.
- `b`: Bottom limit.

### `int height() const`
