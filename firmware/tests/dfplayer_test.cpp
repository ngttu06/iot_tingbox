#include "../SmartPayESP32/DfPlayerPlayback.h"
#include "../SmartPayESP32/DfPlayerProtocol.h"
#include "../SmartPayESP32/DfPlayerTracks.h"
#include "../SmartPayESP32/VietnameseMoney.h"
#include <cassert>
#include <iostream>

static uint32_t ready(DfPlayerPlayback& p,uint32_t now=0){
  p.begin(now);assert(p.tick(now,false).code==0x0c);
  assert(!p.tick(now+2499,false).code);
  assert(p.tick(now+2500,false).code==0x09);
  assert(p.tick(now+2750,false).code==0x06);
  assert(p.tick(now+2900,false).code==0x19);
  assert(p.tick(now+3050,false).code==0x16);
  assert(!p.tick(now+3300,false).code);
  return now+3300;
}
static void finish(DfPlayerPlayback& p,uint32_t now){
  p.tick(now+10,true);p.tick(now+30,true); // LOW stable
  p.tick(now+100,false);p.tick(now+150,false); // HIGH stable
}
int main(){
  const auto frame=dfPlayerFrame(0x12,1);
  const std::array<uint8_t,10> expected{{0x7e,0xff,6,0x12,0,0,1,0xfe,0xe8,0xef}};
  assert(frame==expected);
  DfPlayerParser parser;
  assert(!parser.feed(0x00));
  auto bad=frame;bad[8]^=1;
  for(auto byte:bad)assert(!parser.feed(byte));
  for(unsigned i=0;i<9;++i)assert(!parser.feed(frame[i]));
  assert(parser.feed(frame[9]) && parser.command==0x12 && parser.argument==1);
  assert(!parser.feed(0x7e));assert(!parser.feed(0x7e));
  for(unsigned i=1;i<9;++i)assert(!parser.feed(frame[i]));
  assert(parser.feed(frame[9]));
  for(const auto& track:DFPLAYER_TRACKS)assert(findDfPlayerTrack(track.id)==track.number);
  assert(!findDfPlayerTrack(nullptr) && !findDfPlayerTrack("missing"));
  for(uint64_t amount:{1ULL,10000ULL,150000ULL,999999999999ULL})
    for(const auto& id:buildPaymentAnnouncementClips(amount))assert(findDfPlayerTrack(id.c_str()));

  DfPlayerPlayback p;uint32_t t=ready(p);
  assert(!p.start({}) && !p.start({0}) && !p.start({10000}));
  assert(p.start({1,2}) && !p.start({3}));
  assert(p.tick(t,false).argument==1);finish(p,t);
  assert(p.busy() && !p.succeeded());
  assert(!p.tick(t+299,false).code);
  assert(p.tick(t+300,false).argument==2);finish(p,t+300);
  assert(!p.busy() && p.succeeded());

  t=ready(p);assert(p.start({1}));p.tick(t,false);
  p.tick(t+3000,false);assert(!p.busy() && !p.succeeded() && p.error());
  assert(p.tick(t+3001,false).code==0x16);
  assert(p.start({1}));p.tick(t+3200,false);finish(p,t+3200);
  assert(p.succeeded()); // Retry after missing/disconnected BUSY.

  t=ready(p);assert(p.start({1}));p.tick(t,false);
  p.tick(t+1,true);p.tick(t+21,true);p.tick(t+15021,true);
  assert(!p.busy() && !p.succeeded()); // Stuck LOW must never ACK.
  t=ready(p);assert(p.start({1}));p.tick(t,true);
  assert(!p.busy() && !p.succeeded()); // LOW before play rejected.

  t=ready(p);assert(p.start({1}));p.tick(t,false);
  p.tick(t+1,true);p.tick(t+10,false);p.tick(t+20,true);p.tick(t+40,true);
  p.tick(t+60,false);p.tick(t+80,true);p.tick(t+100,false);
  p.tick(t+149,false);assert(p.busy());p.tick(t+150,false);assert(p.succeeded());

  t=ready(p);assert(p.start({1}));p.tick(t,false);p.fail("SD removed");
  assert(!p.busy() && !p.succeeded() && p.tick(t+1,false).code==0x16);
  p.begin(0);assert(p.start({1}));p.fail("boot error");
  assert(!p.succeeded());p.tick(1,false); // Pending stop, then boot continues.

  t=ready(p,UINT32_MAX-2000);assert(p.start({24}));
  assert(p.tick(t,false).argument==24);finish(p,t);assert(p.succeeded());
  std::cout<<"PASS: DFPlayer UART/checksum/parser/map, sequence, BUSY debounce, failures, retry and timer wrap\n";
}