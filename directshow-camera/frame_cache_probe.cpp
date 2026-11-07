#define NOMINMAX
#include <windows.h>
#include <dshow.h>
#include <wrl/client.h>
#include <iostream>
#include "shared_frame.h"
#include "isolated_dll_guard.h"
#ifndef BH_ISOLATED_TEST
#error This probe must use isolated IPC names
#endif
#include "test_sink.h"
using Microsoft::WRL::ComPtr;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
uint32_t Pixel(int x,int y,int bias){return ((17+x+3*y+bias)&255)|(((85+2*x+y+bias)&255)<<8)|(((163+(x^y)+bias)&255)<<16);}
bool Last(TestSink*s,int bias){auto a=s->Pixels();if(a.size()<2)return false;for(size_t i=a.size()-2;i<a.size();i++){if(bias<0){if(a[i][0]!=0x202020)return false;}else{const int x[]={0,639,0,37},y[]={0,0,479,221};for(int j=0;j<4;j++)if(a[i][j]!=Pixel(x[j],y[j],bias))return false;}}return true;}
int Test(bool rgb24){auto dll=LoadLibraryW(L"BandHeartBeatDirectShow-test.dll");CHECK(dll&&IsolatedFrameDll(dll));auto get=reinterpret_cast<HRESULT(WINAPI*)(REFCLSID,REFIID,void**)>(GetProcAddress(dll,"DllGetClassObject"));CLSID id={0x5b4131af,0x3298,0x47aa,{0x9c,0x12,0x33,0x40,0x7a,0x81,0x91,0x4b}};ComPtr<IClassFactory>factory;CHECK(get(id,IID_PPV_ARGS(&factory))==S_OK);ComPtr<IBaseFilter>source;CHECK(factory->CreateInstance(nullptr,IID_PPV_ARGS(&source))==S_OK);ComPtr<IPin>pin;CHECK(source->FindPin(L"Output",&pin)==S_OK);ComPtr<IAMStreamConfig>cfg;pin.As(&cfg);AM_MEDIA_TYPE*mt=nullptr;VIDEO_STREAM_CONFIG_CAPS caps{};CHECK(cfg->GetStreamCaps(rgb24?1:0,&mt,reinterpret_cast<BYTE*>(&caps))==S_OK);CHECK(cfg->SetFormat(mt)==S_OK);CoTaskMemFree(mt->pbFormat);CoTaskMemFree(mt);auto sink=new TestSink();sink->verifyPixel=false;sink->acceptRgb24=true;CHECK(pin->Connect(sink,nullptr)==S_OK);CHECK(source->Run(0)==S_OK);CHECK(sink->Wait(3));CHECK(Last(sink,-1));
 auto map=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,32+640*480*4,BH_SHM_NAME);CHECK(map&&GetLastError()!=ERROR_ALREADY_EXISTS);auto view=static_cast<BYTE*>(MapViewOfFile(map,FILE_MAP_WRITE,0,0,0));auto gate=CreateMutexW(nullptr,FALSE,BH_FRAME_MUTEX);CHECK(view&&gate);uint64_t sequence=0;auto publish=[&](int bias){WaitForSingleObject(gate,1000);auto h=new(view)BhFrameHeader();h->sequence=++sequence;for(int y=0;y<480;y++)for(int x=0;x<640;x++){auto value=Pixel(x,y,bias)|0xff000000;memcpy(view+32+(y*640+x)*4,&value,4);}ReleaseMutex(gate);};auto wait=[&]{return sink->Wait(sink->count+4);};publish(0);CHECK(wait());CHECK(Last(sink,0));
 WaitForSingleObject(gate,1000);auto first=sink->Pixels().size();Sleep(600);auto held=sink->Pixels();for(size_t i=first;i<held.size();i++)CHECK(held[i][0]==Pixel(0,0,0));ReleaseMutex(gate);CHECK(wait());CHECK(Last(sink,0));publish(30);CHECK(wait());CHECK(Last(sink,30));WaitForSingleObject(gate,1000);reinterpret_cast<BhFrameHeader*>(view)->version=99;ReleaseMutex(gate);CHECK(wait());CHECK(Last(sink,-1));publish(50);CHECK(wait());CHECK(Last(sink,50));Sleep(2200);CHECK(Last(sink,-1));publish(70);CHECK(wait());CHECK(Last(sink,70));UnmapViewOfFile(view);CloseHandle(map);CloseHandle(gate);CHECK(wait());CHECK(Last(sink,-1));CHECK(source->Stop()==S_OK);CHECK(pin->Disconnect()==S_OK);sink->Disconnect();CHECK(sink->valid);sink->Release();cfg.Reset();pin.Reset();source.Reset();factory.Reset();auto unload=reinterpret_cast<HRESULT(WINAPI*)()>(GetProcAddress(dll,"DllCanUnloadNow"));CHECK(unload()==S_OK);FreeLibrary(dll);std::cout<<"PASS RGB"<<(rgb24?24:32)<<" firstempty/valid/contention/recovery/invalid/stale2s/producerstop and4point stride/channel conversion\n";return 0;}
int main(){CoInitializeEx(nullptr,COINIT_MULTITHREADED);int result=Test(false);if(!result)result=Test(true);CoUninitialize();return result;}
