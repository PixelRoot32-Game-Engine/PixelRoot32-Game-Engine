#pragma once

#include <cstdint>
#include <platforms/PlatformDefaults.h>

namespace effectdemo {

// Display configuration - use different names to avoid conflicts with engine macros
constexpr int DEMO_DISPLAY_WIDTH = PHYSICAL_DISPLAY_WIDTH;
constexpr int DEMO_DISPLAY_HEIGHT = PHYSICAL_DISPLAY_HEIGHT;

// State machine
enum class EffectDemoState : uint8_t {
    MAIN,
    EFFECT_ACTIVE
};

// Effect preset durations (ms)
constexpr unsigned long PRESET_SHAKE_DUR = 2000u;
constexpr unsigned long PRESET_PUNCH_DUR = 1500u;
constexpr unsigned long PRESET_OFFSET_DUR = 2000u;

// Effect preset amplitudes
constexpr int PRESET_SHAKE_AMP = 8;
constexpr int PRESET_PUNCH_AMP = 6;
constexpr int PRESET_OFFSET_AMP = 4;

// Auto-cancel timeout (ms)
constexpr unsigned long AUTO_CANCEL_MS = 2000u;

// Menu layout
constexpr float TITLE_Y = 10.0f;
constexpr int TITLE_FONT_SIZE = 1;

constexpr float BTN_WIDTH = 180.0f;
constexpr float BTN_HEIGHT = 16.0f;
constexpr float BTN_START_Y = 30.0f;
constexpr float BTN_GAP = 2.0f;
constexpr int BTN_FONT_SIZE = 1;

constexpr float NAV_INSTR_Y_OFFSET = 35.0f;
constexpr float SEL_INSTR_Y_OFFSET = 25.0f;
constexpr int INSTRUCTION_FONT_SIZE = 1;

// Button mappings
constexpr uint8_t BTN_NAV_UP = 0;
constexpr uint8_t BTN_NAV_DOWN = 1;
constexpr uint8_t BTN_BACK = 4;
constexpr uint8_t BTN_SELECT = 5;

// Active effect label
constexpr const char* ACTIVE_EFFECT_LABEL = "Active: ";

} // namespace effectdemo
