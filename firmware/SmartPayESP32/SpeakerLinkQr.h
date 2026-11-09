#pragma once
#include <string>
#include <vector>
#include "src/qr/qrcode.h"

// Shared offline renderer: no CDN, external QR service, JS or key in a URL.
inline std::string speakerLinkSvg(const std::string& key){
  if(key.size()!=40 || key.compare(0,4,"SPK-")!=0)return "";
  for(size_t i=4;i<40;++i){
    if(i==12 || i==17 || i==22 || i==27){if(key[i]!='-')return "";}
    else if(!((key[i]>='0' && key[i]<='9') || (key[i]>='a' && key[i]<='f')))return "";
  }
  QRCode qr;std::vector<uint8_t> data(qrcode_getBufferSize(4));
  if(qrcode_initText(&qr,data.data(),4,ECC_MEDIUM,key.c_str())!=0)return "";
  const std::string side=std::to_string(qr.size+8);
  std::string svg="<svg id=\"bank-qr\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 "+side+" "+side+
    "\" width=\"246\" height=\"246\" role=\"img\" aria-label=\"QR liên kết ngân hàng\" shape-rendering=\"crispEdges\">"
    "<rect width=\"100%\" height=\"100%\" fill=\"white\"/><path fill=\"black\" d=\"";
  for(int y=0;y<qr.size;++y)for(int x=0;x<qr.size;++x)if(qrcode_getModule(&qr,x,y))
    svg+="M"+std::to_string(x+4)+" "+std::to_string(y+4)+"h1v1h-1z";
  return svg+="\"/></svg>";
}