#include "DisplayManager.h"
#include "src/qr/qrcode.h"
#include <vector>

// Font 2 has no Vietnamese glyph set: transliterate for display only; backend preserves original content.
static String asciiText(const String& input){
  const char* groups[]={"àáạảãâầấậẩẫăằắặẳẵÀÁẠẢÃÂẦẤẬẨẪĂẰẮẶẲẴ","èéẹẻẽêềếệểễÈÉẸẺẼÊỀẾỆỂỄ","ìíịỉĩÌÍỊỈĨ","òóọỏõôồốộổỗơờớợởỡÒÓỌỎÕÔỒỐỘỔỖƠỜỚỢỞỠ","ùúụủũưừứựửữÙÚỤỦŨƯỪỨỰỬỮ","ỳýỵỷỹỲÝỴỶỸ","đĐ"};
  const char replacements[]={'a','e','i','o','u','y','d'};
  String out;
  for(size_t i=0;i<input.length() && out.length()<160;){
    unsigned char c=input[i];size_t len=c<128?1:(c&0xE0)==0xC0?2:(c&0xF0)==0xE0?3:4;
    if(i+len>input.length())break;
    if(len==1){out+=c=='\n'?'\n':c>=32&&c<=126?char(c):' ';}
    else{String glyph=input.substring(i,i+len);bool found=false;
      for(int j=0;j<7;j++)if(String(groups[j]).indexOf(glyph)>=0){out+=replacements[j];found=true;break;}
      if(!found)out+='?';
    }
    i+=len;
  }
  return out;
}
static String money(uint64_t amount){
  char b[24];snprintf(b,sizeof(b),"%llu",(unsigned long long)amount);String s=b;
  for(int i=s.length()-3;i>0;i-=3)s=s.substring(0,i)+"."+s.substring(i);
  return s+" VND";
}
void DisplayManager::begin(){tft.init();tft.setRotation(1);tft.fillScreen(TFT_WHITE);tft.setTextFont(2);}
void DisplayManager::message(const String& title,const String& detail){
  tft.fillScreen(TFT_WHITE);tft.setTextColor(TFT_NAVY,TFT_WHITE);tft.drawString(title,12,16,4);wrapped(detail,65,8);
}
void DisplayManager::wrapped(const String& text,int y,int maxLines){
  String s=asciiText(text);
  for(int line=0;line<maxLines && s.length();line++){
    int newline=s.indexOf('\n');int n=newline>=0?newline:s.length();
    while(n>0 && tft.textWidth(s.substring(0,n),2)>tft.width()-24)n--;
    if(!n){if(newline==0){s=s.substring(1);continue;}break;}
    tft.drawString(s.substring(0,n),12,y+line*20,2);s=s.substring(n);
    if(s.startsWith("\n"))s=s.substring(1);
  }
}
bool DisplayManager::order(const String& id,uint64_t amount,const String& payload){
  QRCode qr;std::vector<uint8_t> data;bool ok=false;
  for(uint8_t version=5;version<=12;version++){
    data.resize(qrcode_getBufferSize(version));
    if(qrcode_initText(&qr,data.data(),version,ECC_LOW,payload.c_str())==0){ok=true;break;}
  }
  if(!ok){message("QR ERROR","Payload too large");return false;}
  tft.fillScreen(TFT_WHITE);
  int scale=250/(qr.size+8),side=(qr.size+8)*scale,left=(tft.width()-side)/2,top=4;
  if(scale<1)return false;
  // Four-module quiet zone around the black modules.
  for(int y=0;y<qr.size;y++)for(int x=0;x<qr.size;x++)if(qrcode_getModule(&qr,x,y))
    tft.fillRect(left+(x+4)*scale,top+(y+4)*scale,scale,scale,TFT_BLACK);
  tft.setTextColor(TFT_NAVY,TFT_WHITE);tft.drawCentreString(money(amount),tft.width()/2,255,4);
  tft.drawCentreString(id+" - Dang cho",tft.width()/2,290,2);return true;
}
void DisplayManager::credit(bool paid,uint64_t amount,const String& content,const String& bank,const String& at){
  tft.fillScreen(TFT_WHITE);tft.setTextColor(paid?TFT_DARKGREEN:TFT_NAVY,TFT_WHITE);
  tft.drawString(paid?"DA THANH TOAN":"DA NHAN TIEN",12,8,4);
  if(paid){tft.drawLine(425,25,437,38,TFT_DARKGREEN);tft.drawLine(437,38,465,10,TFT_DARKGREEN);}
  tft.drawString("+"+money(amount),12,50,4);wrapped(bank+" "+at,90,2);wrapped("Noi dung: "+content,135,8);
}
bool DisplayManager::bankLink(const String& key,const String& apSsid,const String& apPassword){
  QRCode qr;std::vector<uint8_t> data(qrcode_getBufferSize(4));
  if(qrcode_initText(&qr,data.data(),4,ECC_MEDIUM,key.c_str())!=0)return false;
  tft.fillScreen(TFT_WHITE);tft.setTextColor(TFT_NAVY,TFT_WHITE);
  tft.drawCentreString("LIEN KET NGAN HANG",tft.width()/2,4,4);
  int scale=220/(qr.size+8),left=(tft.width()-(qr.size+8)*scale)/2;
  for(int y=0;y<qr.size;++y)for(int x=0;x<qr.size;++x)if(qrcode_getModule(&qr,x,y))
    tft.fillRect(left+(x+4)*scale,36+(y+4)*scale,scale,scale,TFT_BLACK);
  tft.drawCentreString(key,tft.width()/2,250,2);
  tft.drawCentreString(apSsid.length()?"Wi-Fi: "+apSsid+" / "+apPassword:"Admin quet ma de lien ket. Khong chia se ma.",tft.width()/2,274,2);
  tft.drawCentreString("QR cung co tai 192.168.4.1 khi noi Wi-Fi loa",tft.width()/2,296,2);
  return true;
}