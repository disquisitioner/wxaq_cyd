/*
 * config.h -- Configuration settings for the CYD Weather Aggregator (wxaq_cyd)
 *
 * Author: David Bryant (david@disquiry.com)
 */

// Display cofniguration
// Screen background color. ILI9341_BLACK is a simple option but can specify
// any value in RGB565 color space
#define SCREEN_BACKGROUND 0x0006
const uint8_t screenRotation = 3; // rotation 3 orients 0,0 next to USB connector

// NTP settings, for handling timestamps and time skew.  Should be set to
// GMT given device data timestamps are all in GMT (Zulu)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int  daylightOffset_sec = 3600;

// How old do latest dweeted values have to be before displaying
// warning indicators on the display (red WX* and AQ* in the lower right
// corner of the display)?  Measured in seconds
const int dataDelayAlert_sec = 1800;

// CYD specific i2c pin configuration used in Wire.begin()
#define CYD_SDA 22
#define CYD_SCL 27

// CYD Touchscreen hardware connections
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// CYD display (TFT) pinout
#define TFT_BACKLIGHT 21
#define TFT_CS 15
#define TFT_DC 2
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_RST -1