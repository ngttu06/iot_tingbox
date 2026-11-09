#pragma once
#include <cstddef>

// Fixed-size, nonblocking console input. Discard an overlong line in full.
class SerialCommand {
 public:
  const char* feed(char c){
    if(c=='\r' || c=='\n'){
      const bool ready=length && !overflow;
      buffer[length]='\0';length=0;overflow=false;
      return ready?buffer:nullptr;
    }
    if(overflow)return nullptr;
    if(length==sizeof(buffer)-1){overflow=true;return nullptr;}
    buffer[length++]=c;return nullptr;
  }
 private:
  char buffer[32]{};
  size_t length=0;
  bool overflow=false;
};