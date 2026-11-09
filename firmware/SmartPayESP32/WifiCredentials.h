#pragma once
#include <cstdint>
#include <string>

// Eight decimal digits, including leading zeroes. Rejection sampling avoids
// modulo bias; the device supplies esp_random(), tests supply deterministic RNG.
template<typename Random>
inline std::string setupApPassword(Random random){
  uint32_t value;
  do{value=random();}while(value>=4200000000U);
  value%=100000000U;
  std::string password(8,'0');
  for(int i=7;i>=0;--i){password[i]=char('0'+value%10);value/=10;}
  return password;
}

inline bool validWifiCredentials(const std::string& ssid,const std::string& password){
  if(ssid.empty() || ssid.size()>32 || ssid.find('\0')!=std::string::npos || password.find('\0')!=std::string::npos)return false;
  if(password.empty())return true; // Explicit support for open networks.
  if(password.size()<8 || password.size()>63)return false;
  for(unsigned char c:password)if(c<32 || c>126)return false;
  return true;
}
inline std::string portalEscape(const std::string& value){
  std::string out;
  for(char c:value)switch(c){
    case '&':out+="&amp;";break;case '<':out+="&lt;";break;
    case '>':out+="&gt;";break;case '"':out+="&quot;";break;
    case '\'':out+="&#39;";break;default:out+=c;
  }
  return out;
}