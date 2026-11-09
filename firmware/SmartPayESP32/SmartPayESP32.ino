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
#include "StartupSounds.h"
#include "SerialCommand.h"
#include "SpeakerStore.h"
#include "SpeakerRecovery.h"
#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

AudioManager audio;
DisplayManager display;
QueueHandle_t eventQueue,updateQueue,playedQueue,wifiConnectedQueue,commandQueue;
QueueHandle_t replayQueue;
StartupSounds startupSounds;
PaymentState state=PaymentState::BOOT;
PaymentEvent active{};
bool activeEvent=false,wasPlaying=false,online=false;
String shownPortal;
bool showingLink=false;
unsigned long receivedAt=0;
SerialCommand serialCommand;

static void serviceSerial(){
  // Limit work per loop so pasted input cannot starve the MP3 decoder.
  for(unsigned n=0;n<64 && Serial.available();++n){
    const char* command=serialCommand.feed((char)Serial.read());
    if(!command)continue;
    if(!strcmp(command,"status")){
      Serial.printf("Status: uptime=%lus, heap=%lu, wifi=%s, backend=%s, audio=%s, last_audio=%s, epoch=%lld\n",
        millis()/1000,(unsigned long)ESP.getFreeHeap(),WiFi.status()==WL_CONNECTED?"connected":"disconnected",
        online?"online":"offline",audio.busy()?"playing":"idle",audio.succeeded()?"OK":"not completed",(long long)time(nullptr));
    }else if(!strcmp(command,"audio-test") || !strcmp(command,"audio-test-sequence")){
      // Never take over a payment or overwrite its playback result.
      if(state==PaymentState::ERROR || activeEvent || audio.busy() || (!HARDWARE_SMOKE_TEST && uxQueueMessagesWaiting(eventQueue))){
        Serial.println("Audio test rejected: busy, pending payment or error");
      }else{
        Serial.println("Audio test: welcome / connectivity clips (no payment / no ACK)");
        const bool started=!strcmp(command,"audio-test-sequence")?audio.playDiagnosticSequence():audio.playClip("chao_mung");
        if(!started)Serial.println("Audio test failed to start");
      }
    }else if(!strncmp(command,"bank-replay ",12) || !strncmp(command,"bank-replay-amount ",19)){
      SpeakerReplayRequest requested{};
      if(!speakerReplayRequest(command,requested) || HARDWARE_SMOKE_TEST || !replayQueue){Serial.println("Replay rejected: invalid event ID/amount/command unavailable");}
      else if(xQueueSend(replayQueue,&requested,0)!=pdTRUE)Serial.println("Replay rejected: queue busy");
    }else if(!strcmp(command,"bank-link") || !strcmp(command,"bank-retry") || !strcmp(command,"setup-wifi")){
      int action=!strcmp(command,"bank-link")?1:!strcmp(command,"bank-retry")?2:4;
      if(HARDWARE_SMOKE_TEST || !commandQueue || xQueueSend(commandQueue,&action,0)!=pdTRUE)Serial.println("Command unavailable/busy");
    }else Serial.println("Commands: status | audio-test | bank-link | bank-retry | bank-replay <event-id> | bank-replay-amount <event-id> <VND> (operator confirms NOT heard / amount verified) | setup-wifi | help");
  }
}

void networkTask(void*){
  WifiProvisioner wifi;wifi.begin(WIFI_SSID,WIFI_PASSWORD);
  // Wi-Fi enables the hardware RNG before the one-time device UUID is created.
  SpeakerStore store;const bool storeReady=store.begin();bool healthy=storeReady;
  bool reviewRequired=storeReady && store.ledger.pending();
  SpeakerRecovery recovery;
  bool gatewayOnline=false;
  bool inflight=false;String awaiting;
  unsigned retry=3000;uint32_t nextPoll=0,linkUntil=millis()+120000;
  bool linkVisible=storeReady,pauseForLink=false;
  if(storeReady){wifi.bankIdentity(store.key);Serial.println("Bank link: automatic startup QR ready; local Wi-Fi portal QR enabled");}
  String message=healthy?"Đang kết nối gateway.":"Lỗi lưu trữ. Dừng xử lý; không ACK.";
  if(healthy && store.ledger.pending()){
    const auto* r=store.ledger.pending();
    if(r->test){
      // Non-financial welcome tests may resume after an interrupted diagnostic
      // reboot. NEVER apply this recovery to real payment announcements.
      PaymentEvent p{};p.test=true;strlcpy(p.id,r->id.c_str(),sizeof(p.id));
      if(xQueueSend(eventQueue,&p,0)==pdTRUE){
        inflight=true;reviewRequired=false;message="Tiếp tục thử loa bằng câu chào sau reset; không phải tiền thật.";
        Serial.printf("Gateway resuming TEST only: event=%s\n",p.id);
      }else{healthy=false;message="Không tiếp tục được thông báo thử. Lỗi hàng đợi.";}
    }else{
      healthy=false;message=String("Giao dịch dang dở sau reset. Kiểm tra thủ công; không tự đọc/ACK. Mã bank: ")+r->reference.c_str()+"; event: "+r->id.c_str();
    }
  }
  // Publish the startup QR before the first HTTPS request, even if TLS/network
  // takes seconds or the key has not yet been registered on the server.
  NetworkUpdate initial{};initial.linkQr=linkVisible;initial.portal=wifi.active();
  strlcpy(initial.speakerKey,storeReady?store.key.c_str():"",sizeof(initial.speakerKey));
  strlcpy(initial.apSsid,wifi.ssid().c_str(),sizeof(initial.apSsid));
  strlcpy(initial.apPassword,wifi.password().c_str(),sizeof(initial.apPassword));
  strlcpy(initial.gatewayMessage,message.c_str(),sizeof(initial.gatewayMessage));
  xQueueOverwrite(updateQueue,&initial);
  ApiClient api(store.key);
  auto persist=[&](size_t slot,const SpeakerLedger::Record& r){return store.write(slot,r);};
  auto resultOk=[&](ApiClient::Result result){
    if(result==ApiClient::Result::Forbidden){
      if(!store.setBlocked(true))healthy=false;
      message="403: Dừng gateway. Admin kiểm tra liên kết, rồi chọn Kiểm tra lại.";
    }else if(result==ApiClient::Result::Retry)message="Mất kết nối gateway. Đang chờ thử lại.";
    else if(result==ApiClient::Result::Invalid)message="Gateway trả dữ liệu/lỗi không hợp lệ. Không ACK.";
    return result==ApiClient::Result::Ok;
  };
  bool wifiNotified=false;
  String loggedGatewayMessage;
  auto serviceWifi=[&](){
    wifi.loop();
    if(!wifiNotified && WiFi.status()==WL_CONNECTED && !wifi.connecting()){
      configTime(0,0,"time.google.com","pool.ntp.org","time.cloudflare.com");
      const bool connected=true;
      wifiNotified=xQueueSend(wifiConnectedQueue,&connected,0)==pdTRUE;
    }
  };
  configTime(0,0,"pool.ntp.org","time.google.com");
  for(;;){
    serviceWifi();
    int action=wifi.takeBankAction(),serialAction=0;
    if(xQueueReceive(commandQueue,&serialAction,0)==pdTRUE)action=serialAction;
    SpeakerReplayRequest requested{};
    if(xQueueReceive(replayQueue,&requested,0)==pdTRUE){
      const auto* pending=store.ledger.pending();
      if(storeReady && reviewRequired && !inflight && pending && pending->scope==store.scope.c_str() && recovery.authorize(pending,requested.id)){
        healthy=true;nextPoll=0;linkVisible=false;
        Serial.printf("Gateway operator authorized ONE pending replay: event=%s\n",requested.id);
        message="Đã cho phép đọc lại đúng giao dịch chưa nghe; chờ dữ liệu API.";
        if(requested.amount){
          // Explicit local recovery of a confirmed amount, never a fallback
          // for ordinary polls. Reuse the durable pending record unchanged.
          PaymentEvent p{};strlcpy(p.id,pending->id.c_str(),sizeof(p.id));p.amount=requested.amount;
          strlcpy(p.content,pending->reference.c_str(),sizeof(p.content));strlcpy(p.bank,"Bank recovery",sizeof(p.bank));
          recovery.clear();
          if(xQueueSend(eventQueue,&p,0)==pdTRUE){
            inflight=true;reviewRequired=false;
            Serial.printf("Gateway LOCAL recovery queued: event=%s, amount=%llu VND; operator confirmed, not a new API event\n",p.id,(unsigned long long)p.amount);
            message="Khôi phục theo số tiền đã đối soát. Chờ phát xong mới lưu hoàn tất/ACK.";
          }else{healthy=false;message="Lỗi hàng đợi khôi phục. Giữ bản ghi dang dở; không ACK.";}
        }
      }else Serial.println("Replay rejected: ID mismatch, no pending real payment or audio in flight");
    }
    if(action==1){wifi.open();linkVisible=storeReady;pauseForLink=true;linkUntil=millis()+120000;}
    if(action==4)wifi.open();
    if(action==2 && healthy){
      if(!store.setBlocked(false)){healthy=false;message="Lỗi lưu trạng thái khóa.";}
      else{message="Đang kiểm tra liên kết.";nextPoll=0;linkVisible=false;}
    }
    if(action==3){
      if(!healthy || inflight || store.ledger.pending())message="Không đổi phạm vi khi đang xử lý/lỗi.";
      else if(!store.newScope() || !store.setBlocked(false)){healthy=false;message="Lỗi lưu phạm vi liên kết.";}
      else{awaiting="";message="Đã đổi phạm vi. Đang kiểm tra gateway.";nextPoll=0;linkVisible=false;}
    }
    if(action==5 && storeReady && reviewRequired){
      recovery.clear();
      // Explicit operator attestation, never automatic recovery of uncertain audio.
      const auto* pending=store.ledger.pending();
      if(!inflight && pending && store.ledger.complete(pending->id,persist)){
        healthy=true;reviewRequired=false;message="Đã xác nhận thủ công. Sẽ ACK khi server trả lại event.";nextPoll=0;
      }else message="Không thể xác nhận: đang phát, không có bản ghi hoặc lưu lỗi.";
    }
    if(linkVisible && (int32_t)(millis()-linkUntil)>=0)linkVisible=false;
    if(!linkVisible)pauseForLink=false;
    char completed[129];
    while(xQueueReceive(playedQueue,&completed,0)==pdTRUE){
      if(store.ledger.complete(completed,persist)){
        Serial.printf("Gateway playback persisted: event=%s\n",completed);
        awaiting=completed;inflight=false;nextPoll=0;
      }
      else{healthy=false;message="Lỗi lưu ID đã phát. Dừng; không ACK/không tự phát lại.";}
    }
    NetworkUpdate u{};
    if(healthy && !store.blocked && !inflight && !pauseForLink && WiFi.status()==WL_CONNECTED && !wifi.connecting() &&
        (!nextPoll || (int32_t)(millis()-nextPoll)>=0)){
      bool ok=true;
      if(awaiting.length()){
        ok=resultOk(api.ack(awaiting.c_str()));
        if(ok){Serial.printf("Gateway ACK confirmed: event=%s\n",awaiting.c_str());awaiting="";}
      }
      if(ok){
        JsonDocument batch;ok=resultOk(api.poll(batch));
        if(ok && (!batch["events"].is<JsonArray>() || batch["events"].size()>20)){
          ok=false;message="Danh sách gateway không hợp lệ. Không ACK.";
        }
        if(ok){
          message="Sẵn sàng nhận thông báo ngân hàng.";
          if(recovery.active())message="Chờ đúng event dang dở từ API; nếu hết hạn cần đối soát, không tự bỏ qua.";
          for(JsonVariant item:batch["events"].as<JsonArray>()){
            if(!item.is<JsonObject>() || !item["id"].is<String>() || !item["reference"].is<String>() ||
                !item["test"].is<bool>()){
              ok=false;message="Event thiếu/sai ID hoặc mã giao dịch. Không ACK.";break;
            }
            SpeakerLedger::Record r{item["id"].as<std::string>(),item["reference"].as<std::string>(),
              store.scope.c_str(),item["test"].as<bool>(),false};
            if(!SpeakerLedger::valid(r) || r.id.find('\0')!=std::string::npos || r.reference.find('\0')!=std::string::npos){
              ok=false;message="ID/mã giao dịch quá dài hoặc không hợp lệ.";break;
            }
            // During recovery, do not ACK/play other events, even if the target
            // has expired or lies beyond this batch. Preserve all evidence.
            if(recovery.active() && !recovery.permits(r))continue;
            // Validate currency before reference matching: never treat another
            // currency as an already-announced VND payment with the same ref.
            uint64_t amount=0;
            if(!item["currency"].is<String>() || item["currency"].as<String>()!="VND" ||
                !item["amount"].is<String>() || !speakerVndAmount(item["amount"].as<std::string>(),amount)){
              ok=false;message="Chưa hỗ trợ tiền tệ/số tiền này. Không phát/ACK.";break;
            }
            auto match=store.ledger.match(r);
            const bool replay=match==SpeakerLedger::Match::Uncertain && recovery.permits(r);
            if(match==SpeakerLedger::Match::Uncertain && !replay){healthy=false;message="Giao dịch chưa rõ đã phát. Cần kiểm tra thủ công.";break;}
            if(match==SpeakerLedger::Match::Played){
              if(!store.ledger.rememberDuplicate(r,persist)){healthy=false;message="Không lưu được ID bổ sung. Dừng; không ACK.";break;}
              if(!resultOk(api.ack(r.id.c_str()))){ok=false;break;}
              continue;
            }
            // Durable intent BEFORE handing audio to the other core. A reset here
            // requires operator review instead of risking a second announcement.
            if(!replay && !store.ledger.begin(r,persist)){healthy=false;message="Bộ nhớ đầy/lỗi. Dừng để không quên giao dịch đã đọc.";break;}
            recovery.clear();reviewRequired=false;
            PaymentEvent p{};strlcpy(p.id,r.id.c_str(),sizeof(p.id));p.amount=amount;p.test=r.test;
            strlcpy(p.content,r.reference.c_str(),sizeof(p.content));strlcpy(p.bank,"Bank gateway",sizeof(p.bank));
            if(xQueueSend(eventQueue,&p,0)!=pdTRUE){healthy=false;message="Lỗi hàng đợi âm thanh. Không ACK.";break;}
            Serial.printf("Gateway queued: event=%s, test=%s\n",p.id,p.test?"true":"false");
            inflight=true;message=p.test?"Đang thử loa bằng câu chào, không phải tiền thật; chưa ACK.":"Đang phát thông báo; chưa ACK.";break;
          }
        }
      }
      gatewayOnline=ok && healthy && !store.blocked;
      retry=gatewayOnline?3000:min(retry*2,60000U);
      nextPoll=millis()+retry;
      if(!gatewayOnline)Serial.printf("Gateway retry scheduled: %ums; played IDs retained, no audio replay\n",retry);
    }
    u.online=gatewayOnline && healthy && !store.blocked && WiFi.status()==WL_CONNECTED;
    if(store.blocked)message="403: Dừng gateway. Admin kiểm tra liên kết rồi chọn Kiểm tra lại.";
    if(loggedGatewayMessage!=message){Serial.printf("Gateway state: %s\n",message.c_str());loggedGatewayMessage=message;}
    wifi.bankStatus(message);strlcpy(u.gatewayMessage,message.c_str(),sizeof(u.gatewayMessage));
    u.linkQr=linkVisible && store.key.length()==40;
    if(u.linkQr)strlcpy(u.speakerKey,store.key.c_str(),sizeof(u.speakerKey));
    u.portal=wifi.active();
    if(u.portal){strlcpy(u.apSsid,wifi.ssid().c_str(),sizeof(u.apSsid));strlcpy(u.apPassword,wifi.password().c_str(),sizeof(u.apPassword));}
    xQueueOverwrite(updateQueue,&u);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
void setup(){
  Serial.begin(115200);
  Serial.printf("SmartPay boot: %s, flash=%lu, PSRAM=%lu\n",ESP.getChipModel(),(unsigned long)ESP.getFlashChipSize(),(unsigned long)ESP.getPsramSize());
  display.begin();audio.begin();
  Serial.println("Commands: status | audio-test | bank-link | bank-retry | bank-replay <event-id> | bank-replay-amount <event-id> <VND> (operator confirms NOT heard / amount verified) | setup-wifi | help");
  if(HARDWARE_SMOKE_TEST){
    display.order("SMOKE",150000,"00020101021238540010A00000072701240006970422011000000000000208QRIBFTTA530370454061500005802VN62120808DH12345663049264");
    audio.playAmount(150000);return;
  }
  display.message("SMARTPAY","Dang ket noi Wi-Fi / backend...");state=PaymentState::WIFI_CONNECTING;
  eventQueue=xQueueCreate(1,sizeof(PaymentEvent));updateQueue=xQueueCreate(1,sizeof(NetworkUpdate));playedQueue=xQueueCreate(1,129);
  wifiConnectedQueue=xQueueCreate(1,sizeof(bool));
  commandQueue=xQueueCreate(4,sizeof(int));
  replayQueue=xQueueCreate(1,sizeof(SpeakerReplayRequest));
  if(!eventQueue||!updateQueue||!playedQueue||!wifiConnectedQueue||!commandQueue||!replayQueue||xTaskCreatePinnedToCore(networkTask,"network",16384,nullptr,1,nullptr,0)!=pdPASS){
    display.message("ERROR","Khong tao duoc network task");state=PaymentState::ERROR;
  }
}
void loop(){
  audio.loop();
  serviceSerial();
  if(HARDWARE_SMOKE_TEST){delay(1);return;}
  if(state==PaymentState::ERROR){delay(50);return;}
  NetworkUpdate u;
  if(xQueueReceive(updateQueue,&u,0)==pdTRUE){
    online=u.online;
    if(!activeEvent){
      if(u.linkQr){
        if(!showingLink){
          showingLink=display.bankLink(u.speakerKey,u.apSsid,u.apPassword);
          Serial.println(showingLink?"Bank link: startup/requested QR drawn on TFT":"Bank link: TFT QR render failed");
        }
      }else{
      if(showingLink){showingLink=false;shownPortal="";state=PaymentState::BOOT;}
      if(u.portal){
        String detail=String("Wi-Fi: ")+u.apSsid+"\nPassword: "+u.apPassword+"\nMo trinh duyet: 192.168.4.1\n"+u.gatewayMessage;
        if(shownPortal!=detail){display.message("CAI DAT WI-FI",detail);shownPortal=detail;}
        state=PaymentState::WIFI_CONNECTING;
      }
      else if(!online){String detail=u.gatewayMessage;if(shownPortal!=detail){display.message("GATEWAY",detail);shownPortal=detail;}state=PaymentState::OFFLINE;}
      else if(state!=PaymentState::READY && millis()-receivedAt>6000){display.message("SAN SANG",u.gatewayMessage);state=PaymentState::READY;shownPortal="";}
      }
    }
  }
  bool wifiConnected;
  if(xQueueReceive(wifiConnectedQueue,&wifiConnected,0)==pdTRUE && wifiConnected)startupSounds.wifiConnected();
  if(const char* clip=startupSounds.nextClip(audio.busy(),activeEvent)){
    if(!audio.playClip(clip))Serial.printf("Startup audio failed: %s\n",clip);
  }
  // Leave payment queued until the current greeting/notification has finished.
  if(!activeEvent && !audio.busy() && xQueueReceive(eventQueue,&active,0)==pdTRUE){
    showingLink=false;
    shownPortal="";
    activeEvent=true;
    if(active.test)display.message("THU LOA - KHONG PHAI TIEN THAT","Phat cau chao de kiem tra loa. Khong xac nhan tien nhan.");
    else display.credit(active.paid,active.amount,active.content,active.bank,active.at);
    if(!audio.playNotification(active.amount,active.test)){display.message("AUDIO ERROR","Khong ACK. Reset roi kiem tra thu cong giao dich dang do.");state=PaymentState::ERROR;return;}
    state=PaymentState::AUDIO_PLAYING;wasPlaying=true;
  }
  if(activeEvent && wasPlaying && !audio.busy()){
    wasPlaying=false;
    if(!audio.succeeded()){display.message("AUDIO ERROR","Khong ACK. Reset roi kiem tra thu cong giao dich dang do.");state=PaymentState::ERROR;return;}
    if(xQueueSend(playedQueue,&active.id,0)!=pdTRUE){state=PaymentState::ERROR;return;}
    activeEvent=false;receivedAt=millis();state=PaymentState::PAYMENT_RECEIVED;
  }
  delay(1);
}