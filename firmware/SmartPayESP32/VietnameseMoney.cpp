#include "VietnameseMoney.h"

namespace {
const char* digits[] = {"khong","mot","hai","ba","bon","nam","sau","bay","tam","chin"};
void group(unsigned n, bool full, std::vector<std::string>& out) {
  unsigned h=n/100,t=(n/10)%10,u=n%10;
  if(h || full) {out.emplace_back(digits[h]);out.emplace_back("tram");}
  if(t>1) {out.emplace_back(digits[t]);out.emplace_back("muoi_hang_chuc");}
  else if(t==1) out.emplace_back("muoi");
  else if(u && (h || full)) out.emplace_back("linh");
  if(u) out.emplace_back(u==1 && t>1 ? "mot_sau_muoi" : u==5 && t>0 ? "lam" : digits[u]);
}
}
std::vector<std::string> numberToVietnameseClips(uint64_t amount) {
  std::vector<std::string> out;
  if(amount>999999999999ULL) return out;
  if(!amount) return {"khong"};
  const char* units[]={"","nghin","trieu","ty"};
  unsigned g[4]={};
  for(int i=0;i<4;i++){g[i]=amount%1000;amount/=1000;}
  bool started=false;
  for(int i=3;i>=0;i--) if(g[i]) {
    group(g[i],started && g[i]<100,out);
    if(i) out.emplace_back(units[i]);
    started=true;
  }
  return out;
}

std::vector<std::string> buildPaymentAnnouncementClips(uint64_t amount) {
  auto clips=numberToVietnameseClips(amount);
  if(clips.empty())return clips;
  clips.insert(clips.begin(),{"cam_on","da_nhan_duoc"});
  clips.emplace_back("viet_nam_dong");
  return clips;
}