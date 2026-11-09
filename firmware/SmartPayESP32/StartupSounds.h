#pragma once

// Used only by the audio loop. Network task signals the first successful Wi-Fi connection.
class StartupSounds {
 public:
  void wifiConnected(){wifiPending=true;}
  const char* nextClip(bool audioBusy,bool paymentActive){
    if(audioBusy || paymentActive)return nullptr;
    if(!welcomePlayed){welcomePlayed=true;return "chao_mung";}
    if(wifiPending && !wifiPlayed){wifiPlayed=true;return "ket_noi_wifi";}
    return nullptr;
  }
 private:
  bool welcomePlayed=false,wifiPending=false,wifiPlayed=false;
};