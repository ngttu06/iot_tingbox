#pragma once
#include "WiFi.h"
#include <cstring>
inline struct {
  int calls=0,result=1,error=-1,timeout=0;uint16_t port=0;std::string host,ca;
} secureState;
class WiFiClientSecure {
 protected:
  int32_t _timeout=0;
  const char* _CA_cert=nullptr;
 public:
  virtual ~WiFiClientSecure()=default;
  virtual int connect(const char*,uint16_t,int32_t){return 0;}
  int connect(IPAddress,uint16_t port,const char* host,const char* ca,const char*,const char*){
    ++secureState.calls;secureState.host=host;secureState.ca=ca?ca:"";
    secureState.timeout=_timeout;secureState.port=port;errno=113;return secureState.result;
  }
  void setCACert(const char* ca){_CA_cert=ca;}
  int lastError(char* buffer,size_t size){std::strncpy(buffer,"mock error",size);return secureState.error;}
};