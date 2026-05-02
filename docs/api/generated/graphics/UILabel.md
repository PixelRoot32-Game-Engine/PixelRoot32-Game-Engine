# UILabel

<Badge type="info" text="Class" />

**Source:** `UILabel.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

A simple text label UI element.

Displays a string of text on the screen. Auto-calculates its bounds based on text length and size.

## Inheritance

[UIElement](./UIElement.md) → `UILabel`

## Methods

### `void setVisible(bool v)`

**Description:**

Sets visibility.

**Parameters:**

- `v`: True to show, false to hide.

### `void centerX(int screenWidth)`

**Description:**

Centers the label horizontally on the screen.

**Parameters:**

- `screenWidth`: Width of the screen/container.

### `void recalcSize()`

**Description:**

Recalculates width and height based on current text and font size.
