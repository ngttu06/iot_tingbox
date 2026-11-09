#pragma once
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <cerrno>
#include <cstring>

// HTTPClient calls this overload before writing ANY HTTP headers/API key.
// Resolve once, but retain the hostname for SNI and certificate verification.
class GatewaySecureClient:public WiFiClientSecure {
 public:
  int connect(const char* host,uint16_t port,int32_t timeout) override{
    const uint32_t started=millis();
    IPAddress address;
    if(!WiFi.hostByName(host,address)){
      Serial.printf("Gateway connect failed: stage=DNS, elapsed=%lums, rssi=%lddBm, wifi=%d\n",
        (unsigned long)(millis()-started),(long)WiFi.RSSI(),int(WiFi.status()));
      return 0;
    }
    // This overload keeps the original hostname; never connect by IP alone.
    _timeout=timeout;
    errno=0;
    const int connected=WiFiClientSecure::connect(address,port,host,_CA_cert,nullptr,nullptr);
    const int socketError=errno;
    if(connected)return connected;
    char detail[180]{};
    const int tls=lastError(detail,sizeof(detail));
    Serial.printf("Gateway connect failed: stage=TCP/TLS, ip=%s, elapsed=%lums, rssi=%lddBm, wifi=%d, heap=%lu, socket_errno=%d (%s), tls=%d (%s)\n",
      address.toString().c_str(),(unsigned long)(millis()-started),(long)WiFi.RSSI(),int(WiFi.status()),
      (unsigned long)ESP.getFreeHeap(),socketError,std::strerror(socketError),tls,detail);
    // -1 is often a socket failure, not a certificate error. This independent
    // bounded TCP-only probe sends no credentials and is NOT an HTTP fallback.
    // A successful probe does not prove the earlier socket/handshake succeeded.
    if(tls==-1 && WiFi.status()==WL_CONNECTED){
      WiFiClient probe;errno=0;
      const bool reachable=probe.connect(address,port,2000)==1;
      const int probeError=errno;probe.stop();
      Serial.printf("Gateway TCP probe (separate connection): ip=%s, reachable=%s, errno=%d (%s)\n",
        address.toString().c_str(),reachable?"yes":"no",probeError,std::strerror(probeError));
    }
    return 0;
  }
};