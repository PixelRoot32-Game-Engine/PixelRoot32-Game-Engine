//                            USER DEFINED SETTINGS FOR PLATFORMIO
//   Configuration for ST7789 240x240 display on ESP32
//   Based on working Arduino configuration

// User defined information reported by "Read_User_Setup" test & diagnostics example
#define USER_SETUP_INFO "User_Setup_PlatformIO_ST7789"

// ##################################################################################
//
// Section 1. Call up the right driver file and any options for it
//
// ##################################################################################

// Only define one driver, the other ones must be commented out
#define ST7789_DRIVER      // Full configuration option for ST7789 display

// For ST7789, define the pixel width and height in portrait orientation
#define TFT_WIDTH  240 // ST7789 240 x 240
#define TFT_HEIGHT 240 // ST7789 240 x 240

// ##################################################################################
//
// Section 2. Define the pins that are used to interface with the display here
//
// ##################################################################################

// ###### EDIT THE PIN NUMBERS IN THE LINES FOLLOWING TO SUIT YOUR ESP32 SETUP   ######

// For ESP32 Dev board with ST7789 display
// The hardware SPI can be mapped to any pins

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_DC    2
#define TFT_RST   4
#define TFT_CS   -1   // ST7789 sin CS

// ##################################################################################
//
// Section 3. Define the fonts that are to be used here
//
// ##################################################################################

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define SMOOTH_FONT

// ##################################################################################
//
// Section 4. Other options
//
// ##################################################################################

// Define the SPI clock frequency, this affects the graphics rendering speed.
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
