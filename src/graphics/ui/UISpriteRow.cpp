#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UISpriteRow.h"

namespace pixelroot32::graphics::ui {

namespace {

/// True if `stateIndex` addresses a real slot in the fixed states array.
bool isValidStateIndex(int stateIndex) {
    return stateIndex >= 0 && stateIndex < UISpriteRow::kMaxStates;
}

}  // namespace

UISpriteRow::UISpriteRow(pixelroot32::math::Vector2 position)
    : UIElement(position, 0, 0, UIElementType::GENERIC) {
}

UISpriteRow::UISpriteRow(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y)
    : UIElement(x, y, 0, 0, UIElementType::GENERIC) {
}

void UISpriteRow::setStateSprite(int stateIndex, const Sprite& sprite, Color tint) {
    if (!isValidStateIndex(stateIndex)) return;
    states[stateIndex] = makeUISpriteRef(sprite, tint);
    recalcSize();
}

void UISpriteRow::setStateSprite(int stateIndex, const Sprite2bpp& sprite, uint8_t paletteSlot) {
    if (!isValidStateIndex(stateIndex)) return;
    states[stateIndex] = makeUISpriteRef(sprite, paletteSlot);
    recalcSize();
}

void UISpriteRow::setStateSprite(int stateIndex, const Sprite4bpp& sprite, uint8_t paletteSlot) {
    if (!isValidStateIndex(stateIndex)) return;
    states[stateIndex] = makeUISpriteRef(sprite, paletteSlot);
    recalcSize();
}

void UISpriteRow::setUnitsPerIcon(uint8_t units) {
    // A fill level of N needs N+1 sprites (empty plus N steps), so the states
    // array size is the hard ceiling here.
    constexpr uint8_t maxUnits = static_cast<uint8_t>(kMaxStates - 1);
    if (units < 1) {
        unitsPerIcon = 1;
    } else if (units > maxUnits) {
        unitsPerIcon = maxUnits;
    } else {
        unitsPerIcon = units;
    }
}

void UISpriteRow::setCapacity(uint8_t iconCount) {
    capacity = iconCount;
    recalcSize();
}

void UISpriteRow::setSpacing(int pixels) {
    spacing = pixels;
    recalcSize();
}

void UISpriteRow::setRowSpacing(int pixels) {
    rowSpacing = pixels;
    recalcSize();
}

void UISpriteRow::setIconsPerRow(uint8_t perRow) {
    iconsPerRow = perRow;
    recalcSize();
}

int UISpriteRow::iconWidth() const {
    int widest = 0;
    for (int i = 0; i < kMaxStates; ++i) {
        const int candidate = uiSpriteRefWidth(states[i]);
        if (candidate > widest) widest = candidate;
    }
    return widest;
}

int UISpriteRow::iconHeight() const {
    int tallest = 0;
    for (int i = 0; i < kMaxStates; ++i) {
        const int candidate = uiSpriteRefHeight(states[i]);
        if (candidate > tallest) tallest = candidate;
    }
    return tallest;
}

int UISpriteRow::iconsOnWidestRow() const {
    const int total = static_cast<int>(capacity);
    if (iconsPerRow == 0) return total;
    const int perRow = static_cast<int>(iconsPerRow);
    return (total < perRow) ? total : perRow;
}

int UISpriteRow::rowCount() const {
    const int total = static_cast<int>(capacity);
    if (total <= 0) return 0;
    if (iconsPerRow == 0) return 1;
    const int perRow = static_cast<int>(iconsPerRow);
    return (total + perRow - 1) / perRow;
}

void UISpriteRow::recalcSize() {
    const int columns = iconsOnWidestRow();
    const int rows = rowCount();

    width = (columns > 0) ? (columns * iconWidth() + (columns - 1) * spacing) : 0;
    height = (rows > 0) ? (rows * iconHeight() + (rows - 1) * rowSpacing) : 0;
}

int UISpriteRow::stateIndexAt(int iconIndex) const {
    if (iconIndex < 0 || iconIndex >= static_cast<int>(capacity)) return 0;

    // Widened deliberately: `value` is caller-supplied and unclamped, and
    // iconIndex * unitsPerIcon can exceed int range for absurd inputs long
    // before the subtraction would have been meaningful.
    const long long units = static_cast<long long>(unitsPerIcon);
    const long long filled = static_cast<long long>(value) - static_cast<long long>(iconIndex) * units;

    if (filled <= 0) return 0;
    if (filled >= units) return static_cast<int>(units);
    return static_cast<int>(filled);
}

void UISpriteRow::iconOffsetAt(int iconIndex, int& outOffsetX, int& outOffsetY) const {
    outOffsetX = 0;
    outOffsetY = 0;
    if (iconIndex < 0 || iconIndex >= static_cast<int>(capacity)) return;

    const int perRow = (iconsPerRow == 0) ? static_cast<int>(capacity) : static_cast<int>(iconsPerRow);
    if (perRow <= 0) return;

    const int column = iconIndex % perRow;
    const int row = iconIndex / perRow;

    outOffsetX = column * (iconWidth() + spacing);
    outOffsetY = row * (iconHeight() + rowSpacing);
}

void UISpriteRow::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void UISpriteRow::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!isVisible) return;
    if (capacity == 0) return;

    bool oldBypass = renderer.isOffsetBypassEnabled();
    if (fixedPosition) {
        renderer.setOffsetBypass(true);
    }

    const int originX = static_cast<int>(position.x);
    const int originY = static_cast<int>(position.y);

    for (int i = 0; i < static_cast<int>(capacity); ++i) {
        const UISpriteRef& state = states[stateIndexAt(i)];
        // A row configured with only some of its states skips the unset ones
        // instead of drawing garbage — partial configuration is a legitimate
        // intermediate state while a game is being built.
        if (!uiSpriteRefIsSet(state)) continue;

        int offsetX = 0;
        int offsetY = 0;
        iconOffsetAt(i, offsetX, offsetY);

        drawUISpriteRef(renderer, state, originX + offsetX, originY + offsetY, false);
    }

    if (fixedPosition) {
        renderer.setOffsetBypass(oldBypass);
    }
}

}  // namespace pixelroot32::graphics::ui

#endif  // PIXELROOT32_ENABLE_UI_SYSTEM
