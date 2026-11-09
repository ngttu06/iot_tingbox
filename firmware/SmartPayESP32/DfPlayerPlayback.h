#pragma once
#include <cstdint>
#include <utility>
#include <vector>

// Hardware-independent sequencing. Only a debounced BUSY low -> high cycle
// completes a track. UART ACK/finish messages and elapsed time never mean success.
class DfPlayerPlayback {
 public:
  struct Command {uint8_t code=0;uint16_t argument=0;};
  void begin(uint32_t now){
    phase=Phase::RESET;since=now;running=false;completed=false;failure=nullptr;
    tracks.clear();index=0;stopPending=false;
  }
  bool start(std::vector<uint16_t> sequence){
    if(running || sequence.empty())return false;
    for(auto track:sequence)if(!track || track>9999)return false;
    tracks=std::move(sequence);index=0;running=true;completed=false;failure=nullptr;
    return true;
  }
  void fail(const char* reason){
    if(!running)return;
    running=false;completed=false;failure=reason;stopPending=true;
    if(phase==Phase::START || phase==Phase::END || phase==Phase::GAP)phase=Phase::READY;
  }
  Command tick(uint32_t now,bool busyLow){
    if(stopPending){stopPending=false;return {0x16,0};}
    switch(phase){
      case Phase::RESET:phase=Phase::BOOT;since=now;return {0x0c,0};
      case Phase::BOOT:
        if(uint32_t(now-since)<2500)return {};
        phase=Phase::DEVICE;since=now;return {0x09,2}; // SD card
      case Phase::DEVICE:
        if(uint32_t(now-since)<250)return {};
        phase=Phase::VOLUME;since=now;return {0x06,20}; // 0..30
      case Phase::VOLUME:
        if(uint32_t(now-since)<150)return {};
        phase=Phase::LOOP;since=now;return {0x19,1}; // single-track repeat off
      case Phase::LOOP:
        if(uint32_t(now-since)<150)return {};
        phase=Phase::SETTLE;since=now;return {0x16,0}; // stop boot/autoplay
      case Phase::SETTLE:
        if(uint32_t(now-since)<250)return {};
        phase=Phase::READY;
        break;
      case Phase::START:
        if(uint32_t(now-since)>=3000){fail("BUSY did not go LOW: check SD/files/power/UART/BUSY");return {};}
        if(!busyLow)edge=false;
        else if(!edge){edge=true;edgeAt=now;}
        else if(uint32_t(now-edgeAt)>=20){phase=Phase::END;since=now;edge=false;}
        return {};
      case Phase::END:
        if(uint32_t(now-since)>=15000){fail("BUSY stayed LOW / clip timeout");return {};}
        if(busyLow)edge=false;
        else if(!edge){edge=true;edgeAt=now;}
        else if(uint32_t(now-edgeAt)>=50){
          if(++index==tracks.size()){running=false;completed=true;phase=Phase::READY;}
          else{phase=Phase::GAP;since=now;}
        }
        return {};
      case Phase::GAP:
        if(uint32_t(now-since)<150)return {};
        phase=Phase::READY;
        break;
      case Phase::READY:break;
    }
    if(phase==Phase::READY && running){
      if(busyLow){fail("BUSY LOW before play: module busy or wiring fault");return {};}
      phase=Phase::START;since=now;edge=false;return {0x12,tracks[index]};
    }
    return {};
  }
  bool busy() const{return running;}
  bool succeeded() const{return completed;}
  const char* error() const{return failure;}
 private:
  enum class Phase {RESET,BOOT,DEVICE,VOLUME,LOOP,SETTLE,READY,START,END,GAP};
  Phase phase=Phase::RESET;
  uint32_t since=0,edgeAt=0;
  bool running=false,completed=false,stopPending=false,edge=false;
  std::vector<uint16_t> tracks;
  unsigned index=0;
  const char* failure=nullptr;
};