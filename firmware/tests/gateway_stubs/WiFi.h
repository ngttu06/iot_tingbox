#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <cerrno>
constexpr int WL_CONNECTED=3;
inline uint32_t millis(){return 100;}
struct IPAddress {std::string toString() const{return "192.0.2.1";}};
inline struct {
  bool dns=true;int state=WL_CONNECTED;
  bool hostByName(const char*,IPAddress&){return dns;}
  long RSSI(){return -60;}
  int status(){return state;}
} WiFi;
inline struct {unsigned long getFreeHeap(){return 200000;}} ESP;
inline struct {
  std::vector<std::string> lines;
  template<typename... Args> void printf(const char* format,Args... args){
    char line[1024];std::snprintf(line,sizeof(line),format,args...);lines.emplace_back(line);
  }
} Serial;
inline struct {int calls=0;bool reachable=false;bool stopped=false;uint16_t port=0;int timeout=0;} probeState;
struct WiFiClient {
  int connect(IPAddress,uint16_t port,int timeout){
    ++probeState.calls;probeState.port=port;probeState.timeout=timeout;errno=111;return probeState.reachable;
  }
  void stop(){probeState.stopped=true;}
};