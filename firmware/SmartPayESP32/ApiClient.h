#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class ApiClient {
 public:
  bool get(const String& path,JsonDocument& response);
  bool ack(uint64_t event);
 private:
  bool request(const String& path,JsonDocument* response,const String& body="");
};