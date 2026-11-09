#include "../SmartPayESP32/SpeakerLedger.h"
#include <cassert>
#include <iostream>
#include <map>

int main(){
  using L=SpeakerLedger;using M=L::Match;
  std::map<size_t,L::Record> disk;
  auto save=[&](size_t i,const L::Record& r){disk[i]=r;return true;};
  auto fail=[](size_t,const L::Record&){return false;};
  L ledger;L::Record a{"event-a","BANK001",std::string(32,'a'),false,false};
  assert(ledger.match(a)==M::New);
  assert(!ledger.begin(a,fail) && ledger.size()==0);
  assert(ledger.begin(a,save) && ledger.match(a)==M::Uncertain);
  L reboot;assert(reboot.load(0,disk.at(0)) && reboot.pending());
  assert(!reboot.begin(a,save)); // Reset before/during/after sound cannot replay.
  assert(!ledger.complete(a.id,fail) && ledger.match(a)==M::Uncertain);
  assert(ledger.complete(a.id,save) && ledger.match(a)==M::Played);
  // Lost ACK + reboot: IDs remain played; no new playback.
  L restored;assert(restored.load(0,disk.at(0)) && restored.match(a)==M::Played);
  auto duplicate=a;duplicate.id="new-event-id";
  assert(restored.match(duplicate)==M::Played);
  assert(!restored.rememberDuplicate(duplicate,fail));
  assert(restored.rememberDuplicate(duplicate,save));
  duplicate.scope=std::string(32,'b');
  assert(restored.match(duplicate)==M::Played); // Aliased ID survives scope change.
  auto other=a;other.id="other";other.scope=std::string(32,'b');
  assert(restored.match(other)==M::New); // Reference is scoped to a link.
  other=a;other.id="test";other.test=true;assert(restored.match(other)==M::New);
  L testLedger;std::map<size_t,L::Record> testDisk;
  auto testSave=[&](size_t i,const L::Record& r){testDisk[i]=r;return true;};
  assert(testLedger.begin(other,testSave));
  assert(testLedger.match(other)==M::Uncertain);
  assert(testLedger.complete(other.id,testSave));
  assert(testLedger.match(other)==M::Played); // Poll retry must only ACK.
  auto real=other;real.id="real-money";real.test=false;
  assert(testLedger.match(real)==M::New); // Test ref cannot suppress real money.
  auto testAgain=other;testAgain.id="different-test-id";
  assert(testLedger.match(testAgain)==M::New); // Distinct button clicks each play.
  other=a;other.id="empty-ref";other.reference="";assert(restored.match(other)==M::New);
  assert(!restored.load(3,a));
  L bad;auto invalid=a;invalid.id="";assert(!bad.load(0,invalid));
  assert(!bad.load(1,a));assert(bad.load(0,a));assert(!bad.load(1,a));
  L full;
  for(size_t i=0;i<L::capacity;++i){auto r=a;r.id=std::to_string(i);r.reference="";r.done=true;assert(full.load(i,r));}
  assert(!full.begin(other,save) && full.size()==L::capacity);
  uint64_t amount=0;
  for(const auto* text:{"1","150000","150000.00","999999999999","0001.000"})assert(speakerVndAmount(text,amount));
  assert(speakerVndAmount("150000.00",amount) && amount==150000);
  for(const auto* text:{"","0","0.00","-1","+1","1.5","1,000","1e3","1.",".1"," 1","1000000000000","999999999999999999999999999999999"})assert(!speakerVndAmount(text,amount));
  assert(!speakerVndAmount(std::string("10\0junk",7),amount));
  std::cout<<"PASS: durable intent/completion, failed writes, crash recovery, ID/reference dedup, alias persistence, scopes, test isolation, missing reference, full storage and exact money\n";
}