#include "../SmartPayESP32/WifiCredentials.h"
#include <cassert>
#include <iostream>
int main(){
  assert(validWifiCredentials("shop","12345678"));
  assert(validWifiCredentials("open",""));
  assert(!validWifiCredentials("","12345678"));
  assert(validWifiCredentials(std::string(32,'s'),std::string(63,'p')));
  assert(!validWifiCredentials(std::string(33,'s'),"12345678"));
  assert(!validWifiCredentials("shop",std::string(64,'p')));
  assert(!validWifiCredentials("shop","1234567"));
  assert(!validWifiCredentials("shop","1234\n678"));
  assert(!validWifiCredentials(std::string("s\0x",3),"12345678"));
  assert(!validWifiCredentials("shop",std::string("12345678\0",9)));
  assert(portalEscape("<script>\"'&")=="&lt;script&gt;&quot;&#39;&amp;");
  assert(portalEscape("Wi-Fi shop")=="Wi-Fi shop");
  std::cout<<"12 Wi-Fi validation / HTML escaping cases passed\n";
}