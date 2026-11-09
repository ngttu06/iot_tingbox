#pragma once
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_system.h>
#include "SpeakerLedger.h"

class SpeakerStore {
 public:
  SpeakerLedger ledger;
  String key,scope;
  bool blocked=false;
  bool begin(){
    if(!device.begin("spk-device",false) || !events.begin("spk-events",false))return false;
    key=device.getString("key","");
    if(key.isEmpty()){
      if(device.isKey("scope") || device.isKey("created"))return false;
      uint8_t b[16];esp_fill_random(b,sizeof(b));b[6]=(b[6]&15)|64;b[8]=(b[8]&63)|128;
      char uuid[41];snprintf(uuid,sizeof(uuid),"SPK-%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);
      key=uuid;if(device.putString("key",key)!=key.length())return false;
    }
    if(key.length()!=40 || !key.startsWith("SPK-"))return false;
    for(size_t i=4;i<40;++i){
      if(i==12 || i==17 || i==22 || i==27){if(key[i]!='-')return false;}
      else if(!((key[i]>='0'&&key[i]<='9') || (key[i]>='a'&&key[i]<='f')))return false;
    }
    scope=device.getString("scope","");
    if(scope.isEmpty()){
      if(device.isKey("created"))return false;
      if(!newScope())return false;
    }
    if(scope.length()!=32 || device.putBool("created",true)!=1)return false;
    blocked=device.getBool("blocked",false);
    const size_t expected=events.getUInt("count",0);
    if(expected>SpeakerLedger::capacity)return false;
    bool gap=false;
    for(size_t i=0;i<SpeakerLedger::capacity;++i){
      String name=String("e")+unsigned(i);
      if(!events.isKey(name.c_str())){gap=true;continue;}
      if(gap)return false;
      JsonDocument doc;String json=events.getString(name.c_str(),"");
      if(deserializeJson(doc,json) || !doc["id"].is<String>() || !doc["reference"].is<String>() ||
          !doc["scope"].is<String>() || !doc["test"].is<bool>() || !doc["done"].is<bool>())return false;
      SpeakerLedger::Record r{doc["id"].as<std::string>(),doc["reference"].as<std::string>(),
        doc["scope"].as<std::string>(),doc["test"].as<bool>(),doc["done"].as<bool>()};
      if(!ledger.load(i,r))return false;
    }
    return ledger.size()==expected;
  }
  bool write(size_t slot,const SpeakerLedger::Record& r){
    JsonDocument doc;doc["id"]=r.id;doc["reference"]=r.reference;doc["scope"]=r.scope;doc["test"]=r.test;doc["done"]=r.done;
    String json;serializeJson(doc,json);String name=String("e")+unsigned(slot);
    if(events.putString(name.c_str(),json)!=json.length())return false;
    // Count makes a missing tail record detectable. A torn two-key append
    // fails closed on next boot rather than silently forgetting any event.
    if(slot==ledger.size() && events.putUInt("count",slot+1)!=sizeof(uint32_t))return false;
    return true;
  }
  bool newScope(){
    if(ledger.pending())return false;
    uint8_t bytes[16];esp_fill_random(bytes,sizeof(bytes));char text[33];
    for(size_t i=0;i<16;++i)snprintf(text+i*2,3,"%02x",bytes[i]);
    String next=text;if(device.putString("scope",next)!=next.length())return false;
    scope=next;return true;
  }
  bool setBlocked(bool value){if(device.putBool("blocked",value)!=1)return false;blocked=value;return true;}
 private:
  Preferences device,events;
};