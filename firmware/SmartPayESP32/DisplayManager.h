#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager {
 public:
  void begin();
  void message(const String& title,const String& detail="");
  bool order(const String& id,uint64_t amount,const String& payload);
  void credit(bool paid,uint64_t amount,const String& content,const String& bank,const String& at);
 private:
  TFT_eSPI tft;
  void wrapped(const String& text,int y,int maxLines);
};