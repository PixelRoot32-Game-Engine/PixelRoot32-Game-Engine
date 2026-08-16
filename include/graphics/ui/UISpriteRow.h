/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "UIElement.h"
#include "UISpriteRef.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UISpriteRow
 * @brief A UI leaf that draws a row of repeated icons whose fill is driven by
 *        one value — hearts, lives, keys, ammo.
 *
 * Inherits from UIElement.
 *
 * **One element draws every icon.** The obvious alternative — a UILayout
 * holding N UISprite children — costs one scene entity per icon, and a row of
 * 16 hearts would take two thirds of the 24-entity budget recommended for the
 * ESP32-C3. Worse, every one of those entities re-enters `Scene::sortEntities()`
 * (an insertion sort that runs each frame once depth sorting is on) to produce
 * an order that never changes. A single element sidesteps both.
 *
 * The engine does not know what a heart is. It knows a `value`, a `capacity`,
 * and how many units fill one icon:
 *
 * - `unitsPerIcon == 1` gives binary icons: a lives row, a key count.
 * - `unitsPerIcon == 2` gives half-steps: value 5 over 3 icons reads
 *   full, full, half.
 * - Higher values give finer partial icons (quarter hearts at 4).
 *
 * State sprites are indexed by fill level: index 0 is empty, index
 * `unitsPerIcon` is full, and the values between are the partial steps. They
 * need not share a sprite format.
 *
 * `setCapacity()` may grow at runtime — a heart container picked up mid-game
 * widens the element, and any layout holding it sees the new preferred size.
 * `setIconsPerRow()` wraps onto further rows, which is how a heart bar longer
 * than the screen stays on screen.
 *
 * @code
 * UISpriteRow hearts(Vector2(8, 8));
 * hearts.setStateSprite(0, kHeartEmpty);
 * hearts.setStateSprite(1, kHeartHalf);
 * hearts.setStateSprite(2, kHeartFull);
 * hearts.setUnitsPerIcon(2);      // half-heart granularity
 * hearts.setIconsPerRow(8);       // wrap like the NES original
 * hearts.setCapacity(3);          // three containers to start
 * hearts.setValue(6);             // all full
 * hearts.setFixedPosition(true);  // immune to camera scroll
 * scene.addEntity(&hearts);
 * @endcode
 */
class UISpriteRow : public UIElement {
public:
    /**
     * @brief Number of fill states an icon can have, empty through full.
     *
     * Bounds `unitsPerIcon` to kMaxStates - 1, which allows quarter-steps.
     * Fixed-size storage: no heap, no per-icon allocation.
     */
    static constexpr int kMaxStates = 5;

    /**
     * @brief Constructs an empty row at `position`.
     *
     * Starts with no states and zero capacity, so drawing is a no-op until
     * both are configured.
     */
    explicit UISpriteRow(pixelroot32::math::Vector2 position);

    /**
     * @brief Constructs an empty row at (x, y).
     */
    UISpriteRow(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y);

    virtual ~UISpriteRow() = default;

    /**
     * @brief Assigns the 1bpp sprite drawn at fill level `stateIndex`.
     * @param stateIndex 0 = empty .. unitsPerIcon = full. Out-of-range
     *        indices are ignored rather than clamped, so a caller's off-by-one
     *        cannot silently overwrite a neighbouring state.
     * @param sprite Sprite to reference. Must outlive this element.
     * @param tint Color the set pixels are drawn in.
     */
    void setStateSprite(int stateIndex, const Sprite& sprite, Color tint = Color::White);

    /**
     * @brief Assigns the 2bpp sprite drawn at fill level `stateIndex`.
     * @param stateIndex 0 = empty .. unitsPerIcon = full. Out-of-range ignored.
     * @param sprite Sprite to reference. Must outlive this element.
     * @param paletteSlot Sprite palette slot to resolve colors through.
     */
    void setStateSprite(int stateIndex, const Sprite2bpp& sprite, uint8_t paletteSlot = 0);

    /**
     * @brief Assigns the 4bpp sprite drawn at fill level `stateIndex`.
     * @param stateIndex 0 = empty .. unitsPerIcon = full. Out-of-range ignored.
     * @param sprite Sprite to reference. Must outlive this element.
     * @param paletteSlot Sprite palette slot to resolve colors through.
     */
    void setStateSprite(int stateIndex, const Sprite4bpp& sprite, uint8_t paletteSlot = 0);

    /**
     * @brief Sets how many units fill a single icon.
     * @param units Clamped to [1, kMaxStates - 1].
     */
    void setUnitsPerIcon(uint8_t units);

    /// Units required to fill one icon.
    uint8_t getUnitsPerIcon() const { return unitsPerIcon; }

    /**
     * @brief Sets how many icons are drawn.
     *
     * May grow or shrink at runtime; the element's width/height follow.
     */
    void setCapacity(uint8_t iconCount);

    /// Number of icons drawn.
    uint8_t getCapacity() const { return capacity; }

    /**
     * @brief Sets the filled amount, in units.
     *
     * Values below zero or above `capacity * unitsPerIcon` are not errors:
     * they render as a fully empty or fully full row. Games routinely
     * overshoot on the frame damage lands.
     */
    void setValue(int filledUnits) { value = filledUnits; }

    /// Filled amount, in units, as last set (unclamped).
    int getValue() const { return value; }

    /// Sets the horizontal gap between icons, in pixels.
    void setSpacing(int pixels);

    /// Horizontal gap between icons, in pixels.
    int getSpacing() const { return spacing; }

    /// Sets the vertical gap between wrapped rows, in pixels.
    void setRowSpacing(int pixels);

    /// Vertical gap between wrapped rows, in pixels.
    int getRowSpacing() const { return rowSpacing; }

    /**
     * @brief Sets how many icons fit on a row before wrapping.
     * @param iconsPerRow 0 disables wrapping (a single unbounded row).
     */
    void setIconsPerRow(uint8_t iconsPerRow);

    /// Icons per row before wrapping, or 0 when wrapping is disabled.
    uint8_t getIconsPerRow() const { return iconsPerRow; }

    /**
     * @brief Fill state of the icon at `iconIndex`.
     * @param iconIndex Icon position, 0-based.
     * @return 0 (empty) .. unitsPerIcon (full). Returns 0 for an index outside
     *         [0, capacity).
     *
     * Exposed because a game often needs the same answer the row draws with —
     * to flash the icon that just changed, or park a cursor on it.
     */
    int stateIndexAt(int iconIndex) const;

    /**
     * @brief Offset of the icon at `iconIndex` relative to this element's
     *        position, accounting for spacing and wrapping.
     * @param iconIndex Icon position, 0-based.
     * @param outOffsetX Receives the horizontal offset in pixels.
     * @param outOffsetY Receives the vertical offset in pixels.
     *
     * Both outputs are set to 0 for an index outside [0, capacity).
     */
    void iconOffsetAt(int iconIndex, int& outOffsetX, int& outOffsetY) const;

    /**
     * @brief No-op. The row has no internal clock; it renders whatever
     *        setValue() last said.
     * @param deltaTime Ignored.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws every icon, honoring isVisible and fixedPosition.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    UISpriteRef states[kMaxStates];  ///< Fill-level sprites, empty .. full.
    int value = 0;                   ///< Filled amount in units (unclamped).
    int spacing = 0;                 ///< Horizontal gap between icons.
    int rowSpacing = 0;              ///< Vertical gap between wrapped rows.
    uint8_t capacity = 0;            ///< Icons drawn.
    uint8_t unitsPerIcon = 1;        ///< Units that fill one icon.
    uint8_t iconsPerRow = 0;         ///< Wrap width; 0 = no wrapping.

    /**
     * @brief Widest sprite across the configured states.
     *
     * Taken as a maximum rather than from one nominated state so a full icon
     * that is larger than its empty counterpart still fits the cell.
     */
    int iconWidth() const;

    /// Tallest sprite across the configured states. See iconWidth().
    int iconHeight() const;

    /// Icons on the widest row, given capacity and wrapping.
    int iconsOnWidestRow() const;

    /// Number of rows the icons occupy, given capacity and wrapping.
    int rowCount() const;

    /**
     * @brief Recomputes width/height from icon size, capacity and wrapping.
     *
     * Called by every setter that can change the span, so a layout always
     * reads a current preferred size.
     */
    void recalcSize();
};

}  // namespace pixelroot32::graphics::ui

#endif  // PIXELROOT32_ENABLE_UI_SYSTEM
