#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UILayout.h"

namespace pixelroot32::graphics::ui {



void UILayout::clearElements() {
    elements.clear();
    updateLayout();
}

}

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
