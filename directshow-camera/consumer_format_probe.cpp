#define NOMINMAX
#include <windows.h>
#include <dshow.h>
#include <wrl/client.h>
#include <iostream>
using Microsoft::WRL::ComPtr;
int main(int argc,char**argv){CoInitializeEx(nullptr,COINIT_MULTITHREADED);HMODULE dll=argc>1?LoadLibraryA(argv[1]):LoadLibraryW(L"BandHeartBeatDirectShow-test.dll");if(!dll)return 2;bool old=argc>2;int result=0;{auto fn=reinterpret_cast<HRESULT(WINAPI*)(REFCLSID,REFIID,void**)>(GetProcAddress(dll,"DllGetClassObject"));CLSID id={0x5b4131af,0x3298,0x47aa,{0x9c,0x12,0x33,0x40,0x7a,0x81,0x91,0x4b}};ComPtr<IClassFactory>factory;if(FAILED(fn(id,IID_PPV_ARGS(&factory))))return 3;ComPtr<IBaseFilter>source;factory->CreateInstance(nullptr,IID_PPV_ARGS(&source));ComPtr<IPin>pin;source->FindPin(L"Output",&pin);ComPtr<IAMStreamConfig>cfg;pin.As(&cfg);for(int type:{0,1})for(int fps:{5,15,20,24,25,30,60}){AM_MEDIA_TYPE*mt=nullptr;VIDEO_STREAM_CONFIG_CAPS caps{};if(FAILED(cfg->GetStreamCaps(type,&mt,reinterpret_cast<BYTE*>(&caps))))return 4;reinterpret_cast<VIDEOINFOHEADER*>(mt->pbFormat)->AvgTimePerFrame=10000000/fps;auto hr=cfg->SetFormat(mt);bool accepted=SUCCEEDED(hr),expected=old?fps==30:fps<=30;std::cout<<"Chromium-style SetFormat RGB"<<(type?24:32)<<" "<<fps<<"fps HRESULT0x"<<std::hex<<hr<<std::dec<<" expected="<<expected<<"\n";if(accepted!=expected)result=1;CoTaskMemFree(mt->pbFormat);CoTaskMemFree(mt);}}FreeLibrary(dll);CoUninitialize();return result;}
