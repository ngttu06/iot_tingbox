#include "AudioManager.h"
#include "AudioAssets.h"
#include "VietnameseMoney.h"

void AudioManager::begin(){output.SetPinout(26,25,22);output.SetGain(0.45);}
bool AudioManager::playAmount(uint64_t amount){
  if(playing)return false;
  completed=false;
  const auto sequence=buildPaymentAnnouncementClips(amount);
  if(sequence.empty())return false;
  std::vector<const AudioClip*> clips;
  for(const auto& id:sequence){
    const auto* clip=findAudioClip(id.c_str());
    if(!clip)return false;
    clips.push_back(clip);
  }
  decoder.reset();source.reset();
  source.reset(new AudioFileSourceClipSequence(std::move(clips)));
  decoder.reset(new AudioGeneratorMP3());
  if(!decoder->begin(source.get(),&output)){
    decoder.reset();source.reset();return false;
  }
  playing=true;return true;
}
void AudioManager::loop(){
  if(!playing)return;
  if(decoder && decoder->isRunning()){
    if(decoder->loop())return;
    // Normal EOF leaves the decoder running; internal fatal errors stop it.
    completed=decoder->isRunning() && source->getPos()==source->getSize();
    decoder->stop();
  }else completed=false;
  playing=false;decoder.reset();source.reset();
}