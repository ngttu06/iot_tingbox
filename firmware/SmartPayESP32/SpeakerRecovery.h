#pragma once
#include "SpeakerLedger.h"

struct SpeakerReplayRequest {
  char id[129]{};
  uint64_t amount=0; // Nonzero only for explicit, operator-confirmed local recovery.
};

inline bool speakerReplayRequest(const std::string& text,SpeakerReplayRequest& out){
  const std::string prefix="bank-replay ",localPrefix="bank-replay-amount ";
  SpeakerReplayRequest parsed{};std::string id;
  if(text.compare(0,localPrefix.size(),localPrefix)==0){
    const auto split=text.find(' ',localPrefix.size());
    if(split==std::string::npos || !speakerVndAmount(text.substr(split+1),parsed.amount))return false;
    id=text.substr(localPrefix.size(),split-localPrefix.size());
  }else if(text.compare(0,prefix.size(),prefix)==0)id=text.substr(prefix.size());
  else return false;
  if(id.empty() || id.size()>128 || id.find_first_of(" \t\r\n")!=std::string::npos || id.find('\0')!=std::string::npos)return false;
  id.copy(parsed.id,id.size());out=parsed;return true;
}

// Volatile, operator-authorized retry. Never erase or mark the pending record
// complete merely to unblock polling, and never authorize another event.
class SpeakerRecovery {
 public:
  bool authorize(const SpeakerLedger::Record* pending,const std::string& id){
    if(!pending || pending->done || pending->test || pending->id!=id)return false;
    record=*pending;armed=true;return true;
  }
  bool active() const{return armed;}
  bool permits(const SpeakerLedger::Record& event) const{
    return armed && event.id==record.id && event.reference==record.reference &&
      event.scope==record.scope && event.test==record.test && !event.done;
  }
  void clear(){armed=false;}
 private:
  SpeakerLedger::Record record;
  bool armed=false;
};