#pragma once

#include <graphics/Color.h>

#include "AnimatedTiles.h"

namespace animatedtilemap {
    class ColorPaletteManager {
    public:
        ColorPaletteManager();
        virtual ~ColorPaletteManager();

        void initalizeGamePalettes();

    private:
        bool isInitialized = false;

        void setupBackgroundPalettes();
    };
} // namespace animatedtilemap
