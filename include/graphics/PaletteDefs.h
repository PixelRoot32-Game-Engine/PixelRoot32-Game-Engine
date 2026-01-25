/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include <cstdint>

namespace pixelroot32::graphics {

static constexpr uint16_t PALETTE_NES[] = {
    0x0000, // #000000 (Black)
    0xFF9C, // #FFF1E8 (White)
    0x216A, // #1D2B53 (Navy)
    0x2D7F, // #29ADFF (Blue)
    0xFE55, // #FFCCAA (Cyan/Skin)
    0x042A, // #008751 (DarkGreen)
    0x0707, // #00E436 (Green)
    0xFF45, // #FFEC27 (LightGreen/Yellowish)
    0xFD00, // #FFA300 (Yellow)
    0xAA87, // #AB5236 (Orange)
    0xFBB4, // #FF77A8 (LightRed/Pink)
    0xF809, // #FF004D (Red)
    0x792A, // #7E2553 (DarkRed/Maroon)
    0x83B3, // #83769C (Purple)
    0x62AA, // #5F574F (Magenta/DarkGray)
    0xC618, // #C2C3C7 (Gray)
};

static constexpr uint16_t PALETTE_GB[] = {
    0x11C2, // #0F380F (Black)
    0xF7FD, // #F4FFEB (White)
    0x42E5, // #3E5F2A (Navy/DarkerGreen)
    0x5BE7, // #5C7F3A (Blue/DarkerGreen)
    0xAE71, // #AFCF8F (Cyan/LightGreen)
    0x1A63, // #1B4D1B (DarkGreen)
    0x3306, // #306230 (Green)
    0x53E8, // #4F7F3F (LightGreen)
    0x9DC2, // #9BBC0F (Yellow/Green)
    0xB64D, // #B6C96A (Orange/LightGreen)
    0xDF78, // #DDEFC7 (LightRed/VeryLightGreen)
    0x8D42, // #8BAC0F (Red/Green)
    0x6C69, // #6B8F4E (DarkRed/Green)
    0x7D6B, // #7FAE5C (Purple/Green)
    0xE7BB, // #E8F7DC (Magenta/AlmostWhite)
    0xCF34, // #CFE5A5 (Gray/LightGreen)
};

static constexpr uint16_t PALETTE_GBC[] = {
    0x0000, // #000000 (Black)
    0xF7BE, // #F8F8F8 (White)
    0x18CF, // #181878 (Navy)
    0x21DD, // #2038EC (Blue)
    0x5D5E, // #58A8F8 (Cyan)
    0x08C4, // #081820 (DarkGreen/VeryDark)
    0x334A, // #346856 (Green)
    0x8DEE, // #88C070 (LightGreen)
    0xF6E6, // #F8E030 (Yellow)
    0xD480, // #D89000 (Orange)
    0xF5B6, // #F8B8B8 (LightRed)
    0xA986, // #B03030 (Red)
    0xE28A, // #E85050 (DarkRed/LightRed)
    0x7B5D, // #7B68EE (Purple)
    0xDFB9, // #E0F8D0 (Magenta/LightGreen)
    0x9D13, // #A0A0A0 (Gray)
};

static constexpr uint16_t PALETTE_PICO8[] = {
    0x0000, // #000000 (Black)
    0xFF9C, // #FFF1E8 (White)
    0x216A, // #1D2B53 (Navy)
    0x2D7F, // #29ADFF (Blue)
    0xFE55, // #FFCCAA (Cyan/Skin)
    0x042A, // #008751 (DarkGreen)
    0x0707, // #00E436 (Green)
    0xFF45, // #FFEC27 (LightGreen/Yellowish)
    0xFD00, // #FFA300 (Yellow)
    0xAA87, // #AB5236 (Orange)
    0xFBB4, // #FF77A8 (LightRed/Pink)
    0xF809, // #FF004D (Red)
    0x792A, // #7E2553 (DarkRed/Maroon)
    0x83B3, // #83769C (Purple)
    0x62AA, // #5F574F (Magenta/DarkGray)
    0xC618, // #C2C3C7 (Gray)
};

static constexpr uint16_t PALETTE_PR32[] = {
    0x0000, // #000000 (Black)
    0xFFFF, // #FFFFFF (White)
    0x1907, // #1B1F3B (Navy)
    0x025F, // #0047FF (Blue)
    0x061F, // #00C2FF (Cyan)
    0x13C2, // #0E7A0D (DarkGreen)
    0x3648, // #2ECC40 (Green)
    0xA7F3, // #A8FF9E (LightGreen)
    0xFEA0, // #FFD500 (Yellow)
    0xFCE3, // #FF9F1C (Orange)
    0xC3FF, // #C77DFF (LightRed/LightPurple)
    0xB884, // #C1121F (Red)
    0x6822, // #6A040F (DarkRed)
    0x7977, // #7B2CBF (Purple)
    0xCE79, // #CECECE (Magenta/LightGray)
    0x8C71, // #8D8D8D (Gray)
};

}
