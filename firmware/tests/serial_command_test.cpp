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
  for(int i=0;i<31;++i)assert(!input.feed('a'));
  const char* full=input.feed('\n');assert(full && std::strlen(full)==31);
  for(int i=0;i<1000;++i)assert(!input.feed('b'));
  assert(!input.feed('\n')); // Never execute a truncated command.
  for(char c:std::string("help"))assert(!input.feed(c));
  assert(!std::strcmp(input.feed('\n'),"help"));
  std::cout<<"PASS: serial commands, CRLF, empty lines, bounded input and overflow recovery\n";
}