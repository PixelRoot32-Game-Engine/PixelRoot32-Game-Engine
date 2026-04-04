#pragma once
#include <cstdint>

// Custom Neon Palette for TicTacToe
static const uint16_t CUSTOM_NEON_PALETTE[16] = {
    0x0000, // 0: Black (Background)
    0xFFFF, // 1: White (Text)
    0x0010, // 2: Navy
    0x07FF, // 3: Blue (LightBlue maps here) -> Neon Cyan (O Color)
    0x07E0, // 4: Cyan
    0x2104, // 5: DarkGreen (Olive maps here) -> Dark Gray (Grid)
    0x07E0, // 6: Green
    0x7FE0, // 7: LightGreen
    0xFFE0, // 8: Yellow (Gold maps here) -> Neon Yellow (Border/Cursor)
    0xFD20, // 9: Orange
    0xF81F, // 10: LightRed -> Neon Pink (X Color)
    0xF800, // 11: Red
    0x8000, // 12: DarkRed
    0x8010, // 13: Purple
    0xF81F, // 14: Magenta
    0x8410  // 15: Gray (LightGray maps here) -> Gray (Instructions)
};