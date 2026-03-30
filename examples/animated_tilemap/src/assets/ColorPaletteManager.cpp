#include "ColorPaletteManager.h"

namespace animatedtilemap {
    namespace pr32 = pixelroot32;
    using namespace pr32::graphics;

    ColorPaletteManager::ColorPaletteManager() {
        // Constructor is empty as we handle initialization in initalizeGamePalettes
    }

    ColorPaletteManager::~ColorPaletteManager() {
        // Cleanup is handled by the scene arena system
    }

    void ColorPaletteManager::initalizeGamePalettes() {
        if (isInitialized) return;  // Prevent double initialization

        // Enable dual palette mode for separate sprite/background palettes
        enableDualPaletteMode(true);

        // Initialize slot banks
        initBackgroundPaletteSlots();

        // Setup standard palette assignments
        setupBackgroundPalettes();
        
        isInitialized = true;
    }

    void ColorPaletteManager::setupBackgroundPalettes() {
        // Assign standard palettes to slots
        setBackgroundCustomPaletteSlot(0, animatedtiles::BACKGROUND_PALETTE);
        setBackgroundCustomPaletteSlot(1, animatedtiles::GROUND_PALETTE);
        setBackgroundCustomPaletteSlot(2, animatedtiles::DETAILS_PALETTE);
    }
} // namespace animatedtilemap
