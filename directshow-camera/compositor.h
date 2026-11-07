#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include "tracking.h"
#pragma comment(lib,"gdiplus.lib")
namespace bh_render {
// One GDI+ lifetime per module, with thread-safe C++ static initialization.
struct Runtime { ULONG_PTR token=0; Runtime(){Gdiplus::GdiplusStartupInput in;Gdiplus::GdiplusStartup(&token,&in,nullptr);} ~Runtime(){if(token)Gdiplus::GdiplusShutdown(token);} };
inline void Round(Gdiplus::GraphicsPath&p,float x,float y,float w,float h,float r){p.AddArc(x,y,r*2,r*2,180,90);p.AddArc(x+w-r*2,y,r*2,r*2,270,90);p.AddArc(x+w-r*2,y+h-r*2,r*2,r*2,0,90);p.AddArc(x,y+h-r*2,r*2,r*2,90,90);p.CloseFigure();}
inline void Label(Gdiplus::Graphics&g,const std::wstring&s,float x,float y,float w,float h,float size,Gdiplus::Color color,int style=Gdiplus::FontStyleRegular){
 Gdiplus::FontFamily family(L"Segoe UI");Gdiplus::StringFormat format;format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);format.SetTrimming(Gdiplus::StringTrimmingNone);format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
 for(;;){Gdiplus::Font f(&family,size,style,Gdiplus::UnitPixel);Gdiplus::RectF measured;g.MeasureString(s.c_str(),-1,&f,Gdiplus::PointF(0,0),&format,&measured);if((measured.Width<=w&&measured.Height<=h)||size<=6){Gdiplus::SolidBrush b(color);g.DrawString(s.c_str(),-1,&f,Gdiplus::RectF(x,y,w,h),&format,&b);break;}size-=.5f;}
}
inline void Heart(Gdiplus::Graphics&g,float pulse,bool outline,Gdiplus::Color color){
 Gdiplus::GraphicsPath heart;heart.AddBezier(30.f,39.f,4.f,23.f,19.f,9.f,30.f,21.f);heart.AddBezier(30.f,21.f,41.f,9.f,56.f,23.f,30.f,39.f);heart.CloseFigure();
 Gdiplus::Matrix t;t.Translate(30,28);t.Scale(pulse,pulse);t.Translate(-30,-28);heart.Transform(&t);
 if(outline){Gdiplus::Pen pen(color,1.8f);pen.SetLineJoin(Gdiplus::LineJoinRound);g.DrawPath(&pen,&heart);}else{Gdiplus::SolidBrush fill(color);g.FillPath(&fill,&heart);}
}
}
inline void Compose(std::vector<BYTE>&pixels,const Anchor&a,int bpm,bool live,bool demo,ULONGLONG tick,const CameraControls&settings=CameraControls{}){
 using namespace Gdiplus;using namespace bh_render;
 if(pixels.size()<640u*480u*4u||!a.visible)return;
 static Runtime runtime;if(!runtime.token)return;auto c=Sanitize(settings);
 // Render to a transparent, supersampled surface; SourceOver applies real alpha to video.
 Bitmap card(360,172,PixelFormat32bppPARGB);Graphics g(&card);g.Clear(Color(0,0,0,0));g.ScaleTransform(2,2);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
 bool active=bpm>0&&(live||demo);Color coral=active?Color(255,255,112,125):Color(255,153,165,177);Color white(255,246,249,252),muted(255,175,188,200);
 GraphicsPath shadow;Round(shadow,2,4,176,81,14);SolidBrush shade(Color(50,0,0,0));g.FillPath(&shade,&shadow);
 GraphicsPath body;Round(body,1,1,178,81,13);LinearGradientBrush glass(PointF(0,0),PointF(0,82),Color(c.style==1?185:220,30,39,49),Color(195,15,22,30));g.FillPath(&glass,&body);Pen border(Color(68,196,218,234),.7f);g.DrawPath(&border,&body);
 float phase=active?static_cast<float>(tick%60000)*bpm/60000.f:0;float pulse=1.f+.075f*std::pow(std::max(0.f,std::sin(phase*6.283185f)),6.f);
 if(c.style==0){SolidBrush glow(Color(active?24:8,255,112,125));g.FillEllipse(&glow,9.f,8.f,42.f,42.f);Heart(g,pulse,false,coral);}
 else if(c.style==1)Heart(g,1.f,true,coral);
 else if(c.style==2){Pen baseline(Color(50,144,220,211),1.f);g.DrawLine(&baseline,10.f,29.f,51.f,29.f);PointF wave[]={{10,29},{19,29},{23,24},{27,39},{32,16},{36,30},{41,29},{51,29}};Pen ecg(active?Color(255,108,224,205):muted,1.8f);ecg.SetLineJoin(LineJoinRound);g.DrawLines(&ecg,wave,8);}
 else {Pen rail(Color(55,187,205,218),3.f);rail.SetStartCap(LineCapRound);rail.SetEndCap(LineCapRound);g.DrawArc(&rail,12.f,12.f,37.f,37.f,145.f,250.f);Pen arc(coral,3.f);arc.SetStartCap(LineCapRound);arc.SetEndCap(LineCapRound);float fraction=active?(std::clamp(bpm,40,180)-40)/140.f:.0f;g.DrawArc(&arc,12.f,12.f,37.f,37.f,145.f,std::max(3.f,250.f*fraction));Label(g,active?L"PULSE":L"WAIT",16,23,30,12,6.5f,muted);}
 Label(g,active?std::to_wstring(bpm):L"\x2014",61,6,75,43,32.f*c.scale/100.f,white,FontStyleBold);Label(g,L"BPM",137,24,31,17,9.f,muted,FontStyleBold);
 Pen divider(Color(28,204,220,231),.6f);g.DrawLine(&divider,12.f,52.f,168.f,52.f);
 Color signal=demo?Color(255,240,187,108):live?Color(255,105,221,184):muted;SolidBrush dot(signal);g.FillEllipse(&dot,12.f,60.f,4.f,4.f);
 Label(g,demo?L"DEMO / NO BAND":live?L"LIVE BAND":L"DISCONNECTED",20,55,148,13,8.f,signal,FontStyleBold);
 const wchar_t*status=a.mode==AnchorMode::Tracking?L"FACE TRACKING":a.mode==AnchorMode::Holding?L"FACE LOST / HOLDING":a.mode==AnchorMode::Manual?L"MANUAL POSITION":a.mode==AnchorMode::Searching?L"SEARCHING FOR FACE":L"NO FACE / CORNER";
 Label(g,status,12,68,156,10,6.8f,Color(255,145,162,178));
 Bitmap frame(640,480,640*4,PixelFormat32bppRGB,pixels.data());Graphics output(&frame);output.SetCompositingMode(CompositingModeSourceOver);output.SetInterpolationMode(InterpolationModeHighQualityBicubic);float z=c.size/100.f;output.DrawImage(&card,RectF(a.x,a.y,180*z,86*z),0,0,360,172,UnitPixel);
}
