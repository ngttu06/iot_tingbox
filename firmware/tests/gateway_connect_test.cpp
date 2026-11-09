#include "../SmartPayESP32/GatewaySecureClient.h"
#include <cassert>
#include <iostream>

int main(){
  GatewaySecureClient secure;secure.setCACert("trusted CA");
  WiFiClientSecure& client=secure; // HTTPClient's virtual dispatch.
  assert(client.connect("tingbox.ngttu.com",443,10000)==1);
  assert(secureState.host=="tingbox.ngttu.com" && secureState.ca=="trusted CA");
  assert(secureState.port==443 && secureState.timeout==10000);
  assert(probeState.calls==0);
  WiFi.dns=false;
  assert(client.connect("tingbox.ngttu.com",443,10000)==0);
  assert(secureState.calls==1 && Serial.lines.back().find("stage=DNS")!=std::string::npos);
  WiFi.dns=true;secureState.result=0;
  assert(client.connect("tingbox.ngttu.com",443,10000)==0);
  assert(probeState.calls==1 && probeState.timeout==2000 && probeState.stopped);
  probeState.reachable=true;
  assert(client.connect("tingbox.ngttu.com",443,10000)==0); // TCP probe never bypasses TLS.
  assert(probeState.calls==2);
  secureState.error=-9984; // A specific TLS error must not trigger TCP probe.
  assert(client.connect("tingbox.ngttu.com",443,10000)==0 && probeState.calls==2);
  secureState.error=-1;WiFi.state=0;
  assert(client.connect("tingbox.ngttu.com",443,10000)==0 && probeState.calls==2);
  for(const auto& line:Serial.lines)assert(line.find("trusted CA")==std::string::npos);
  std::cout<<"PASS: DNS/TCP/TLS failure paths, virtual dispatch, preserved SNI/CA/timeout, bounded credential-free TCP probe and fail-closed result\n";
}