#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class ApiClient {
 public:
  enum class Result { Ok,Retry,Forbidden,Invalid };
  explicit ApiClient(const String& key):speakerKey(key){}
  Result poll(JsonDocument& response);
  Result ack(const char* id);
 private:
  String speakerKey;
  Result request(JsonDocument& response,const String& body="");
};