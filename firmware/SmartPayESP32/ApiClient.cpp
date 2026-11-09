#include "ApiClient.h"
#include "SpeakerTrust.h"
#include "GatewaySecureClient.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <time.h>
#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

#ifndef SPEAKER_BASE_URL
#define SPEAKER_BASE_URL "https://tingbox.ngttu.com"
#endif
#ifndef SPEAKER_ROOT_CA
#define SPEAKER_ROOT_CA SPEAKER_DEFAULT_CA
#endif
namespace {
class ResponseBuffer:public Stream {
 public:
  String text;
  size_t write(uint8_t b) override{return write(&b,1);}
  size_t write(const uint8_t* p,size_t n) override{
    if(n>16384-text.length())return 0;
    return text.concat(reinterpret_cast<const char*>(p),n)?n:0;
  }
  int available() override{return 0;}
  int read() override{return -1;}
  int peek() override{return -1;}
  void flush() override{}
};
}
ApiClient::Result ApiClient::request(JsonDocument& response,const String& body){
  if(WiFi.status()!=WL_CONNECTED)return Result::Retry;
  if(time(nullptr)<1700000000){Serial.println("Gateway waiting: NTP time not synchronized (TLS kept secure)");return Result::Retry;}
  String base=SPEAKER_BASE_URL;while(base.endsWith("/"))base.remove(base.length()-1);
  if(!base.startsWith("https://") || !strlen(SPEAKER_ROOT_CA))return Result::Invalid;
  GatewaySecureClient secure;HTTPClient http;secure.setCACert(SPEAKER_ROOT_CA);
  secure.setHandshakeTimeout(12);
  // Cloudflare/server latency can exceed the old 4s window on device Wi-Fi.
  // Requests stay off the audio core; never weaken CA/hostname verification.
  http.setConnectTimeout(10000);http.setTimeout(15000);http.setReuse(false);
  http.useHTTP10(true);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  if(!http.begin(secure,base+"/api/speaker-gateway"))return Result::Retry;
  http.addHeader("x-speaker-key",speakerKey);
  http.addHeader("Content-Type","application/json");
  int code=body.length()?http.POST(body):http.GET();
  Result result=Result::Retry;
  if(code==403)result=Result::Forbidden;
  else if(code==200){
    ResponseBuffer buffer;
    if(http.getSize()<=16384 && http.writeToStream(&buffer)>=0 &&
        !deserializeJson(response,buffer.text) && response["ok"].is<bool>() && response["ok"].as<bool>())result=Result::Ok;
    else result=Result::Invalid;
  }else if(code>=400 && code<500)result=Result::Invalid;
  if(result!=Result::Ok){
    Serial.printf("Gateway HTTP failure: method=%s, code=%d, result=%d\n",body.length()?"POST":"GET",code,int(result));
    if(code<0)Serial.printf("Gateway transport: %s (not an HTTP status)\n",HTTPClient::errorToString(code).c_str());
  }
  http.end();return result;
}
ApiClient::Result ApiClient::poll(JsonDocument& response){return request(response);}
ApiClient::Result ApiClient::ack(const char* id){
  JsonDocument doc;doc["ids"].to<JsonArray>().add(id);String body;serializeJson(doc,body);
  JsonDocument response;return request(response,body);
}