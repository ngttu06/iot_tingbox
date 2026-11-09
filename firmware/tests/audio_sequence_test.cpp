#include "../SmartPayESP32/AudioFileSourceClipSequence.h"
#include "../SmartPayESP32/VietnameseMoney.h"
#include <cassert>
#include <iostream>

int main(){
  const unsigned char a[]={1,2,3},b[]={4,5};
  const AudioClip first={"a",a,sizeof(a)},second={"b",b,sizeof(b)};
  AudioFileSourceClipSequence source({&first,&second,&first});
  unsigned char buffer[16]={};
  assert(source.isOpen() && source.getSize()==8);
  assert(source.read(buffer,4)==4 && buffer[3]==4);
  assert(source.read(buffer,16)==4 && buffer[0]==5 && buffer[3]==3);
  assert(source.read(buffer,1)==0 && source.getPos()==8);
  assert(source.seek(-4,SEEK_END) && source.getPos()==4);
  assert(source.read(buffer,2)==2 && buffer[0]==5 && buffer[1]==1);
  assert(source.seek(-6,SEEK_CUR) && source.getPos()==0);
  assert(!source.seek(-1,SEEK_SET) && !source.seek(9,SEEK_SET));
  assert(!source.seek(0,12345));
  assert(source.seek(3,SEEK_SET) && source.read(buffer,1)==1 && buffer[0]==4);
  assert(source.seek(8,SEEK_SET) && source.read(buffer,1)==0);
  assert(source.close() && !source.isOpen() && source.read(buffer,1)==0);
  assert(!source.seek(0,SEEK_SET));
  AudioFileSourceClipSequence empty({}),invalid({nullptr});
  assert(!empty.isOpen() && !invalid.isOpen());
  for(uint64_t amount:{0ULL,10000ULL,150000ULL,21000ULL,999999999999ULL}){
    std::vector<const AudioClip*> clips;
    std::vector<unsigned char> expected;
    for(const auto& id:buildPaymentAnnouncementClips(amount)){
      const auto* clip=findAudioClip(id.c_str());assert(clip);
      clips.push_back(clip);
      expected.insert(expected.end(),clip->data,clip->data+clip->size);
    }
    for(unsigned block:{1U,7U,1024U,4096U}){
      AudioFileSourceClipSequence stream(clips);
      std::vector<unsigned char> actual,chunk(block);
      while(auto n=stream.read(chunk.data(),block)){
        actual.insert(actual.end(),chunk.begin(),chunk.begin()+n);
      }
      assert(actual==expected && stream.getPos()==expected.size());
    }
  }
  std::cout<<"PASS: Flash clip sequence reads, seek, EOF and announcement assets\n";
}