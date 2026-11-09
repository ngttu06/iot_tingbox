#pragma once
#include <Arduino.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <memory>
#include <vector>
#include <string>

class AudioManager {
 public:
  void begin();
  bool playAmount(uint64_t amount);
  void loop();
  bool busy() const { return playing; }
  bool succeeded() const { return completed; }
 private:
  AudioOutputI2S output;
  std::unique_ptr<AudioFileSourcePROGMEM> source;
  std::unique_ptr<AudioGeneratorMP3> decoder;
  std::vector<std::string> sequence;
  size_t position=0;
  bool playing=false,completed=false;
  void next();
};