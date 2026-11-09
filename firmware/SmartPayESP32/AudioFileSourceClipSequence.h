#pragma once
#include <AudioFileSource.h>
#include "AudioAssets.h"
#include <cstdio>
#include <vector>
#include <utility>

// Read Flash directly. Only clip pointers, not the full MP3, occupy RAM.
class AudioFileSourceClipSequence : public AudioFileSource {
 public:
  explicit AudioFileSourceClipSequence(std::vector<const AudioClip*> clips)
      : clips(std::move(clips)) {
    opened=!this->clips.empty();
    for(const auto* clip:this->clips){
      if(!clip || !clip->data || !clip->size || clip->size>UINT32_MAX-size){
        opened=false;size=0;return;
      }
      size+=clip->size;
    }
  }
  uint32_t read(void* data,uint32_t len) override {
    if(!opened || !data)return 0;
    uint32_t count=0;
    auto* dest=static_cast<uint8_t*>(data);
    while(count<len && index<clips.size()){
      const auto* clip=clips[index];
      uint32_t chunk=clip->size-offset;
      if(chunk>len-count)chunk=len-count;
      memcpy_P(dest+count,clip->data+offset,chunk);
      count+=chunk;position+=chunk;offset+=chunk;
      if(offset==clip->size){++index;offset=0;}
    }
    return count;
  }
  bool seek(int32_t pos,int dir) override {
    if(!opened)return false;
    int64_t target;
    switch(dir){
      case SEEK_SET:target=pos;break;
      case SEEK_CUR:target=int64_t(position)+pos;break;
      case SEEK_END:target=int64_t(size)+pos;break;
      default:return false;
    }
    if(target<0 || target>size)return false;
    position=static_cast<uint32_t>(target);index=0;offset=position;
    while(index<clips.size() && offset>=clips[index]->size){
      offset-=clips[index]->size;++index;
    }
    return true;
  }
  bool close() override {opened=false;position=0;index=0;offset=0;return true;}
  bool isOpen() override {return opened;}
  uint32_t getSize() override {return opened?size:0;}
  uint32_t getPos() override {return opened?position:0;}
 private:
  std::vector<const AudioClip*> clips;
  size_t index=0;
  uint32_t size=0,position=0,offset=0;
  bool opened=false;
};