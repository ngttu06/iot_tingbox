#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Never evict played IDs automatically. Full/corrupt storage stops playback.
class SpeakerLedger {
 public:
  static constexpr size_t capacity=128;
  struct Record {std::string id,reference,scope;bool test=false,done=false;};
  enum class Match { New,Played,Uncertain };
  using Writer=std::function<bool(size_t,const Record&)>;
  static bool valid(const Record& r){
    return !r.id.empty() && r.id.size()<=128 && r.reference.size()<=256 && r.scope.size()==32;
  }
  bool load(size_t slot,const Record& r){
    if(slot!=records.size() || slot>=capacity || !valid(r))return false;
    for(const auto& old:records)if(old.id==r.id)return false;
    records.push_back(r);return true;
  }
  Match match(const Record& event) const{
    for(const auto& r:records){
      if(r.id==event.id || (!event.test && !r.test && !event.reference.empty() &&
          r.scope==event.scope && r.reference==event.reference))return r.done?Match::Played:Match::Uncertain;
    }
    return Match::New;
  }
  bool begin(const Record& event,const Writer& write){
    if(!valid(event) || pending() || records.size()>=capacity || match(event)!=Match::New)return false;
    Record r=event;r.done=false;
    if(!write(records.size(),r))return false;
    records.push_back(r);return true;
  }
  bool complete(const std::string& id,const Writer& write){
    for(size_t i=0;i<records.size();++i)if(records[i].id==id){
      if(records[i].done)return true;
      Record r=records[i];r.done=true;
      if(!write(i,r))return false;
      records[i]=r;return true;
    }
    return false;
  }
  bool rememberDuplicate(const Record& event,const Writer& write){
    if(!valid(event) || match(event)!=Match::Played)return false;
    for(const auto& r:records)if(r.id==event.id)return true;
    if(pending() || records.size()>=capacity)return false;
    Record r=event;r.done=true;
    if(!write(records.size(),r))return false;
    records.push_back(r);return true;
  }
  const Record* pending() const{for(const auto& r:records)if(!r.done)return &r;return nullptr;}
  size_t size() const{return records.size();}
 private:
  std::vector<Record> records;
};

// Exact decimal input. VND fractions must be zero; never round money via float.
inline bool speakerVndAmount(const std::string& text,uint64_t& value){
  if(text.empty() || text.size()>32)return false;
  uint64_t n=0;size_t i=0;
  for(;i<text.size() && text[i]>='0' && text[i]<='9';++i){
    n=n*10+unsigned(text[i]-'0');if(n>999999999999ULL)return false;
  }
  if(!i)return false;
  if(i<text.size()){
    if(text[i++]!='.' || i==text.size())return false;
    for(;i<text.size();++i)if(text[i]!='0')return false;
  }
  if(!n)return false;
  value=n;return true;
}