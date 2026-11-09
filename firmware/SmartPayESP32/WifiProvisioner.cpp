#include "WifiProvisioner.h"
#include "WifiCredentials.h"
#include "WifiPortal.h"
#include "SpeakerLinkQr.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <algorithm>

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
  // Mains-powered speaker: prefer consistent HTTPS latency over modem sleep.
  WiFi.setSleep(false);
  server.on("/",HTTP_GET,[this]{page();});
  server.on("/save",HTTP_POST,[this]{save();});
  server.on("/bank",HTTP_POST,[this]{
    if(!fromAp() || server.arg("csrf")!=csrf){replyError(403,"Phiên cài đặt không hợp lệ.");return;}
    if(bankAction || connecting()){replyError(409,"Thiết bị đang xử lý. Vui lòng thử lại.");return;}
    String action=server.arg("action");
    if(action=="link")bankAction=1;
    else if(action=="retry")bankAction=2;
    else if(action=="scope" && server.arg("confirm")=="yes")bankAction=3;
    else if(action=="review" && server.arg("confirm")=="yes")bankAction=5;
    else{replyError(400,"Thao tác không hợp lệ.");return;}
    connectedAt=millis();headers();server.send(200,"application/json","{\"ok\":true}");
  });
  server.on("/status",HTTP_GET,[this]{sendStatus();});
  server.on("/scan",HTTP_GET,[this]{scan();});
  server.on("/scan",HTTP_POST,[this]{scan();});
  server.on("/portal.js",HTTP_GET,[this]{
    if(!fromAp()){replyError(403,"Hãy kết nối với Wi-Fi của TingBox để cài đặt.");return;}
    headers();server.send_P(200,"application/javascript; charset=utf-8",WIFI_PORTAL_JS);
  });
  server.onNotFound([this]{
    if(!fromAp()){replyError(403,"Hãy kết nối với Wi-Fi của TingBox để cài đặt.");return;}
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
  apSsid=String("TingBox-")+suffix;
  apPassword=setupApPassword([]{return esp_random();}).c_str();
  csrf=randomHex(); // CSRF remains a separate full-length random token.
  IPAddress ip(192,168,4,1);
  if(!WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0)) || !WiFi.softAP(apSsid.c_str(),apPassword.c_str(),1,false,2)){
    WiFi.softAPdisconnect(true);return;
  }
  if(!dns.start(53,"*",ip)){WiFi.softAPdisconnect(true);return;}
  server.begin();portalActive=true;connectedAt=millis();
  // Local USB console allows setup before a TFT is connected. Never log saved Wi-Fi credentials.
  Serial.printf("Wi-Fi setup AP: %s\nAP password: %s\nSetup URL: http://192.168.4.1/\n",apSsid.c_str(),apPassword.c_str());
  state="idle";status="Chọn mạng Wi-Fi 2.4 GHz và nhập mật khẩu để bắt đầu.";
}
void WifiProvisioner::close(){
  WiFi.scanDelete();scanJson="{\"state\":\"idle\",\"networks\":[]}";scanAt=0;
  server.stop();dns.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);portalActive=false;
}
void WifiProvisioner::connectSaved(){
  if(!savedSsid.isEmpty())WiFi.begin(savedSsid.c_str(),savedPassword.c_str());
}
void WifiProvisioner::headers(){
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("X-Content-Type-Options","nosniff");
  server.sendHeader("Content-Security-Policy","default-src 'none'; script-src 'self'; connect-src 'self'; style-src 'unsafe-inline'; form-action 'self'; base-uri 'none'; frame-ancestors 'none'");
}
void WifiProvisioner::replyError(int code,const String& message){
  headers();server.send(code,"text/plain; charset=utf-8",message);
}
void WifiProvisioner::page(int code,const String& formSsid){
  if(!fromAp()){replyError(403,"Hãy kết nối với Wi-Fi của TingBox để cài đặt.");return;}
  headers();
  String html=FPSTR(WIFI_PORTAL_HTML);
  html.replace("{{AP}}",portalEscape(apSsid.c_str()).c_str());
  html.replace("{{STATE}}",state);
  html.replace("{{STATUS}}",portalEscape(status.c_str()).c_str());
  html.replace("{{CSRF}}",csrf);
  html.replace("{{DISABLED}}",connecting()?"disabled":"");
  const std::string svg=speakerLinkSvg(speakerKey.c_str());
  html.replace("{{BANK_QR}}",svg.empty()?"<p>Mã liên kết chưa sẵn sàng. Hãy kiểm tra bộ nhớ thiết bị.</p>":svg.c_str());
  html.replace("{{BANK_KEY}}",svg.empty()?"Chưa có mã":speakerKey);
  // User-controlled SSID must be substituted last, never interpreted as a template.
  const String& shown=code==400?formSsid:(connecting()?candidateSsid:savedSsid);
  html.replace("{{SSID}}",portalEscape(shown.c_str()).c_str());
  server.send(code,"text/html; charset=utf-8",html);
}
void WifiProvisioner::sendStatus(){
  if(!fromAp()){replyError(403,"Hãy kết nối với Wi-Fi của TingBox để cài đặt.");return;}
  headers();JsonDocument doc;doc["state"]=state;doc["message"]=status;
  doc["gateway"]=gatewayStatus;
  String json;serializeJson(doc,json);server.send(200,"application/json; charset=utf-8",json);
}
void WifiProvisioner::scan(){
  if(!fromAp()){replyError(403,"Hãy kết nối với Wi-Fi của TingBox để cài đặt.");return;}
  headers();
  if(server.method()==HTTP_POST){
    if(server.arg("csrf")!=csrf){replyError(403,"Phiên cài đặt đã hết hạn. Hãy tải lại trang.");return;}
    if(connecting()){replyError(409,"Đang kết nối Wi-Fi. Vui lòng chờ trước khi quét lại.");return;}
    if(!scanning && (!scanAt || millis()-scanAt>=10000)){
      scanAt=millis();
      // Do not disconnect the station to scan: existing connectivity stays intact.
      int result=WiFi.scanNetworks(true,false,false,150);
      scanning=result==WIFI_SCAN_RUNNING || result>=0;
      scanJson=scanning?"{\"state\":\"scanning\",\"networks\":[]}":"{\"state\":\"error\",\"networks\":[]}";
    }
  }
  server.send(200,"application/json; charset=utf-8",scanJson);
}
void WifiProvisioner::finishScan(){
  if(!scanning)return;
  int count=WiFi.scanComplete();
  if(count==WIFI_SCAN_RUNNING && millis()-scanAt<15000)return;
  scanning=false;
  if(count<0){
    esp_wifi_scan_stop();WiFi.scanDelete();scanJson="{\"state\":\"error\",\"networks\":[]}";return;
  }
  // Bound response size and retain the 20 strongest unique SSID/security pairs.
  struct Network{String ssid;int32_t rssi;bool open;};
  Network networks[20];int used=0;
  for(int i=0;i<count && i<256;i++){
    String name=WiFi.SSID(i);if(name.isEmpty())continue;
    int32_t strength=WiFi.RSSI(i);bool isOpen=WiFi.encryptionType(i)==WIFI_AUTH_OPEN;
    int found=-1;
    for(int j=0;j<used;j++)if(networks[j].ssid==name && networks[j].open==isOpen){found=j;break;}
    if(found>=0){networks[found].rssi=std::max(networks[found].rssi,strength);}
    else if(used<20){networks[used++]={name,strength,isOpen};}
    else if(strength>networks[used-1].rssi){networks[used-1]={name,strength,isOpen};}
    std::sort(networks,networks+used,[](const Network& a,const Network& b){return a.rssi>b.rssi;});
  }
  JsonDocument doc;doc["state"]="ready";JsonArray list=doc["networks"].to<JsonArray>();
  for(int i=0;i<used;i++){
    JsonObject item=list.add<JsonObject>();item["ssid"]=networks[i].ssid;item["rssi"]=networks[i].rssi;item["open"]=networks[i].open;
  }
  scanJson="";serializeJson(doc,scanJson);WiFi.scanDelete();
}
void WifiProvisioner::save(){
  if(!fromAp() || server.arg("csrf")!=csrf){replyError(403,"Phiên cài đặt không hợp lệ. Hãy kết nối Wi-Fi TingBox rồi tải lại trang.");return;}
  if(testing || submitted || (submitAt && millis()-submitAt<3000)){replyError(429,"Thiết bị đang xử lý. Vui lòng chờ trước khi thử lại.");return;}
  if(scanning){replyError(409,"Đang quét mạng Wi-Fi. Vui lòng chờ vài giây rồi kết nối.");return;}
  String s=server.arg("ssid"),p=server.arg("password");
  if(!validWifiCredentials(s.c_str(),p.c_str()) || s.length()!=strlen(s.c_str()) || p.length()!=strlen(p.c_str())){
    state="error";status="Tên Wi-Fi cần 1–32 byte. Mật khẩu cần 8–63 ký tự không dấu, hoặc để trống với mạng không có mật khẩu.";
    if(server.arg("ajax")=="1")replyError(400,status);else page(400,s);
    return;
  }
  candidateSsid=s;candidatePassword=p;submitted=true;submitAt=millis();
  state="connecting";status="Đang thử kết nối, tối đa 30 giây. Nếu điện thoại bị ngắt, hãy kết nối lại mạng TingBox của thiết bị.";
  if(server.arg("ajax")=="1")sendStatus();
  else{headers();server.sendHeader("Location","/");server.send(303,"text/plain","");}
}
void WifiProvisioner::loop(){
  if(portalActive){dns.processNextRequest();server.handleClient();}
  unsigned long now=millis();
  finishScan();
  if(submitted && now-submitAt>=500){
    submitted=false;testing=true;attemptAt=now;
    WiFi.disconnect(false,false);WiFi.begin(candidateSsid.c_str(),candidatePassword.c_str());
  }
  if(testing){
    if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==candidateSsid){
      JsonDocument doc;doc["ssid"]=candidateSsid;doc["password"]=candidatePassword;String json;serializeJson(doc,json);
      if(prefs.putString("credentials",json)==json.length()){
        savedSsid=candidateSsid;savedPassword=candidatePassword;state="success";status="Đã lưu Wi-Fi. TingBox sẽ tự kết nối khi bật lại. Trang cài đặt sẽ đóng sau 2 phút có Wi-Fi. Bạn có thể chuyển điện thoại về mạng thường dùng.";
      }else{
        state="error";status="Không lưu được vào bộ nhớ. Cấu hình cũ được giữ nguyên; vui lòng thử lại.";
        WiFi.disconnect(false,false);connectSaved();
      }
      candidateSsid="";candidatePassword="";testing=false;connectedAt=now;
    }else if(now-attemptAt>=30000){
      testing=false;candidateSsid="";candidatePassword="";
      state="error";status="Không thể kết nối. Hãy kiểm tra tên mạng, mật khẩu và Wi-Fi 2.4 GHz. Cấu hình cũ được giữ nguyên.";
      WiFi.disconnect(false,false);connectSaved();connectedAt=now;
    }
  }
  if(WiFi.status()==WL_CONNECTED){
    offlineAt=0;
    if(!connectedAt)connectedAt=now;
    // Keep setup/QR available while a phone is connected to this protected AP.
    if(portalActive && WiFi.softAPgetStationNum()>0)connectedAt=now;
    if(portalActive && !testing && !submitted && !scanning && now-connectedAt>=120000)close();
  }else{
    if(state=="success"){state="error";status="Wi-Fi đã lưu nhưng hiện bị mất kết nối. TingBox đang thử kết nối lại; hãy kiểm tra bộ phát Wi-Fi.";}
    connectedAt=0;
    if(!offlineAt)offlineAt=now;
    if(now-offlineAt>=30000 && !portalActive)open();
    if(!testing && !submitted && !scanning && now-retryAt>=30000){retryAt=now;connectSaved();}
  }
}