#include "../SmartPayESP32/HardwarePins.h"
#include "../tft/User_Setup.h"
#include <iostream>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#ifndef USE_HSPI_PORT
#error "S3 TFT requires explicit HSPI port"
#endif
static_assert(AUDIO_BCLK==4 && AUDIO_LRC==5 && AUDIO_DIN==6,"S3 I2S pins");
static_assert(TFT_MISO==13 && TFT_MOSI==11 && TFT_SCLK==12,"S3 SPI pins");
static_assert(TFT_CS==10 && TFT_DC==9 && TFT_RST==14,"S3 TFT control pins");
#else
static_assert(AUDIO_BCLK==26 && AUDIO_LRC==25 && AUDIO_DIN==22,"WROOM I2S pins");
static_assert(TFT_MISO==19 && TFT_MOSI==23 && TFT_SCLK==18,"WROOM SPI pins");
static_assert(TFT_CS==5 && TFT_DC==27 && TFT_RST==14,"WROOM TFT control pins");
#endif

int main(){std::cout<<"PASS: "<<USER_SETUP_INFO<<" pin mapping\n";}