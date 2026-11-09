#pragma once
#include <Arduino.h>
#include "AudioBackend.h"
#if AUDIO_USE_DFPLAYER
#include "DfPlayerPlayback.h"
#include "DfPlayerProtocol.h"
#else
#include "AudioFileSourceClipSequence.h"
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <memory>
#endif
#include <vector>
#include <string>

class AudioManager {
 public:
  void begin();
  bool playAmount(uint64_t amount);
  bool playNotification(uint64_t amount,bool test);
  bool playClip(const char* id);
  bool playDiagnosticSequence(){return playSequence({"chao_mung","ket_noi_wifi","chao_mung"});}
  void loop();
  bool busy() const {
#if AUDIO_USE_DFPLAYER
    return player.busy();
#else
    return playing;
#endif
  }
  bool succeeded() const {
#if AUDIO_USE_DFPLAYER
    return player.succeeded();
#else
    return completed;
#endif
  }
 private:
  bool playSequence(const std::vector<std::string>& sequence);
#if AUDIO_USE_DFPLAYER
  HardwareSerial uart{1};
  DfPlayerPlayback player;
  DfPlayerParser parser;
  int lastBusyLevel=-1;
#else
  AudioOutputI2S output;
  std::unique_ptr<AudioFileSourceClipSequence> source;
  std::unique_ptr<AudioGeneratorMP3> decoder;
  bool playing=false,completed=false;
#endif
};