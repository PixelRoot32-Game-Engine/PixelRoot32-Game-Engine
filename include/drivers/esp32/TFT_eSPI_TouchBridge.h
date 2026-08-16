/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Links TFT_eSPI XPT2046 touch reads to the input stack. The drawer owns the
 * TFT_eSPI instance; the touch adapter calls getTouch() on that instance.
 */
#pragma once

#if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)

#include "input/TouchPoint.h"

class TFT_eSPI;

namespace pixelroot32::drivers::esp32 {

/**
 * @brief Register the display driver used for touch (call after tft.init()).
 */
void registerTftForXpt2046Touch(TFT_eSPI* tft);

/**
 * @brief True after registerTftForXpt2046Touch has been called.
 */
bool touchBridgeHasTft();

/**
 * @brief Callback that drains any display transfer still owning the SPI bus.
 */
using TouchBusFlushFn = void (*)();

/**
 * @brief Register a hook run before every touch read (pass nullptr to clear).
 *
 * The display driver leaves the last DMA block of a frame in flight with the SPI
 * write transaction still open. Touch shares that bus, so it must flush first.
 */
void registerTouchBusFlushHook(TouchBusFlushFn fn);

/**
 * @brief Read one touch point via TFT_eSPI::getTouch (panel coordinates).
 * @param points Out; only index 0 used for single-touch XPT2046.
 * @param count Out; 0 if not pressed, 1 if pressed.
 */
void readTouchFromTftEspi(pixelroot32::input::TouchPoint* points, uint8_t& count);

} // namespace pixelroot32::drivers::esp32

#endif
