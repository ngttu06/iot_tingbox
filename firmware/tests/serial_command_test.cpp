#include "../SmartPayESP32/SerialCommand.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

int main(){
  SerialCommand input;
  for(char c:std::string("status"))assert(!input.feed(c));
  assert(!std::strcmp(input.feed('\r'),"status"));
  assert(!input.feed('\n'));
  assert(!input.feed('\n'));
  for(char c:std::string("audio-test"))assert(!input.feed(c));
  assert(!std::strcmp(input.feed('\n'),"audio-test"));
  const std::string replay="bank-replay 830784fa-d648-4c3b-ba85-4ca67f22afe4";
  for(char c:replay)assert(!input.feed(c));
  assert(replay==input.feed('\n'));
  const std::string localReplay="bank-replay-amount "+std::string(128,'x')+" 999999999999";
  for(char c:localReplay)assert(!input.feed(c));
  assert(localReplay==input.feed('\n'));
  for(int i=0;i<191;++i)assert(!input.feed('a'));
  const char* full=input.feed('\n');assert(full && std::strlen(full)==191);
  for(int i=0;i<1000;++i)assert(!input.feed('b'));
  assert(!input.feed('\n')); // Never execute a truncated command.
  for(char c:std::string("help"))assert(!input.feed(c));
  assert(!std::strcmp(input.feed('\n'),"help"));
  std::cout<<"PASS: serial commands, CRLF, empty lines, bounded input and overflow recovery\n";
}