#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <FS.h>
// TFT_eSPI suppresses the global FS alias; WebServer's ETag signature requires it.
using fs::FS;
#include <WebServer.h>
#include <Preferences.h>

// Owned exclusively by networkTask; no WebServer/WiFi calls from the audio loop.
class WifiProvisioner {
 public:
  void begin(const char* defaultSsid,const char* defaultPassword);
  void loop();
  void open();
  bool active() const{return portalActive;}
  bool connecting() const{return testing || submitted;}
  const String& ssid() const{return apSsid;}
  const String& password() const{return apPassword;}
  void bankStatus(const String& value){gatewayStatus=value;}
  void bankIdentity(const String& key){speakerKey=key;}
  int takeBankAction(){int a=bankAction;bankAction=0;return a;}
 private:
  WebServer server{80};
  DNSServer dns;
  Preferences prefs;
  String savedSsid,savedPassword,candidateSsid,candidatePassword;
  String apSsid,apPassword,csrf,status,state="idle",scanJson="{\"state\":\"idle\",\"networks\":[]}";
  bool portalActive=false,testing=false,submitted=false,scanning=false;
  String gatewayStatus="Chưa kết nối gateway.";
  String speakerKey;
  int bankAction=0;
  unsigned long scanAt=0;
  unsigned long attemptAt=0,offlineAt=0,connectedAt=0,retryAt=0,submitAt=0;
  void page(int code=200,const String& formSsid="");
  void sendStatus();
  void scan();
  void finishScan();
  void headers();
  void replyError(int code,const String& message);
  void save();
  void close();
  void connectSaved();
  bool fromAp();
};