#pragma once

#if defined(CONFIG_IDF_TARGET_ESP32S3)
// ESP32-S3 N16R8: avoid flash/PSRAM, native USB and strapping pins.
constexpr int AUDIO_BCLK = 4;
constexpr int AUDIO_LRC = 5;
constexpr int AUDIO_DIN = 6;
constexpr int DFPLAYER_RX = 4; // ESP RX <- module TX
constexpr int DFPLAYER_TX = 5; // ESP TX -> module RX, series 1k resistor
constexpr int DFPLAYER_BUSY = 6; // module BUSY, active LOW
#elif defined(CONFIG_IDF_TARGET_ESP32)
constexpr int AUDIO_BCLK = 26;
constexpr int AUDIO_LRC = 25;
constexpr int AUDIO_DIN = 22;
constexpr int DFPLAYER_RX = 26;
constexpr int DFPLAYER_TX = 25;
constexpr int DFPLAYER_BUSY = 22;
#else
#error "Unsupported SmartPay board: select ESP32 or ESP32-S3"
#endif