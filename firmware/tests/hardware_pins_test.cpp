#include "../SmartPayESP32/HardwarePins.h"
#include "../tft/User_Setup.h"
#include <iostream>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#ifndef USE_HSPI_PORT
#error "S3 TFT requires explicit HSPI port"
#endif
static_assert(AUDIO_BCLK==4 && AUDIO_LRC==5 && AUDIO_DIN==6,"S3 I2S pins");
static_assert(DFPLAYER_RX==16 && DFPLAYER_TX==17 && DFPLAYER_BUSY==18,"S3 DFPlayer pins");
static_assert(TFT_MISO==13 && TFT_MOSI==11 && TFT_SCLK==12,"S3 SPI pins");
static_assert(TFT_CS==10 && TFT_DC==9 && TFT_RST==14,"S3 TFT control pins");
#else
static_assert(AUDIO_BCLK==26 && AUDIO_LRC==25 && AUDIO_DIN==22,"WROOM I2S pins");
static_assert(DFPLAYER_RX==26 && DFPLAYER_TX==25 && DFPLAYER_BUSY==22,"WROOM DFPlayer pins");
static_assert(TFT_MISO==19 && TFT_MOSI==23 && TFT_SCLK==18,"WROOM SPI pins");
static_assert(TFT_CS==5 && TFT_DC==27 && TFT_RST==14,"WROOM TFT control pins");
#endif

constexpr bool isTftPin(int pin){
  return pin==TFT_MISO || pin==TFT_MOSI || pin==TFT_SCLK ||
    pin==TFT_CS || pin==TFT_DC || pin==TFT_RST;
}
static_assert(DFPLAYER_RX!=DFPLAYER_TX && DFPLAYER_RX!=DFPLAYER_BUSY &&
  DFPLAYER_TX!=DFPLAYER_BUSY,"DFPlayer pins must be distinct");
static_assert(!isTftPin(DFPLAYER_RX) && !isTftPin(DFPLAYER_TX) &&
  !isTftPin(DFPLAYER_BUSY),"DFPlayer must not overlap TFT");
#if defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr bool isReservedS3Pin(int pin){
  return pin==0 || pin==3 || pin==19 || pin==20 || (pin>=26 && pin<=37) ||
    pin==43 || pin==44 || pin==45 || pin==46;
}
static_assert(!isReservedS3Pin(DFPLAYER_RX) && !isReservedS3Pin(DFPLAYER_TX) &&
  !isReservedS3Pin(DFPLAYER_BUSY),"Avoid S3 USB, memory, console and strapping pins");
#endif

int main(){std::cout<<"PASS: "<<USER_SETUP_INFO<<" pin mapping\n";}