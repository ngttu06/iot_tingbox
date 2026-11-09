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
 private:
  WebServer server{80};
  DNSServer dns;
  Preferences prefs;
  String savedSsid,savedPassword,candidateSsid,candidatePassword;
  String apSsid,apPassword,csrf,status;
  bool portalActive=false,testing=false,submitted=false;
  unsigned long attemptAt=0,offlineAt=0,connectedAt=0,retryAt=0,submitAt=0;
  void page();
  void save();
  void close();
  void connectSaved();
  bool fromAp();
};