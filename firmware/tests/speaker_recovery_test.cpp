#include "../SmartPayESP32/SpeakerRecovery.h"
#include <cassert>
#include <iostream>
int main(){
  SpeakerLedger ledger;SpeakerLedger::Record p{"pending","BANKREF",std::string(32,'a'),false,false};
  auto save=[](size_t,const SpeakerLedger::Record&){return true;};
  assert(ledger.begin(p,save));SpeakerRecovery auth;
  assert(!auth.authorize(nullptr,p.id));assert(!auth.authorize(ledger.pending(),"wrong"));
  assert(!auth.active() && ledger.match(p)==SpeakerLedger::Match::Uncertain);
  assert(auth.authorize(ledger.pending(),p.id) && auth.permits(p));
  auto other=p;other.id="another";assert(!auth.permits(other));
  other=p;other.reference="changed";assert(!auth.permits(other));
  other=p;other.scope=std::string(32,'b');assert(!auth.permits(other));
  other=p;other.test=true;assert(!auth.permits(other));
  assert(ledger.match(p)==SpeakerLedger::Match::Uncertain); // Authorization is not completion/ACK.
  SpeakerReplayRequest request{};
  assert(speakerReplayRequest("bank-replay pending",request) && std::string(request.id)==p.id && !request.amount);
  assert(speakerReplayRequest("bank-replay-amount pending 2000",request) && request.amount==2000 && std::string(request.id)==p.id);
  for(const auto& bad:{"bank-replay-amount pending", "bank-replay-amount pending 0", "bank-replay-amount pending 2.1", "bank-replay-amount pending 2 extra", "bank-replay-amount pending -2", "bank-replay-amount  2000", "bank-replay-amount pending 1000000000000", "bank-replay bad id"}){
    assert(!speakerReplayRequest(bad,request));
    assert(request.amount==2000 && std::string(request.id)==p.id); // Failed parse never modifies a request.
  }
  assert(!speakerReplayRequest("bank-replay "+std::string(129,'x'),request));
  assert(speakerReplayRequest("bank-replay-amount "+std::string(128,'x')+" 999999999999",request));
  assert(ledger.match(p)==SpeakerLedger::Match::Uncertain); // Local approval still is NOT completion.
  auto fail=[](size_t,const SpeakerLedger::Record&){return false;};
  assert(!ledger.complete(p.id,fail) && ledger.pending()); // Failed persistence must never authorize ACK.
  SpeakerRecovery reboot;assert(!reboot.active()); // Never persisted/reused after reset.
  auth.clear();assert(!auth.permits(p));
  assert(ledger.complete(p.id,save) && ledger.match(p)==SpeakerLedger::Match::Played);
  assert(!auth.authorize(ledger.pending(),p.id));
  p.test=true;assert(!auth.authorize(&p,p.id));
  std::cout<<"PASS: explicit pending-ID replay, confirmed amount parsing, reference/scope/test guards, no early completion, failed persistence, volatile authorization and played-ID retention\n";
}