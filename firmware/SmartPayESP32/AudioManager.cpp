#include "AudioManager.h"
#include "AudioAssets.h"
#include "VietnameseMoney.h"
#include "HardwarePins.h"
// Safe fallback with existing SD/Flash assets: test the speaker using a welcome
// phrase, NEVER claim receipt of the test amount as real money.
bool AudioManager::playNotification(uint64_t amount,bool test){
  return test?playClip("chao_mung"):playAmount(amount);
}
#if AUDIO_USE_DFPLAYER
#include "DfPlayerTracks.h"

void AudioManager::begin(){
  pinMode(DFPLAYER_BUSY,INPUT_PULLUP);
  uart.begin(9600,SERIAL_8N1,DFPLAYER_RX,DFPLAYER_TX);
  player.begin(millis());
  Serial.printf("Audio config: DFPlayer UART1 9600, ESP_RX=%d, ESP_TX=%d, BUSY=%d, volume=20/30\n",DFPLAYER_RX,DFPLAYER_TX,DFPLAYER_BUSY);
}
bool AudioManager::playAmount(uint64_t amount){return playSequence(buildPaymentAnnouncementClips(amount));}
bool AudioManager::playClip(const char* id){return id && playSequence({id});}
bool AudioManager::playSequence(const std::vector<std::string>& sequence){
  if(player.busy() || sequence.empty())return false;
  std::vector<uint16_t> tracks;
  for(const auto& id:sequence){
    const auto track=findDfPlayerTrack(id.c_str());
    if(!track){Serial.printf("DFPlayer missing clip mapping: %s\n",id.c_str());return false;}
    tracks.push_back(track);
  }
  if(!player.start(std::move(tracks)))return false;
  Serial.printf("Audio queued: DFPlayer %s, clips=%u\n",sequence.front().c_str(),(unsigned)sequence.size());
  return true;
}
void AudioManager::loop(){
  const bool wasBusy=player.busy();
  for(unsigned n=0;n<64 && uart.available();++n){
    if(!parser.feed((uint8_t)uart.read()))continue;
    Serial.printf("DFPlayer response: command=0x%02x, value=%u\n",parser.command,parser.argument);
    if(parser.command==0x40 || (parser.command==0x3b && (parser.argument&2)))player.fail("module error or SD removed");
  }
  const int busyLevel=digitalRead(DFPLAYER_BUSY);
  if(player.busy() && busyLevel!=lastBusyLevel){
    Serial.printf("DFPlayer BUSY: t=%lums, level=%s\n",millis(),busyLevel==LOW?"LOW":"HIGH");
  }
  lastBusyLevel=busyLevel;
  const auto command=player.tick(millis(),busyLevel==LOW);
  if(command.code){
    const auto frame=dfPlayerFrame(command.code,command.argument);
    if(uart.write(frame.data(),frame.size())!=frame.size())player.fail("UART write failed");
    if(command.code==0x12)Serial.printf("DFPlayer play: /mp3/%04u.mp3, t=%lums\n",command.argument,millis());
  }
  if(wasBusy && !player.busy()){
    if(player.succeeded())Serial.println("Audio finished: OK (DFPlayer BUSY cycle, not an acoustic measurement)");
    else Serial.printf("Audio finished: FAILED (%s)\n",player.error()?player.error():"unknown");
  }
}
#else

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
#endif