#pragma once
#include <windows.h>
#include <cstdint>
#include <algorithm>
#include <new>
#ifdef BH_ISOLATED_TEST
inline constexpr wchar_t CONTROL_NAME[]=L"Local\\BandHeartBeatControlsV2UxTest";
inline constexpr wchar_t CONTROL_MUTEX[]=L"Local\\BandHeartBeatControlsMutexV2UxTest";
#else
inline constexpr wchar_t CONTROL_NAME[]=L"Local\\BandHeartBeatControlsV2";
inline constexpr wchar_t CONTROL_MUTEX[]=L"Local\\BandHeartBeatControlsMutexV2";
#endif
struct CameraControls {
 uint32_t magic=0x42484354,version=2;
 int32_t style=0,attach=1,fallback=1,size=100,scale=100,offset_x=0,offset_y=0,smoothing=65;
 int32_t manual_x=420,manual_y=60,manual=0;
 int32_t transparent_background=0;
};
struct CameraState {
 uint32_t magic=0x42485354,version=2;
 uint64_t tick=0;
 int32_t x=420,y=60,w=180,h=86,tracking=0,bpm=0,live=0,demo=0;
};
struct CameraControlPage {CameraControls controls;CameraState state;};
inline CameraControls Sanitize(CameraControls c){c.transparent_background=c.transparent_background?1:0;c.style=std::clamp(c.style,0,3);c.size=std::clamp(c.size,60,140);c.scale=std::clamp(c.scale,60,160);c.offset_x=std::clamp(c.offset_x,-200,200);c.offset_y=std::clamp(c.offset_y,-200,200);c.smoothing=std::clamp(c.smoothing,0,95);c.manual_x=std::clamp(c.manual_x,0,640);c.manual_y=std::clamp(c.manual_y,0,480);return c;}
class ControlsIpc {
 HANDLE mapping=nullptr,mutex=nullptr;CameraControlPage*page=nullptr;
public:
 ControlsIpc(){mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(CameraControlPage),CONTROL_NAME);bool fresh=GetLastError()!=ERROR_ALREADY_EXISTS;if(mapping)page=static_cast<CameraControlPage*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(CameraControlPage)));mutex=CreateMutexW(nullptr,FALSE,CONTROL_MUTEX);if(page&&mutex&&fresh){if(WaitForSingleObject(mutex,100)==WAIT_OBJECT_0){new(page)CameraControlPage();ReleaseMutex(mutex);}}}
 ~ControlsIpc(){if(page)UnmapViewOfFile(page);if(mapping)CloseHandle(mapping);if(mutex)CloseHandle(mutex);}
 bool Read(CameraControls&c,CameraState&s){if(!page||!mutex)return false;auto wait=WaitForSingleObject(mutex,10);if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return false;bool valid=page->controls.magic==0x42484354&&page->controls.version==2;c=valid?Sanitize(page->controls):CameraControls{};s=page->state;ReleaseMutex(mutex);return valid;}
 void Write(const CameraControls&c){if(!page||!mutex)return;auto wait=WaitForSingleObject(mutex,10);if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED){page->controls=Sanitize(c);ReleaseMutex(mutex);}}
 void State(const CameraState&s){if(!page||!mutex)return;auto wait=WaitForSingleObject(mutex,10);if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED){page->state=s;ReleaseMutex(mutex);}}
};
