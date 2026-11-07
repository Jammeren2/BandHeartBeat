#define NOMINMAX
#include "compositor.h"
#include <iostream>
#include <set>
#include <filesystem>
using namespace Gdiplus;
void Background(std::vector<BYTE>&p,int variant=0){p.resize(640*480*4);for(int y=0;y<480;y++)for(int x=0;x<640;x++){auto i=(y*640+x)*4;bool tile=((x/24+y/24)%2)==0;p[i]=static_cast<BYTE>(45+x/7+variant*15);p[i+1]=static_cast<BYTE>(38+y/9+(tile?13:0));p[i+2]=static_cast<BYTE>(40+x/12+(tile?16:0));if(variant==2){p[i]=static_cast<BYTE>(212+(tile?15:0));p[i+1]=static_cast<BYTE>(217+(tile?15:0));p[i+2]=static_cast<BYTE>(224+(tile?15:0));}p[i+3]=255;}}
int main(){bh_render::Runtime runtime;CLSID png{};UINT n=0,bytes=0;GetImageEncodersSize(&n,&bytes);std::vector<BYTE>storage(bytes);auto enc=reinterpret_cast<ImageCodecInfo*>(storage.data());GetImageEncoders(n,bytes,enc);for(UINT i=0;i<n;i++)if(wcscmp(enc[i].MimeType,L"image/png")==0)png=enc[i].Clsid;
 std::filesystem::create_directories(L"directshow-camera/out/renderer-review");Bitmap sheet(1440,1000,PixelFormat32bppRGB);Graphics out(&sheet);out.Clear(Color(255,12,17,24));std::set<uint64_t>styles;
 const wchar_t*names[]={L"01  GLASS HEART",L"02  OUTLINE HEART",L"03  ECG",L"04  GAUGE"};const wchar_t*rows[]={L"Live / tracking / 100%",L"Demo / holding / 100%",L"Disconnected / corner",L"140% size / 160% type / 999 BPM",L"60% size / 60% type / manual"};
 for(int row=0;row<5;row++)for(int style=0;style<4;style++){
  std::vector<BYTE>p;Background(p);auto before=p;CameraControls c;c.style=style;Anchor a;a.x=80;a.y=180;a.visible=true;a.mode=row==1?AnchorMode::Holding:row==2?AnchorMode::Fallback:row==4?AnchorMode::Manual:AnchorMode::Tracking;a.tracking=row==0||row==3;if(row==3){c.size=140;c.scale=160;}if(row==4){c.size=60;c.scale=60;}
  Compose(p,a,row==2?0:row==3?999:82,row!=1&&row!=2,row==1,1030,c);
  uint64_t hash=0;for(size_t i=0;i<p.size();i++)hash=hash*131+p[i];if(row==0)styles.insert(hash);
  for(int y=0;y<480;y++)for(int x=0;x<640;x++)if(x<79||x>333||y<179||y>302)for(int k=0;k<3;k++)if(p[(y*640+x)*4+k]!=before[(y*640+x)*4+k])return 2;
  Bitmap frame(640,480,2560,PixelFormat32bppRGB,p.data());float sx=static_cast<float>(style*360),sy=static_cast<float>(row*200);bh_render::Label(out,names[style],sx+16,sy+5,330,20,12,Color(255,227,235,242),FontStyleBold);bh_render::Label(out,rows[row],sx+16,sy+27,330,18,10,Color(255,149,168,185));out.DrawImage(&frame,RectF(sx,sy+52,360,148),35,160,360,148,UnitPixel);
  if(row==0&&style==0)frame.Save(L"directshow-camera/out/renderer-review/live-glass-frame.png",&png);
 }
 if(styles.size()!=4)return 3;
 // Invisible states must not touch the video; held states remain visible without fallback.
 std::vector<BYTE>p;Background(p);auto original=p;Anchor a;a.visible=false;Compose(p,a,82,true,false,1000);if(p!=original)return 4;a.visible=true;a.mode=AnchorMode::Holding;CameraControls c;c.fallback=0;Compose(p,a,82,true,false,1000,c);if(p==original)return 5;
 // Check actual translucency: different video remains different underneath the panel.
 std::vector<BYTE>q;Background(q,1);a.x=80;a.y=180;Compose(q,a,82,true,false,1000,c);Background(p);Compose(p,a,82,true,false,1000,c);size_t sample=(220*640+170)*4;if(p[sample]==q[sample])return 6;
 if(sheet.Save(L"directshow-camera/out/renderer-review/contact-sheet.png",&png)!=Ok)return 7;
 Bitmap modes(1280,1920,PixelFormat32bppRGB);Graphics compare(&modes);compare.Clear(Color(255,12,17,24));
 for(int row=0;row<12;row++){
  int state=row/4;bool light=(row%4)/2!=0,plain=row%2!=0;std::set<uint64_t> distinct;
  for(int style=0;style<4;style++){
   std::vector<BYTE> video;Background(video,light?2:0);auto clean=video;CameraControls config;config.style=style;config.transparent_background=plain?1:0;
   Anchor anchor;anchor.x=80;anchor.y=180;anchor.visible=true;anchor.mode=state==1?AnchorMode::Holding:state==2?AnchorMode::Fallback:AnchorMode::Tracking;
   Compose(video,anchor,state==2?0:82,state==0,state==1,1030,config);
   if(video==clean)return 8;uint64_t signature=0;for(size_t i=0;i<video.size();i++)signature=signature*131+video[i];distinct.insert(signature);
   for(int y=0;y<480;y++)for(int x=0;x<640;x++)if(x<79||x>261||y<179||y>267)for(int k=0;k<3;k++)if(video[(y*640+x)*4+k]!=clean[(y*640+x)*4+k])return 9;
   if(plain){
    // Corners, former divider, inter-column gap and former badge must stay untouched.
    const POINT untouched[]={{85,185},{250,190},{170,250},{135,222},{91,209},{170,260}};
    for(auto point:untouched)for(int k=0;k<3;k++){auto i=(point.y*640+point.x)*4+k;if(video[i]!=clean[i]){std::cerr<<"residue "<<point.x<<","<<point.y<<" style "<<style<<"\n";return 10;}}
   }
   float sx=static_cast<float>(style*320),sy=static_cast<float>(row*160);bh_render::Label(compare,names[style],sx+12,sy+3,300,18,11,Color(255,230,239,247),FontStyleBold);
   std::wstring description=plain?L"NO BACKGROUND / ":L"GLASS CARD / ";description+=light?L"LIGHT / ":L"DARK / ";description+=state==0?L"LIVE":state==1?L"DEMO":L"DISCONNECTED";
   bh_render::Label(compare,description,sx+12,sy+23,300,17,9,Color(255,158,178,195));Bitmap frame(640,480,2560,PixelFormat32bppRGB,video.data());compare.DrawImage(&frame,RectF(sx,sy+43,320,117),45,165,320,117,UnitPixel);
  }
  if(distinct.size()!=4)return 11;
 }
 if(modes.Save(L"directshow-camera/out/renderer-review/background-modes.png",&png)!=Ok)return 12;
 std::cout<<"PASS background modes: 48 renders, light/dark live/demo/disconnected, exact unchanged empty pixels, foreground, style distinction, bounds.\n";
 std::cout<<"PASS four styles; 20 renders; bounds; hidden/held; true alpha. PNG: directshow-camera/out/renderer-review/contact-sheet.png\n";
}
