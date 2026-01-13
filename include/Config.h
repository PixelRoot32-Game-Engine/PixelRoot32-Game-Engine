#ifndef CONFIG_H
#define CONFIG_H

// Display configuration
// Direct draw mode - no framebuffer
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240
#define DISPLAY_ROTATION 0

// Frame timing (milliseconds)
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000 / TARGET_FPS)  // ~16ms for 60 FPS

// Menu inactivity timeout (milliseconds)
#define MENU_INACTIVITY_TIMEOUT 10000  // 10 seconds

// Color definitions (RGB565 format)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_ORANGE  0xFD20
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_GOLD    0xFFC0  // Gold color (RGB565: R=255, G=192, B=0)

#endif // CONFIG_H
