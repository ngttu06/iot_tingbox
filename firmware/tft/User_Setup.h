#if defined(ARDUINO_ARCH_ESP32)
// This header is force-included before Arduino.h, including in library sources.
#include <sdkconfig.h>
#endif
#define ILI9488_DRIVER
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define USER_SETUP_INFO "SmartPay ESP32-S3 N16R8 ILI9488 SPI"
// Explicit SPI3 avoids the TFT_eSPI 2.5.43 / Arduino 3.x FSPI register mismatch.
#define USE_HSPI_PORT
#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#else
#define USER_SETUP_INFO "SmartPay ESP32 WROOM ILI9488 SPI"
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 27
#define TFT_RST 14
#endif
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
#define SPI_FREQUENCY 27000000