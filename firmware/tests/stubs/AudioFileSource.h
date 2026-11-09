#pragma once
#include <Arduino.h>
// Host-only interface stub. Firmware build checks the actual library interface.
class AudioFileSource {
 public:
  virtual ~AudioFileSource()=default;
  virtual uint32_t read(void*,uint32_t){return 0;}
  virtual bool seek(int32_t,int){return false;}
  virtual bool close(){return false;}
  virtual bool isOpen(){return false;}
  virtual uint32_t getSize(){return 0;}
  virtual uint32_t getPos(){return 0;}
};