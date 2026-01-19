/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once
#include <cstdint>

namespace pixelroot32::graphics {

static constexpr uint16_t PALETTE_NES[] = {
    0x0000, // #000000
    0x216A, // #1D2B53
    0x792A, // #7E2553
    0xAA87, // #AB5236
    0x62AA, // #5F574F
    0xC618, // #C2C3C7
    0xFF9C, // #FFF1E8
    0xF809, // #FF004D
    0xFD00, // #FFA300
    0xFF45, // #FFEC27
    0x0707, // #00E436
    0x2D7F, // #29ADFF
    0x83B3, // #83769C
    0xFBB4, // #FF77A8
    0xFE55, // #FFCCAA
    0x042A, // #008751
};

static constexpr uint16_t PALETTE_GB[] = {
    0x11C2, // #0F380F
    0x1A63, // #1B4D1B
    0x3306, // #306230
    0x53E8, // #4F7F3F
    0x6C69, // #6B8F4E
    0x8D42, // #8BAC0F
    0x9DC2, // #9BBC0F
    0xB64D, // #B6C96A
    0xCF34, // #CFE5A5
    0xDF78, // #DDEFC7
    0xE7BB, // #E8F7DC
    0xF7FD, // #F4FFEB
    0x42E5, // #3E5F2A
    0x5BE7, // #5C7F3A
    0x7D6B, // #7FAE5C
    0xAE71, // #AFCF8F
};

static constexpr uint16_t PALETTE_GBC[] = {
    0x08C4, // #081820
    0xF7BE, // #F8F8F8
    0x334A, // #346856
    0x8DEE, // #88C070
    0xDFB9, // #E0F8D0
    0x21DD, // #2038EC
    0x5D5E, // #58A8F8
    0x18CF, // #181878
    0x7B5D, // #7B68EE
    0xA986, // #B03030
    0xE28A, // #E85050
    0xF5B6, // #F8B8B8
    0xD480, // #D89000
    0xF6E6, // #F8E030
    0x9D13, // #A0A0A0
    0x0000, // #000000
};

static constexpr uint16_t PALETTE_PICO8[] = {
    0x0000, // #000000
    0x216A, // #1D2B53
    0x792A, // #7E2553
    0x042A, // #008751
    0xAA87, // #AB5236
    0x62AA, // #5F574F
    0xC618, // #C2C3C7
    0xFF9C, // #FFF1E8
    0xF809, // #FF004D
    0xFD00, // #FFA300
    0xFF45, // #FFEC27
    0x0707, // #00E436
    0x2D7F, // #29ADFF
    0x83B3, // #83769C
    0xFBB4, // #FF77A8
    0xFE55, // #FFCCAA
};

static constexpr uint16_t PALETTE_PR32[] = {
    0x0000, // #000000
    0xFFFF, // #FFFFFF
    0x1907, // #1B1F3B
    0x025F, // #0047FF
    0x061F, // #00C2FF
    0x13C2, // #0E7A0D
    0x3648, // #2ECC40
    0xA7F3, // #A8FF9E
    0xFEA0, // #FFD500
    0xFCE3, // #FF9F1C
    0xB884, // #C1121F
    0x6822, // #6A040F
    0x7977, // #7B2CBF
    0xC3FF, // #C77DFF
    0x8C71, // #8D8D8D
    0xCE79, // #CECECE
};

}
