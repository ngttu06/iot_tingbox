#include "../SmartPayESP32/VietnameseMoney.h"
#include <cassert>
#include <iostream>

std::string words(uint64_t n) {
  std::string out;
  for(const auto& w:numberToVietnameseClips(n)){if(!out.empty())out+=' ';out+=w;}
  return out;
}
int main(){
  assert(words(0)=="khong");
  assert(words(10)=="muoi");
  assert(words(11)=="muoi mot");
  assert(words(15)=="muoi lam");
  assert(words(10000)=="muoi nghin");
  assert(words(1000)=="mot nghin");
  assert(words(1001)=="mot nghin khong tram linh mot");
  assert(words(1005)=="mot nghin khong tram linh nam");
  assert(words(1010)=="mot nghin khong tram muoi");
  assert(words(1025)=="mot nghin khong tram hai muoi_hang_chuc lam");
  assert(words(10015)=="muoi nghin khong tram muoi lam");
  assert(words(21000)=="hai muoi_hang_chuc mot_sau_muoi nghin");
  assert(words(24000)=="hai muoi_hang_chuc bon nghin");
  assert(words(25000)=="hai muoi_hang_chuc lam nghin");
  assert(words(105000)=="mot tram linh nam nghin");
  assert(words(125000)=="mot tram hai muoi_hang_chuc lam nghin");
  assert(words(150000)=="mot tram nam muoi_hang_chuc nghin");
  assert(words(1000000)=="mot trieu");
  assert(words(1000001)=="mot trieu khong tram linh mot");
  assert(words(1205000)=="mot trieu hai tram linh nam nghin");
  assert(words(1000000000)=="mot ty");
  assert(!words(999999999999ULL).empty());
  assert(words(1000000000000ULL).empty());
  for(uint64_t i=0;i<100000;i++) assert(!numberToVietnameseClips(i).empty());
  assert((buildPaymentAnnouncementClips(10000)==std::vector<std::string>{
    "cam_on","da_nhan_duoc","muoi","nghin","viet_nam_dong"}));
  for(uint64_t n:{0ULL,15ULL,1001ULL,150000ULL,21000ULL,999999999999ULL}){
    auto clips=buildPaymentAnnouncementClips(n);
    auto number=numberToVietnameseClips(n);
    assert(clips.size()==number.size()+3);
    assert(clips[0]=="cam_on" && clips[1]=="da_nhan_duoc");
    assert(clips.back()=="viet_nam_dong");
    assert(std::vector<std::string>(clips.begin()+2,clips.end()-1)==number);
  }
  assert(buildPaymentAnnouncementClips(1000000000000ULL).empty());
  assert(buildPaymentAnnouncementClips(UINT64_MAX).empty());
  std::cout<<"Money and payment announcement cases + 100000 range cases passed\n";
}