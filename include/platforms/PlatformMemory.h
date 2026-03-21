/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <string.h>

// PROGMEM compatibility macros for cross-platform flash memory access
// These macros allow the same code to access data stored in flash (ESP32/AVR)
// or conventional RAM (Desktop/Native).

#if defined(ESP32) || defined(ESP8266)
    #include <pgmspace.h>
    #define PIXELROOT32_MEMCPY_P memcpy_P
    #define PIXELROOT32_STRCMP_P strcmp_P
    #define PIXELROOT32_FLASH_ATTR PROGMEM
    
    // Data reading macros (Flash vs RAM)
    #define PIXELROOT32_READ_BYTE_P(addr) pgm_read_byte(addr)
    #define PIXELROOT32_READ_WORD_P(addr) pgm_read_word(addr)
    #define PIXELROOT32_READ_DWORD_P(addr) pgm_read_dword(addr)
    #define PIXELROOT32_READ_FLOAT_P(addr) pgm_read_float(addr)
    #define PIXELROOT32_READ_PTR_P(addr) pgm_read_ptr(addr)
#elif defined(__AVR__)
    #include <avr/pgmspace.h>
    #define PIXELROOT32_MEMCPY_P memcpy_P
    #define PIXELROOT32_STRCMP_P strcmp_P
    #define PIXELROOT32_FLASH_ATTR PROGMEM

    #define PIXELROOT32_READ_BYTE_P(addr) pgm_read_byte(addr)
    #define PIXELROOT32_READ_WORD_P(addr) pgm_read_word(addr)
    #define PIXELROOT32_READ_DWORD_P(addr) pgm_read_dword(addr)
    #define PIXELROOT32_READ_FLOAT_P(addr) pgm_read_float(addr)
    #define PIXELROOT32_READ_PTR_P(addr) pgm_read_ptr(addr)
#else
    // Fallback for non-embedded platforms (desktop, testing)
    #ifndef PROGMEM
        #define PROGMEM
    #endif
    #define PIXELROOT32_MEMCPY_P memcpy
    #define PIXELROOT32_STRCMP_P strcmp
    #define PIXELROOT32_FLASH_ATTR

    #define PIXELROOT32_READ_BYTE_P(addr) (*(const uint8_t*)(addr))
    #define PIXELROOT32_READ_WORD_P(addr) (*(const uint16_t*)(addr))
    #define PIXELROOT32_READ_DWORD_P(addr) (*(const uint32_t*)(addr))
    #define PIXELROOT32_READ_FLOAT_P(addr) (*(const float*)(addr))
    #define PIXELROOT32_READ_PTR_P(addr) (*(const void**)(addr))
#endif

// Legacy support (to be deprecated in favor of PIXELROOT32_FLASH_ATTR)
#ifndef PIXELROOT32_SCENE_FLASH_ATTR
    #define PIXELROOT32_SCENE_FLASH_ATTR PIXELROOT32_FLASH_ATTR
#endif
