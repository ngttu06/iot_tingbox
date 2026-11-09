#include "AudioManager.h"
#include "AudioAssets.h"
#include "VietnameseMoney.h"
#include "HardwarePins.h"

void AudioManager::begin(){
  output.SetPinout(AUDIO_BCLK,AUDIO_LRC,AUDIO_DIN);output.SetGain(0.45);
  Serial.printf("Audio config: BCLK=%d, LRC=%d, DIN=%d, gain=0.45\n",AUDIO_BCLK,AUDIO_LRC,AUDIO_DIN);
}
bool AudioManager::playAmount(uint64_t amount){
  return playSequence(buildPaymentAnnouncementClips(amount));
}
bool AudioManager::playClip(const char* id){
  if(!id)return false;
  return playSequence({id});
}
bool AudioManager::playSequence(const std::vector<std::string>& sequence){
  if(playing){Serial.println("Audio rejected: busy");return false;}
  completed=false;
  if(sequence.empty())return false;
  std::vector<const AudioClip*> clips;
  for(const auto& id:sequence){
    const auto* clip=findAudioClip(id.c_str());
    if(!clip){Serial.printf("Audio missing clip: %s\n",id.c_str());return false;}
    clips.push_back(clip);
  }
  decoder.reset();source.reset();
  source.reset(new AudioFileSourceClipSequence(std::move(clips)));
  decoder.reset(new AudioGeneratorMP3());
  if(!decoder->begin(source.get(),&output)){
    Serial.println("Audio start failed: MP3/I2S initialization");
    output.stop();decoder.reset();source.reset();return false;
  }
  Serial.printf("Audio started: %s, clips=%u, bytes=%lu\n",sequence.front().c_str(),(unsigned)sequence.size(),(unsigned long)source->getSize());
  playing=true;return true;
}
void AudioManager::loop(){
  if(!playing)return;
  if(decoder && decoder->isRunning()){
    if(decoder->loop())return;
    // Normal EOF leaves the decoder running; internal fatal errors stop it.
    completed=decoder->isRunning() && source->getPos()==source->getSize();
    Serial.printf("Audio finished: %s, bytes=%lu/%lu\n",completed?"OK":"FAILED",(unsigned long)source->getPos(),(unsigned long)source->getSize());
    decoder->stop();
  }else{completed=false;Serial.println("Audio failed: decoder not running");}
  playing=false;decoder.reset();source.reset();
}