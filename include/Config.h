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

#endif // CONFIG_H
