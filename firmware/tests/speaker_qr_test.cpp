#include "../SmartPayESP32/src/qr/qrcode.h"
#include "../SmartPayESP32/SpeakerLinkQr.h"
#include <cassert>
#include <fstream>
#include <vector>

int main(int argc,char** argv){
  assert(argc==3);
  const char* key="SPK-b8fb059c-d258-4926-b51a-bd6e7d90120f";
  QRCode qr;std::vector<uint8_t> data(qrcode_getBufferSize(4));
  assert(qrcode_initText(&qr,data.data(),4,ECC_MEDIUM,key)==0);
  const int scale=5,side=(qr.size+8)*scale;
  std::ofstream out(argv[1],std::ios::binary);out<<"P5\n"<<side<<" "<<side<<"\n255\n";
  for(int y=0;y<side;++y)for(int x=0;x<side;++x){
    const int gx=x/scale-4,gy=y/scale-4;
    unsigned char c=(gx>=0 && gy>=0 && gx<qr.size && gy<qr.size && qrcode_getModule(&qr,gx,gy))?0:255;
    out.write(reinterpret_cast<const char*>(&c),1);
  }
  assert(out.good());
  const std::string svg=speakerLinkSvg(key);
  assert(!svg.empty() && svg.find("viewBox=\"0 0 41 41\"")!=std::string::npos);
  assert(speakerLinkSvg("<script>").empty());
  assert(speakerLinkSvg("SPK-b8fb059c-d258-4926-b51a-bd6e7d90120<").empty());
  std::ofstream svgFile(argv[2]);svgFile<<svg;assert(svgFile.good());
}