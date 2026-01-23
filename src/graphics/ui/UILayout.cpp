/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UILayout.h"

namespace pixelroot32::graphics::ui {

UILayout::UILayout(float x, float y, float w, float h)
    : UIElement(x, y, w, h) {
}

void UILayout::clearElements() {
    elements.clear();
    updateLayout();
}

}
