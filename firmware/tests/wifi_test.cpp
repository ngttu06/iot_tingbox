#include "../SmartPayESP32/WifiCredentials.h"
#include <cassert>
#include <iostream>
int main(){
  assert(setupApPassword([]{return uint32_t(0);})=="00000000");
  assert(setupApPassword([]{return uint32_t(123);})=="00000123");
  assert(setupApPassword([]{return uint32_t(12345678);})=="12345678");
  assert(setupApPassword([]{return uint32_t(4199999999U);})=="99999999");
  unsigned calls=0;
  assert(setupApPassword([&]{return ++calls==1?UINT32_MAX:uint32_t(42);})=="00000042");
  assert(calls==2);
  calls=0;
  assert(setupApPassword([&]{return ++calls==1?uint32_t(4200000000U):uint32_t(100000000);})=="00000000");
  assert(calls==2);
  uint32_t seed=12345;
  for(int n=0;n<100000;++n){
    const auto password=setupApPassword([&]{seed=seed*1664525U+1013904223U;return seed;});
    assert(password.size()==8 && password.find_first_not_of("0123456789")==std::string::npos);
    assert(validWifiCredentials("TingBox-test",password));
  }
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
  assert(validWifiCredentials(u8"Cửa hàng TingBox","12345678"));
  assert(!validWifiCredentials(std::string(30,'s')+u8"ệ","12345678"));
  assert(portalEscape(u8"Cửa hàng <TingBox> & 'Wi-Fi'")==u8"Cửa hàng &lt;TingBox&gt; &amp; &#39;Wi-Fi&#39;");
  assert(portalEscape("{{CSRF}}")=="{{CSRF}}"); // Literal SSID, never a template token.
  std::cout<<"PASS: 8-digit AP password, zero padding, RNG rejection, 100000 generated passwords and 16 Wi-Fi validation / HTML escaping cases\n";
}