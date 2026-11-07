#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include "controls.h"
struct FaceBox {float x,y,w,h;};
enum class AnchorMode {Searching,Tracking,Holding,Fallback,Manual};
struct Anchor {
 float x=420,y=60;bool tracking=false,visible=true;ULONGLONG seen=0;
 AnchorMode mode=AnchorMode::Searching;
 ULONGLONG previous=0,lost_at=0;bool updated=false,ever_seen=false,lost=false;
 void Update(const std::vector<FaceBox>&faces,ULONGLONG now,const CameraControls&settings=CameraControls{},ULONGLONG observed_at=~0ULL){
  auto c=Sanitize(settings);float w=180.f*c.size/100,h=86.f*c.size/100;
  float dt=updated&&now>=previous?static_cast<float>(now-previous)/1000.f:0.f;ULONGLONG last=previous;previous=now;updated=true;
  auto move=[&](float tx,float ty,float seconds,float tau){tx=std::clamp(tx,0.f,640-w);ty=std::clamp(ty,0.f,480-h);float a=1.f-std::exp(-std::max(0.f,seconds)/tau);x+=(tx-x)*a;y+=(ty-y)*a;};
  visible=true;
  if(!c.attach){tracking=false;lost=false;mode=AnchorMode::Manual;x=static_cast<float>(c.manual_x+c.offset_x);y=static_cast<float>(c.manual_y+c.offset_y);}
  else if(!faces.empty()){
   auto face=*std::max_element(faces.begin(),faces.end(),[](auto a,auto b){return a.w*a.h<b.w*b.h;});
   float tx=face.x+face.w+14;if(tx+w>640)tx=face.x-w-14;float ty=face.y+face.h*.25f;tx+=c.offset_x;ty+=c.offset_y;
   if(!ever_seen){x=tx;y=ty;}else move(tx,ty,dt,.04f+.006f*c.smoothing);
   ever_seen=true;lost=false;tracking=true;seen=observed_at==~0ULL?now:observed_at;mode=AnchorMode::Tracking;
  }else{
   tracking=false;
   if(ever_seen&&!lost){lost=true;lost_at=seen;}
   if(ever_seen&&now-lost_at<2000){mode=AnchorMode::Holding;}
   else{mode=ever_seen?AnchorMode::Fallback:AnchorMode::Searching;visible=c.fallback!=0;
    ULONGLONG begin=ever_seen?std::max(last,lost_at+2000):last;
    float elapsed=updated&&now>=begin?static_cast<float>(now-begin)/1000.f:0.f;
    if(visible)move(640-w-12,480-h-12,elapsed,.35f);
   }
  }
  x=std::clamp(x,0.f,std::max(0.f,640-w));y=std::clamp(y,0.f,std::max(0.f,480-h));
 }
};
