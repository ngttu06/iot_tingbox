#pragma once
#include <Arduino.h>
#include "AudioFileSourceClipSequence.h"
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <memory>
#include <vector>
#include <string>

class AudioManager {
 public:
  void begin();
  bool playAmount(uint64_t amount);
  bool playClip(const char* id);
  void loop();
  bool busy() const { return playing; }
  bool succeeded() const { return completed; }
 private:
  bool playSequence(const std::vector<std::string>& sequence);
  AudioOutputI2S output;
  std::unique_ptr<AudioFileSourceClipSequence> source;
  std::unique_ptr<AudioGeneratorMP3> decoder;
  bool playing=false,completed=false;
};