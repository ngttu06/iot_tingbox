#pragma once
#include <array>
#include <cstdint>

inline std::array<uint8_t,10> dfPlayerFrame(uint8_t command,uint16_t argument){
  std::array<uint8_t,10> frame{{0x7e,0xff,0x06,command,0x00,
    uint8_t(argument>>8),uint8_t(argument),0,0,0xef}};
  uint16_t sum=0;
  for(unsigned i=1;i<=6;++i)sum+=frame[i];
  const uint16_t checksum=uint16_t(0-sum);
  frame[7]=uint8_t(checksum>>8);frame[8]=uint8_t(checksum);
  return frame;
}

class DfPlayerParser {
 public:
  bool feed(uint8_t byte){
    if(!length && byte!=0x7e)return false;
    frame[length++]=byte;
    if((length==2 && byte!=0xff) || (length==3 && byte!=0x06)){
      length=byte==0x7e?1:0;return false;
    }
    if(length!=frame.size())return false;
    length=0;
    uint16_t sum=0;for(unsigned i=1;i<=6;++i)sum+=frame[i];
    sum+=uint16_t(frame[7])<<8|frame[8];
    if(frame[9]!=0xef || sum!=0){
      if(byte==0x7e){frame[0]=byte;length=1;}
      return false;
    }
    command=frame[3];argument=uint16_t(frame[5])<<8|frame[6];return true;
  }
  uint8_t command=0;
  uint16_t argument=0;
 private:
  std::array<uint8_t,10> frame{};
  unsigned length=0;
};