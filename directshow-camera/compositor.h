#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
struct FaceBox {float x,y,w,h;};
struct Anchor {float x=420,y=60;bool tracking=false;ULONGLONG seen=0;
 void Update(const std::vector<FaceBox>&faces,ULONGLONG now){if(faces.empty()){if(now-seen>1500){tracking=false;x=420;y=60;}return;}auto face=*std::max_element(faces.begin(),faces.end(),[](auto a,auto b){return a.w*a.h<b.w*b.h;});float tx=face.x+face.w+14;if(tx+180>640)tx=face.x-194;tx=std::clamp(tx,0.f,460.f);float ty=std::clamp(face.y+face.h*.25f,0.f,370.f);x=tracking?x*.65f+tx*.35f:tx;y=tracking?y*.65f+ty*.35f:ty;tracking=true;seen=now;}
};
inline void Compose(std::vector<BYTE>&pixels,const Anchor&a,int bpm,bool live,bool demo,ULONGLONG tick){
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=640;bi.bmiHeader.biHeight=-480;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;void*bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap||!bits){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return;}auto old=SelectObject(dc,bitmap);memcpy(bits,pixels.data(),pixels.size());
 int x=static_cast<int>(a.x),y=static_cast<int>(a.y);bool active=bpm>0&&(live||demo);float phase=active?static_cast<float>(tick%60000)*bpm/60000.f:0;float pulse=active?1.f+.10f*static_cast<float>(std::pow(std::max(0.f,std::sin(phase*6.283185f)),6)):1.f;int r=static_cast<int>(16*pulse);
 HBRUSH dark=CreateSolidBrush(RGB(27,30,37));RECT panel{x,y,x+180,y+106};FillRect(dc,&panel,dark);DeleteObject(dark);HBRUSH heart=CreateSolidBrush(active?RGB(255,139,115):RGB(135,115,110));auto ob=SelectObject(dc,heart);auto op=SelectObject(dc,GetStockObject(NULL_PEN));int cx=x+30,cy=y+28;BeginPath(dc);MoveToEx(dc,cx,cy+r,nullptr);POINT curve[]={{cx-r*2,cy-r/2},{cx-r,cy-r*2},{cx,cy-r/2},{cx+r,cy-r*2},{cx+r*2,cy-r/2},{cx,cy+r}};PolyBezierTo(dc,curve,6);EndPath(dc);FillPath(dc);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(heart);
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));HFONT font=CreateFontW(-35,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");auto of=SelectObject(dc,font);std::wstring text=active?std::to_wstring(bpm):L"—";TextOutW(dc,x+60,y+7,text.c_str(),static_cast<int>(text.size()));SelectObject(dc,of);DeleteObject(font);font=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");SelectObject(dc,font);text=demo?L"DEMO · NO BAND":live?L"BPM · LIVE":L"BPM · DISCONNECTED";TextOutW(dc,x+10,y+55,text.c_str(),static_cast<int>(text.size()));text=a.tracking?L"FACE TRACKING":L"NO FACE · CORNER";TextOutW(dc,x+10,y+77,text.c_str(),static_cast<int>(text.size()));SelectObject(dc,of);DeleteObject(font);memcpy(pixels.data(),bits,pixels.size());SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
}
