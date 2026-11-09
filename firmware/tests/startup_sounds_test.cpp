#include "../SmartPayESP32/StartupSounds.h"
#include "../SmartPayESP32/AudioAssets.h"
#include <cassert>
#include <cstring>
#include <iostream>

static void expectClip(const char* actual,const char* expected){
  assert(actual && !std::strcmp(actual,expected));
  const auto* clip=findAudioClip(actual);
  assert(clip && clip->size>0);
}

int main(){
  StartupSounds normal;
  expectClip(normal.nextClip(false,false),"chao_mung");
  assert(!normal.nextClip(false,false)); // No successful Wi-Fi yet.
  normal.wifiConnected();
  assert(!normal.nextClip(true,false)); // Greeting still playing.
  expectClip(normal.nextClip(false,false),"ket_noi_wifi");
  for(int i=0;i<100;i++){
    normal.wifiConnected(); // Reconnects must not repeat the notification.
    assert(!normal.nextClip(false,false));
  }

  StartupSounds earlyWifi;
  earlyWifi.wifiConnected();
  assert(!earlyWifi.nextClip(true,false));
  expectClip(earlyWifi.nextClip(false,false),"chao_mung");
  assert(!earlyWifi.nextClip(false,true)); // Never steal payment playback/result.
  assert(!earlyWifi.nextClip(true,true));
  expectClip(earlyWifi.nextClip(false,false),"ket_noi_wifi");
  assert(!earlyWifi.nextClip(false,false));

  StartupSounds reboot;
  expectClip(reboot.nextClip(false,false),"chao_mung");
  reboot.wifiConnected();
  expectClip(reboot.nextClip(false,false),"ket_noi_wifi");
  std::cout<<"PASS: welcome/Wi-Fi order, offline boot, busy/payment guards, reconnect dedup and reboot\n";
}