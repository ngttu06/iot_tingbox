#pragma once
#include <Arduino.h>
enum class PaymentState { BOOT,WIFI_CONNECTING,READY,SHOW_QR,PAYMENT_RECEIVED,AUDIO_PLAYING,OFFLINE,ERROR };
struct PaymentEvent {
  char id[129];
  uint64_t amount;
  bool paid;
  bool test;
  char content[641];
  char bank[48];
  char at[40];
};
struct NetworkUpdate {
  bool online;
  bool portal;
  char apSsid[32];
  char apPassword[16];
  bool linkQr;
  char speakerKey[41];
  char gatewayMessage[180];
};