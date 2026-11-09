#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include "AudioManager.h"
#include "DisplayManager.h"
#include "ApiClient.h"
#include "PaymentState.h"
#include "WifiProvisioner.h"
#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

AudioManager audio;
DisplayManager display;
QueueHandle_t eventQueue,updateQueue,playedQueue;
PaymentState state=PaymentState::BOOT;
PaymentEvent active{};
bool activeEvent=false,wasPlaying=false,online=false;
String shownOrder,shownPortal;
unsigned long receivedAt=0;

static String eventNumber(uint64_t n){char b[24];snprintf(b,sizeof(b),"%llu",(unsigned long long)n);return b;}
void networkTask(void*){
  Preferences prefs;prefs.begin("smartpay",false);
  uint64_t played=prefs.getULong64("last_played",0),awaiting=played,inflight=0;
  ApiClient api;unsigned retry=3000;
  WifiProvisioner wifi;wifi.begin(WIFI_SSID,WIFI_PASSWORD);
  configTime(0,0,"pool.ntp.org","time.google.com");
  for(;;){
    wifi.loop();
    uint64_t completed;
    while(xQueueReceive(playedQueue,&completed,0)==pdTRUE){
      // Commit only after playback; failed writes must not silently advance the cursor.
      if(prefs.putULong64("last_played",completed)==sizeof(uint64_t)){played=completed;awaiting=completed;inflight=0;}
      else{NetworkUpdate u{};xQueueOverwrite(updateQueue,&u);vTaskDelay(pdMS_TO_TICKS(1000));ESP.restart();}
    }
    NetworkUpdate u{};
    if(WiFi.status()==WL_CONNECTED && !wifi.connecting()){
      if(awaiting && api.ack(awaiting))awaiting=0;
      JsonDocument order;
      u.online=api.get("/v1/device/current-order",order);
      if(u.online && !order.isNull()){
        const char* id=order["order_id"]|"";const char* qr=order["qr_payload"]|"";
        if(strlen(id)<sizeof(u.orderId)&&strlen(qr)<sizeof(u.qr)&&order["amount"].is<uint64_t>()){
          u.hasOrder=true;u.amount=order["amount"].as<uint64_t>();strlcpy(u.orderId,id,sizeof(u.orderId));strlcpy(u.qr,qr,sizeof(u.qr));
        }
      }
      // One durable event at a time. No cursor advance until audio completed and NVS committed.
      if(u.online && !awaiting && !inflight && uxQueueMessagesWaiting(eventQueue)==0){
        JsonDocument events;
        if(api.get("/v1/device/events?after_id="+eventNumber(played),events)){
          JsonObject e=events["events"][0];
          if(!e.isNull() && e["event_id"].is<uint64_t>() && e["amount"].is<uint64_t>()){
            PaymentEvent p{};p.id=e["event_id"].as<uint64_t>();p.amount=e["amount"].as<uint64_t>();
            p.paid=String(e["type"]|"")=="ORDER_PAID";
            strlcpy(p.content,e["content"]|"",sizeof(p.content));strlcpy(p.bank,e["bank"]|"",sizeof(p.bank));strlcpy(p.at,e["created_at"]|"",sizeof(p.at));
            if(p.id>played && p.amount>0 && p.amount<=999999999999ULL && xQueueSend(eventQueue,&p,0)==pdTRUE)inflight=p.id;
          }
        }else u.online=false;
      }
    }
    u.portal=wifi.active();
    if(u.portal){strlcpy(u.apSsid,wifi.ssid().c_str(),sizeof(u.apSsid));strlcpy(u.apPassword,wifi.password().c_str(),sizeof(u.apPassword));}
    xQueueOverwrite(updateQueue,&u);
    retry=u.online?3000:min(retry*2,60000U);
    // Wake frequently to commit finished playback promptly, independent of poll backoff.
    for(unsigned waited=0;waited<retry;waited+=100){
      wifi.loop();
      if(uxQueueMessagesWaiting(playedQueue))break;
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}
void setup(){
  Serial.begin(115200);display.begin();audio.begin();
  if(HARDWARE_SMOKE_TEST){
    display.order("SMOKE",150000,"00020101021238540010A00000072701240006970422011000000000000208QRIBFTTA530370454061500005802VN62120808DH12345663049264");
    audio.playAmount(150000);return;
  }
  display.message("SMARTPAY","Dang ket noi Wi-Fi / backend...");state=PaymentState::WIFI_CONNECTING;
  eventQueue=xQueueCreate(1,sizeof(PaymentEvent));updateQueue=xQueueCreate(1,sizeof(NetworkUpdate));playedQueue=xQueueCreate(1,sizeof(uint64_t));
  if(!eventQueue||!updateQueue||!playedQueue||xTaskCreatePinnedToCore(networkTask,"network",16384,nullptr,1,nullptr,0)!=pdPASS){
    display.message("ERROR","Khong tao duoc network task");state=PaymentState::ERROR;
  }
}
void loop(){
  audio.loop();
  if(HARDWARE_SMOKE_TEST){delay(1);return;}
  if(state==PaymentState::ERROR){delay(50);return;}
  NetworkUpdate u;
  if(xQueueReceive(updateQueue,&u,0)==pdTRUE){
    online=u.online;
    if(!activeEvent){
      if(u.portal && !u.hasOrder){
        String detail=String("Wi-Fi: ")+u.apSsid+"\nPassword: "+u.apPassword+"\nMo trinh duyet: 192.168.4.1\n"+(online?"Backend OK":"Dang ket noi / chua co backend");
        if(shownPortal!=detail){display.message("CAI DAT WI-FI",detail);shownPortal=detail;}
        state=PaymentState::WIFI_CONNECTING;shownOrder="";
      }
      else if(!online){shownPortal="";if(state!=PaymentState::OFFLINE)display.message("MAT KET NOI","Khong xac nhan thanh toan khi offline");state=PaymentState::OFFLINE;shownOrder="";}
      else if(u.hasOrder){if(shownOrder!=u.orderId || state!=PaymentState::SHOW_QR){display.order(u.orderId,u.amount,u.qr);shownOrder=u.orderId;}state=PaymentState::SHOW_QR;}
      else if(state!=PaymentState::READY && millis()-receivedAt>6000){display.message("SAN SANG","MBBank - Tao don tren web POS");state=PaymentState::READY;shownOrder="";shownPortal="";}
    }
  }
  if(!activeEvent && xQueueReceive(eventQueue,&active,0)==pdTRUE){
    shownPortal="";
    activeEvent=true;display.credit(active.paid,active.amount,active.content,active.bank,active.at);
    if(!audio.playAmount(active.amount)){display.message("AUDIO ERROR","Khong ACK; khoi dong lai de thu lai");state=PaymentState::ERROR;return;}
    state=PaymentState::AUDIO_PLAYING;wasPlaying=true;
  }
  if(activeEvent && wasPlaying && !audio.busy()){
    wasPlaying=false;
    if(!audio.succeeded()){display.message("AUDIO ERROR","Khong ACK; khoi dong lai de thu lai");state=PaymentState::ERROR;return;}
    if(xQueueSend(playedQueue,&active.id,0)!=pdTRUE){state=PaymentState::ERROR;return;}
    activeEvent=false;receivedAt=millis();state=PaymentState::PAYMENT_RECEIVED;
  }
  delay(1);
}