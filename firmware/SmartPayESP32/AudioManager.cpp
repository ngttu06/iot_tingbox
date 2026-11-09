#include "AudioManager.h"
#include "AudioAssets.h"
#include "VietnameseMoney.h"

void AudioManager::begin(){output.SetPinout(26,25,22);output.SetGain(0.45);}
bool AudioManager::playAmount(uint64_t amount){
  if(playing)return false;
  sequence=numberToVietnameseClips(amount);
  if(sequence.empty())return false;
  sequence.insert(sequence.begin(),"da_nhan");sequence.emplace_back("dong");
  for(const auto& id:sequence)if(!findAudioClip(id.c_str()))return false;
  position=0;playing=true;completed=false;next();return playing;
}
void AudioManager::next(){
  decoder.reset();source.reset();
  if(position>=sequence.size()){playing=false;completed=true;return;}
  const AudioClip* clip=findAudioClip(sequence[position++].c_str());
  if(!clip){playing=false;completed=false;return;}
  source.reset(new AudioFileSourcePROGMEM(clip->data,clip->size));
  decoder.reset(new AudioGeneratorMP3());
  if(!decoder->begin(source.get(),&output)){playing=false;completed=false;}
}
void AudioManager::loop(){
  if(!playing)return;
  if(decoder && decoder->isRunning()){
    if(!decoder->loop()){decoder->stop();next();}
  }else{playing=false;completed=false;}
}