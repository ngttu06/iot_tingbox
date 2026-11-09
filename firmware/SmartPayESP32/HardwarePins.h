#pragma once

#if defined(CONFIG_IDF_TARGET_ESP32S3)
// ESP32-S3 N16R8: avoid flash/PSRAM, native USB and strapping pins.
constexpr int AUDIO_BCLK = 4;
constexpr int AUDIO_LRC = 5;
constexpr int AUDIO_DIN = 6;
#elif defined(CONFIG_IDF_TARGET_ESP32)
constexpr int AUDIO_BCLK = 26;
constexpr int AUDIO_LRC = 25;
constexpr int AUDIO_DIN = 22;
#else
#error "Unsupported SmartPay board: select ESP32 or ESP32-S3"
#endif