#pragma once

#include "platforms/PlatformDefaults.h"

namespace musicdemo {

// Display configuration - use different names to avoid conflicts with engine macros
constexpr int DEMO_DISPLAY_WIDTH = PHYSICAL_DISPLAY_WIDTH;
constexpr int DEMO_DISPLAY_HEIGHT = PHYSICAL_DISPLAY_HEIGHT;

// Menu configuration
constexpr float TITLE_Y = 20.0f;
constexpr int TITLE_FONT_SIZE = 2;

constexpr float BTN_WIDTH = 180.0f;
constexpr float BTN_HEIGHT = 28.0f;
constexpr float BTN_START_Y = 50.0f;
constexpr float BTN_GAP = 6.0f;
constexpr int BTN_FONT_SIZE = 1;

constexpr float NAV_INSTR_Y_OFFSET = 20.0f;
constexpr float SEL_INSTR_Y_OFFSET = 12.0f;
constexpr int INSTRUCTION_FONT_SIZE = 1;

// Sound configuration
constexpr float SOUND_NAV_FREQ = 440.0f;
constexpr float SOUND_NAV_DUR = 0.05f;
constexpr float SOUND_VOL_NAV = 0.3f;

constexpr float SOUND_BLIP_FREQ = 880.0f;
constexpr float SOUND_BLIP_DUR = 0.08f;
constexpr float SOUND_VOL_BLIP = 0.5f;

// Button mappings
constexpr int BTN_SELECT = 0;
constexpr int BTN_UP = 1;
constexpr int BTN_DOWN = 2;
constexpr int BTN_NAV_UP = 1;
constexpr int BTN_NAV_DOWN = 2;

} // namespace musicdemo