/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/Font5x7.h"
#include "graphics/Renderer.h"  // For Sprite definition

namespace pixelroot32::graphics {

// Definition of FONT5X7_GLYPHS array (declared as extern in Font5x7.h)
const Sprite FONT5X7_GLYPHS[95] = {
    {GLYPH_SPACE, 5, 7},           // 32: ' '
    {GLYPH_EXCLAMATION, 5, 7},     // 33: '!'
    {GLYPH_QUOTE, 5, 7},          // 34: '"'
    {GLYPH_HASH, 5, 7},            // 35: '#'
    {GLYPH_DOLLAR, 5, 7},          // 36: '$'
    {GLYPH_PERCENT, 5, 7},         // 37: '%'
    {GLYPH_AMPERSAND, 5, 7},      // 38: '&'
    {GLYPH_APOSTROPHE, 5, 7},      // 39: '''
    {GLYPH_OPEN_PAREN, 5, 7},      // 40: '('
    {GLYPH_CLOSE_PAREN, 5, 7},     // 41: ')'
    {GLYPH_ASTERISK, 5, 7},        // 42: '*'
    {GLYPH_PLUS, 5, 7},            // 43: '+'
    {GLYPH_COMMA, 5, 7},           // 44: ','
    {GLYPH_MINUS, 5, 7},           // 45: '-'
    {GLYPH_PERIOD, 5, 7},          // 46: '.'
    {GLYPH_SLASH, 5, 7},           // 47: '/'
    {GLYPH_0, 5, 7},               // 48: '0'
    {GLYPH_1, 5, 7},               // 49: '1'
    {GLYPH_2, 5, 7},               // 50: '2'
    {GLYPH_3, 5, 7},               // 51: '3'
    {GLYPH_4, 5, 7},               // 52: '4'
    {GLYPH_5, 5, 7},               // 53: '5'
    {GLYPH_6, 5, 7},               // 54: '6'
    {GLYPH_7, 5, 7},               // 55: '7'
    {GLYPH_8, 5, 7},               // 56: '8'
    {GLYPH_9, 5, 7},               // 57: '9'
    {GLYPH_COLON, 5, 7},           // 58: ':'
    {GLYPH_SEMICOLON, 5, 7},       // 59: ';'
    {GLYPH_LESS, 5, 7},            // 60: '<'
    {GLYPH_EQUAL, 5, 7},           // 61: '='
    {GLYPH_GREATER, 5, 7},         // 62: '>'
    {GLYPH_QUESTION, 5, 7},         // 63: '?'
    {GLYPH_AT, 5, 7},              // 64: '@'
    {GLYPH_A, 5, 7},               // 65: 'A'
    {GLYPH_B, 5, 7},               // 66: 'B'
    {GLYPH_C, 5, 7},               // 67: 'C'
    {GLYPH_D, 5, 7},               // 68: 'D'
    {GLYPH_E, 5, 7},               // 69: 'E'
    {GLYPH_F, 5, 7},               // 70: 'F'
    {GLYPH_G, 5, 7},               // 71: 'G'
    {GLYPH_H, 5, 7},               // 72: 'H'
    {GLYPH_I, 5, 7},               // 73: 'I'
    {GLYPH_J, 5, 7},               // 74: 'J'
    {GLYPH_K, 5, 7},               // 75: 'K'
    {GLYPH_L, 5, 7},               // 76: 'L'
    {GLYPH_M, 5, 7},               // 77: 'M'
    {GLYPH_N, 5, 7},               // 78: 'N'
    {GLYPH_O, 5, 7},               // 79: 'O'
    {GLYPH_P, 5, 7},               // 80: 'P'
    {GLYPH_Q, 5, 7},               // 81: 'Q'
    {GLYPH_R, 5, 7},               // 82: 'R'
    {GLYPH_S, 5, 7},               // 83: 'S'
    {GLYPH_T, 5, 7},               // 84: 'T'
    {GLYPH_U, 5, 7},               // 85: 'U'
    {GLYPH_V, 5, 7},               // 86: 'V'
    {GLYPH_W, 5, 7},               // 87: 'W'
    {GLYPH_X, 5, 7},               // 88: 'X'
    {GLYPH_Y, 5, 7},               // 89: 'Y'
    {GLYPH_Z, 5, 7},               // 90: 'Z'
    {GLYPH_OPEN_BRACKET, 5, 7},    // 91: '['
    {GLYPH_BACKSLASH, 5, 7},       // 92: '\'
    {GLYPH_CLOSE_BRACKET, 5, 7},   // 93: ']'
    {GLYPH_CARET, 5, 7},           // 94: '^'
    {GLYPH_UNDERSCORE, 5, 7},      // 95: '_'
    {GLYPH_BACKTICK, 5, 7},        // 96: '`'
    {GLYPH_a, 5, 7},               // 97: 'a'
    {GLYPH_b, 5, 7},               // 98: 'b'
    {GLYPH_c, 5, 7},               // 99: 'c'
    {GLYPH_d, 5, 7},               // 100: 'd'
    {GLYPH_e, 5, 7},               // 101: 'e'
    {GLYPH_f, 5, 7},               // 102: 'f'
    {GLYPH_g, 5, 7},               // 103: 'g'
    {GLYPH_h, 5, 7},               // 104: 'h'
    {GLYPH_i, 5, 7},               // 105: 'i'
    {GLYPH_j, 5, 7},               // 106: 'j'
    {GLYPH_k, 5, 7},               // 107: 'k'
    {GLYPH_l, 5, 7},               // 108: 'l'
    {GLYPH_m, 5, 7},               // 109: 'm'
    {GLYPH_n, 5, 7},               // 110: 'n'
    {GLYPH_o, 5, 7},               // 111: 'o'
    {GLYPH_p, 5, 7},               // 112: 'p'
    {GLYPH_q, 5, 7},               // 113: 'q'
    {GLYPH_r, 5, 7},               // 114: 'r'
    {GLYPH_s, 5, 7},               // 115: 's'
    {GLYPH_t, 5, 7},               // 116: 't'
    {GLYPH_u, 5, 7},               // 117: 'u'
    {GLYPH_v, 5, 7},               // 118: 'v'
    {GLYPH_w, 5, 7},               // 119: 'w'
    {GLYPH_x, 5, 7},               // 120: 'x'
    {GLYPH_y, 5, 7},               // 121: 'y'
    {GLYPH_z, 5, 7},               // 122: 'z'
    {GLYPH_OPEN_BRACE, 5, 7},      // 123: '{'
    {GLYPH_PIPE, 5, 7},            // 124: '|'
    {GLYPH_CLOSE_BRACE, 5, 7},     // 125: '}'
    {GLYPH_TILDE, 5, 7}            // 126: '~'
};

// Definition of FONT_5X7 instance (declared as extern in Font5x7.h)
const Font FONT_5X7 = {
    FONT5X7_GLYPHS,    // glyphs array
    32,                 // firstChar (space)
    126,                // lastChar (tilde)
    5,                  // glyphWidth
    7,                  // glyphHeight
    1,                  // spacing
    8                   // lineHeight (7 + 1)
};

} // namespace pixelroot32::graphics
