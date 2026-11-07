#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <set>
#include <thread>
#include "shared_frame.h"
#include "controls.h"
#include "compositor.h"
uint64_t Hash(const BYTE*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;i++)h=(h^p[i])*1099511628211ull;return h;}
int main(){static_assert(sizeof(CameraControls)==72);ControlsIpc ipc;CameraControls c;CameraState s;c.attach=0;c.manual_x=100;c.manual_y=100;ipc.Write(c);
 // Offline deterministic renderer asserts each style/control affects actual pixels.
 std::set<uint64_t>hashes;Anchor a;a.Update({},2000,c);for(int style=0;style<4;style++){auto x=c;x.style=style;std::vector<BYTE>p(640*480*4,20);Compose(p,a,72,false,true,1000,x);hashes.insert(Hash(p.data(),p.size()));}if(hashes.size()!=4)return 1;
 Anchor tracked;tracked.Update({{200,120,100,100}},2000,c=CameraControls{});float x=tracked.x;c.offset_x=30;tracked.Update({{200,120,100,100}},2100,c);if(tracked.x<=x)return 2;c.attach=0;c.size=140;c.scale=160;c.manual_x=630;c.manual_y=470;a.Update({},2200,c);if(a.x+CardWidth(c.size)>640||a.y+CardHeight(c.size)>480.01f)return 3;
 c=CameraControls{};c.attach=0;c.manual_x=100;c.manual_y=100;ipc.Write(c);std::cout<<"PASS renderer4styles, placement/offset/clamp and control ABI\n";
 // Launch isolated producer, never interact with production/user mappings.
 STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};wchar_t command[]=L"camera-producer-test.exe --mock --seconds 10";if(!CreateProcessW(nullptr,command,nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi))return 4;CloseHandle(pi.hThread);
 HANDLE map=nullptr,gate=nullptr;const BYTE*view=nullptr;for(int i=0;i<60;i++){map=OpenFileMappingW(FILE_MAP_READ,FALSE,BH_SHM_NAME);if(map){view=static_cast<const BYTE*>(MapViewOfFile(map,FILE_MAP_READ,0,0,32+640*480*4));gate=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,BH_FRAME_MUTEX);if(view&&gate)break;}Sleep(50);}if(!view||!gate){TerminateProcess(pi.hProcess,1);CloseHandle(pi.hProcess);return 5;}
 auto snapshot=[&](){std::pair<uint64_t,uint64_t> result{};if(WaitForSingleObject(gate,100)==WAIT_OBJECT_0){auto h=reinterpret_cast<const BhFrameHeader*>(view);result={h->sequence,Hash(view+32,640*480*4)};ReleaseMutex(gate);}return result;};
 uint64_t previous=0;std::set<uint64_t> published;for(int style=0;style<4;style++){c.style=style;c.manual_x=100+style*30;c.size=80+style*10;ipc.Write(c);bool applied=false;for(int i=0;i<40;i++){Sleep(50);ipc.Read(c,s);auto current=snapshot();if(s.x==100+style*30&&s.w==static_cast<int>(CardWidth(80+style*10))&&current.first>previous){previous=current.first;published.insert(current.second);applied=true;break;}}if(!applied){TerminateProcess(pi.hProcess,1);return 6;}}
 if(published.size()!=4)return 9; c.mirror_horizontal=1;ipc.Write(c);Sleep(250);auto mirrored=snapshot();c.mirror_horizontal=0;ipc.Write(c);Sleep(250);auto normal=snapshot();if(mirrored.second==normal.second)return 10;
 c.attach=1;c.fallback=0;ipc.Write(c);Sleep(250);auto hidden=snapshot();c.fallback=1;ipc.Write(c);Sleep(250);auto shown=snapshot();if(hidden.second==shown.second){TerminateProcess(pi.hProcess,1);return 7;}
 c=CameraControls{};c.attach=0;c.manual_x=100;c.manual_y=100;c.transparent_background=0;ipc.Write(c);Sleep(250);
 auto pixel=[&](){uint32_t value=0;if(WaitForSingleObject(gate,100)==WAIT_OBJECT_0){memcpy(&value,view+32+((479-140)*640+220)*4,4);ReleaseMutex(gate);}return value&0xffffff;};auto panel=pixel();c.transparent_background=1;ipc.Write(c);Sleep(250);auto foreground=pixel();if(foreground!=(73u|(70u<<8)|(35u<<16))||panel==foreground){TerminateProcess(pi.hProcess,1);return 10;}
 c.size=140;c.manual_x=630;c.manual_y=470;ipc.Write(c);Sleep(250);ipc.Read(c,s);if(s.w!=176||s.h!=120||s.x+177>640||s.y+121>481){TerminateProcess(pi.hProcess,1);return 11;}
 HANDLE quit=OpenEventW(EVENT_MODIFY_STATE,FALSE,BH_CAPTURE_STOP);if(quit){SetEvent(quit);CloseHandle(quit);}if(WaitForSingleObject(pi.hProcess,5000)!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,1);return 8;}CloseHandle(pi.hProcess);UnmapViewOfFile(view);CloseHandle(map);CloseHandle(gate);std::cout<<"PASS isolated producer control-to-published-pixels styles/position/size/fallback/no-background; clean stop\n";return 0;}


