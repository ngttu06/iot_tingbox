#include "ApiClient.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

bool ApiClient::request(const String& path,JsonDocument* response,const String& body){
  if(WiFi.status()!=WL_CONNECTED)return false;
  String base=BACKEND_URL;
  WiFiClient plain;WiFiClientSecure secure;HTTPClient http;
  http.setConnectTimeout(4000);http.setTimeout(4000);http.setReuse(false);
  bool started=false;
  if(base.startsWith("https://")){
    if(!strlen(ROOT_CA) || time(nullptr)<1700000000)return false;
    secure.setCACert(ROOT_CA);started=http.begin(secure,base+path);
  }else if(ALLOW_LOCAL_HTTP && base.startsWith("http://"))started=http.begin(plain,base+path);
  if(!started)return false;
  http.addHeader("Authorization",String("Bearer ")+DEVICE_TOKEN);
  http.addHeader("Content-Type","application/json");
  int code=body.length()?http.POST(body):http.GET();
  bool ok=code>=200 && code<300;
  if(ok && response){
    if(http.getSize()>49152)ok=false;
    else{String text=http.getString();ok=text.length()<=49152 && !deserializeJson(*response,text);}
  }
  http.end();return ok;
}
bool ApiClient::get(const String& path,JsonDocument& response){return request(path,&response);}
bool ApiClient::ack(uint64_t event){
  char b[32];snprintf(b,sizeof(b),"%llu",(unsigned long long)event);
  JsonDocument doc;doc["device_id"]=DEVICE_ID;doc["status"]="PLAYED";String body;serializeJson(doc,body);
  return request(String("/v1/device/events/")+b+"/ack",nullptr,body);
}