#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
struct FaceBox {float x,y,w,h;};

#include "controls.h"
struct Anchor {
 float x=420,y=60; bool tracking=false; ULONGLONG seen=0;
 void Update(const std::vector<FaceBox>&faces,ULONGLONG now,const CameraControls&c=CameraControls{}){
  float w=180.f*c.size/100,h=86.f*c.size/100;
  if(!c.attach){tracking=false;x=static_cast<float>(c.manual_x+c.offset_x);y=static_cast<float>(c.manual_y+c.offset_y);}
  else if(faces.empty()){if(now-seen>1500){tracking=false;x=640-w-12;y=480-h-12;}}
  else{auto face=*std::max_element(faces.begin(),faces.end(),[](auto a,auto b){return a.w*a.h<b.w*b.h;});
   float tx=face.x+face.w+14;if(tx+w>640)tx=face.x-w-14;float ty=face.y+face.h*.25f;
   tx+=c.offset_x;ty+=c.offset_y;float smooth=c.smoothing/100.f;
   x=tracking?x*smooth+tx*(1-smooth):tx;y=tracking?y*smooth+ty*(1-smooth):ty;tracking=true;seen=now;
  }
  x=std::clamp(x,0.f,std::max(0.f,640-w));y=std::clamp(y,0.f,std::max(0.f,480-h));
 }
};
inline void Compose(std::vector<BYTE>&pixels,const Anchor&a,int bpm,bool live,bool demo,ULONGLONG tick,const CameraControls&settings=CameraControls{}){
 auto c=Sanitize(settings);if(c.attach&&!a.tracking&&!c.fallback)return;
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=640;bi.bmiHeader.biHeight=-480;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 void*bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);
 if(!bitmap||!bits){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return;}auto old=SelectObject(dc,bitmap);memcpy(bits,pixels.data(),pixels.size());
 float z=c.size/100.f;int x=static_cast<int>(a.x),y=static_cast<int>(a.y),width=static_cast<int>(180*z),height=static_cast<int>(86*z);
 HBRUSH panel=CreateSolidBrush(RGB(24,29,35));auto oldBrush=SelectObject(dc,panel);auto oldPen=SelectObject(dc,GetStockObject(NULL_PEN));RoundRect(dc,x,y,x+width,y+height,static_cast<int>(16*z),static_cast<int>(16*z));SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(panel);
 bool active=bpm>0&&(live||demo);float phase=active?static_cast<float>(tick%60000)*bpm/60000.f:0;
 float pulse=active?1.f+.10f*static_cast<float>(std::pow(std::max(0.f,std::sin(phase*6.283185f)),6)):1.f;
 int cx=x+static_cast<int>(30*z),cy=y+static_cast<int>(29*z),r=static_cast<int>(14*z*pulse);COLORREF coral=active?RGB(255,111,116):RGB(128,128,135);
 HPEN pen=CreatePen(PS_SOLID,std::max(1,static_cast<int>(2*z)),coral);auto op=SelectObject(dc,pen);HBRUSH heart=CreateSolidBrush(coral);auto ob=SelectObject(dc,c.style==1?GetStockObject(NULL_BRUSH):heart);
 if(c.style<2){BeginPath(dc);MoveToEx(dc,cx,cy+r,nullptr);POINT curve[]={{cx-r*2,cy-r/2},{cx-r,cy-r*2},{cx,cy-r/2},{cx+r,cy-r*2},{cx+r*2,cy-r/2},{cx,cy+r}};PolyBezierTo(dc,curve,6);EndPath(dc);if(c.style==1)StrokePath(dc);else FillPath(dc);}
 else if(c.style==2){POINT points[]={{cx-r*2,cy},{cx-r,cy},{cx-r/2,cy-r},{cx,cy+r},{cx+r/2,cy-r/2},{cx+r,cy},{cx+r*2,cy}};Polyline(dc,points,7);}
 else{Arc(dc,cx-r-5,cy-r-5,cx+r+5,cy+r+5,cx-r-5,cy+2,cx+r+5,cy+2);MoveToEx(dc,cx,cy,nullptr);float angle=active?(std::clamp(bpm,40,180)-40)/140.f*3.14159f:0;LineTo(dc,cx-static_cast<int>(std::cos(angle)*r),cy-static_cast<int>(std::sin(angle)*r));}
 SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(heart);DeleteObject(pen);
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(246,249,251));
 auto text=[&](int tx,int ty,int size,COLORREF color,const std::wstring&value){if(value.empty())return;auto font=CreateFontW(-size,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");auto before=SelectObject(dc,font);SetTextColor(dc,color);TextOutW(dc,tx,ty,value.c_str(),static_cast<int>(value.size()));SelectObject(dc,before);DeleteObject(font);};
 text(x+static_cast<int>(58*z),y+static_cast<int>(7*z),static_cast<int>(30*z*c.scale/100.f),RGB(246,249,251),active?std::to_wstring(bpm):L"—");
 text(x+static_cast<int>(120*z),y+static_cast<int>(22*z),static_cast<int>(11*z),RGB(177,187,194),L"BPM");
 text(x+static_cast<int>(10*z),y+static_cast<int>(53*z),std::max(9,static_cast<int>(10*z)),RGB(161,186,190),demo?L"DEMO · NO BAND":live?L"LIVE":L"DISCONNECTED");
 text(x+static_cast<int>(10*z),y+static_cast<int>(67*z),std::max(8,static_cast<int>(9*z)),RGB(139,151,162),a.tracking?L"FACE TRACKING":c.attach?L"NO FACE · CORNER":L"MANUAL POSITION");
 memcpy(pixels.data(),bits,pixels.size());SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
}
