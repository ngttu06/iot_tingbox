#include "WifiProvisioner.h"
#include "WifiCredentials.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_system.h>

static String randomHex(){
  char value[33];for(int i=0;i<4;i++)snprintf(value+i*8,9,"%08lx",(unsigned long)esp_random());return value;
}
void WifiProvisioner::begin(const char* defaultSsid,const char* defaultPassword){
  prefs.begin("wifi-setup",false);
  JsonDocument stored;
  if(!deserializeJson(stored,prefs.getString("credentials",""))){
    String s=stored["ssid"]|"",p=stored["password"]|"";
    if(validWifiCredentials(s.c_str(),p.c_str())){savedSsid=s;savedPassword=p;}
  }
  if(savedSsid.isEmpty()){savedSsid=defaultSsid;savedPassword=defaultPassword;}
  WiFi.persistent(false);WiFi.setAutoReconnect(false);WiFi.mode(WIFI_AP_STA);
  server.on("/",HTTP_GET,[this]{page();});
  server.on("/save",HTTP_POST,[this]{save();});
  server.onNotFound([this]{
    if(!fromAp()){server.send(403,"text/plain","AP clients only");return;}
    server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","");
  });
  open();connectSaved();offlineAt=millis();retryAt=millis();
}
bool WifiProvisioner::fromAp(){
  return server.client().localIP()==WiFi.softAPIP();
}
void WifiProvisioner::open(){
  if(portalActive)return;
  WiFi.mode(WIFI_AP_STA);
  char suffix[9];snprintf(suffix,sizeof(suffix),"%06lx",(unsigned long)(ESP.getEfuseMac()&0xffffff));
  apSsid=String("SmartPay-")+suffix;apPassword=randomHex().substring(0,12);csrf=randomHex();
  IPAddress ip(192,168,4,1);
  if(!WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0)) || !WiFi.softAP(apSsid.c_str(),apPassword.c_str(),1,false,2)){
    WiFi.softAPdisconnect(true);return;
  }
  if(!dns.start(53,"*",ip)){WiFi.softAPdisconnect(true);return;}
  server.begin();portalActive=true;connectedAt=millis();
  status="Nhap Wi-Fi 2.4 GHz. Mat khau chi luu tren thiet bi.";
}
void WifiProvisioner::close(){
  server.stop();dns.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);portalActive=false;
}
void WifiProvisioner::connectSaved(){
  if(!savedSsid.isEmpty())WiFi.begin(savedSsid.c_str(),savedPassword.c_str());
}
void WifiProvisioner::page(){
  if(!fromAp()){server.send(403,"text/plain","AP clients only");return;}
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("X-Content-Type-Options","nosniff");
  server.sendHeader("Content-Security-Policy","default-src 'none'; style-src 'unsafe-inline'; form-action 'self'; frame-ancestors 'none'");
  String html=F("<!doctype html><html lang='vi'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>SmartPay Wi-Fi</title><style>body{font:18px system-ui;max-width:460px;margin:30px auto;padding:20px}input,button{box-sizing:border-box;width:100%;padding:12px;margin:10px 0}small{display:block}</style></head><body><h1>Cai dat Wi-Fi SmartPay</h1><p>");
  html+=portalEscape(status.c_str()).c_str();
  html+=F("</p><form method='post' action='/save'><input type='hidden' name='csrf' value='");html+=csrf;
  html+=F("'><label>Ten Wi-Fi (SSID)<input name='ssid' maxlength='32' required autocomplete='off' value='");
  html+=portalEscape(savedSsid.c_str()).c_str();
  html+=F("'></label><label>Mat khau Wi-Fi<input type='password' name='password' maxlength='63' autocomplete='new-password'></label><small>De trong neu mang khong co mat khau. Khong hien lai mat khau da luu.</small><button>Ket noi va luu</button></form><p><a href='/'>Cap nhat trang thai</a></p><p>Chi luu sau khi ket noi thanh cong. Ket noi Wi-Fi khong dong nghia backend/Internet da hoat dong.</p></body></html>");
  server.send(200,"text/html; charset=utf-8",html);
}
void WifiProvisioner::save(){
  if(!fromAp() || server.arg("csrf")!=csrf){server.send(403,"text/plain","Invalid setup session");return;}
  if(testing || submitted || (submitAt && millis()-submitAt<3000)){server.send(429,"text/plain","Please wait before retrying");return;}
  String s=server.arg("ssid"),p=server.arg("password");
  if(!validWifiCredentials(s.c_str(),p.c_str()) || s.length()!=strlen(s.c_str()) || p.length()!=strlen(p.c_str())){
    server.send(400,"text/plain","SSID: 1-32 bytes; password: empty or 8-63 printable ASCII characters");return;
  }
  candidateSsid=s;candidatePassword=p;submitted=true;submitAt=millis();
  status="Dang thu ket noi. Doi khoang 30 giay roi bam Cap nhat trang thai. Neu dien thoai mat ket noi, hay ket noi lai Wi-Fi SmartPay.";
  server.sendHeader("Location","/");server.send(303,"text/plain","");
}
void WifiProvisioner::loop(){
  if(portalActive){dns.processNextRequest();server.handleClient();}
  unsigned long now=millis();
  if(submitted && now-submitAt>=500){
    submitted=false;testing=true;attemptAt=now;
    WiFi.disconnect(false,false);WiFi.begin(candidateSsid.c_str(),candidatePassword.c_str());
  }
  if(testing){
    if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==candidateSsid){
      JsonDocument doc;doc["ssid"]=candidateSsid;doc["password"]=candidatePassword;String json;serializeJson(doc,json);
      if(prefs.putString("credentials",json)==json.length()){
        savedSsid=candidateSsid;savedPassword=candidatePassword;status="Da ket noi va luu Wi-Fi. Thiet bi se tu ket noi sau khi bat lai. Trang cai dat se tat sau 2 phut.";
      }else{
        status="Loi luu bo nho. Chua thay doi cau hinh cu; vui long thu lai.";
        WiFi.disconnect(false,false);connectSaved();
      }
      candidateSsid="";candidatePassword="";testing=false;connectedAt=now;
    }else if(now-attemptAt>=30000){
      testing=false;candidateSsid="";candidatePassword="";
      status="Khong ket noi duoc. Kiem tra Wi-Fi 2.4 GHz/mat khau. Cau hinh cu duoc giu nguyen.";
      WiFi.disconnect(false,false);connectSaved();connectedAt=now;
    }
  }
  if(WiFi.status()==WL_CONNECTED){
    offlineAt=0;
    if(!connectedAt)connectedAt=now;
    if(portalActive && !testing && !submitted && now-connectedAt>=120000)close();
  }else{
    connectedAt=0;
    if(!offlineAt)offlineAt=now;
    if(now-offlineAt>=30000 && !portalActive)open();
    if(!testing && !submitted && now-retryAt>=30000){retryAt=now;connectSaved();}
  }
}