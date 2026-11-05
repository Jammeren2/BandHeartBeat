#include <dshow.h>
#include <iostream>
#include <string>
#include "test_sink.h"
int wmain(int argc,wchar_t**argv){std::wstring cmd=argc>1?argv[1]:L"--dry-run";
 if(cmd==L"--dry-run"){std::wcout<<L"No change. --register / --unregister: only this DLL's HKCU CLSID and video-input category entry. --probe: enumerate/bind this virtual source only.\n";return 0;}
 if(cmd==L"--register"||cmd==L"--unregister"){HMODULE dll=LoadLibraryW(L"BandHeartBeatDirectShow.dll");if(!dll)return 2;auto fn=reinterpret_cast<HRESULT(WINAPI*)()>(GetProcAddress(dll,cmd==L"--register"?"DllRegisterServer":"DllUnregisterServer"));HRESULT hr=fn?fn():E_NOINTERFACE;std::wcout<<cmd<<L" HRESULT 0x"<<std::hex<<hr<<L"\n";FreeLibrary(dll);return FAILED(hr)?3:0;}
 if(cmd!=L"--probe"&&cmd!=L"--probe-producer")return 4;
 HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(hr))return 5;ICreateDevEnum*devices=nullptr;IEnumMoniker*items=nullptr;IMoniker*m=nullptr;IBaseFilter*filter=nullptr;
 hr=CoCreateInstance(CLSID_SystemDeviceEnum,nullptr,CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,(void**)&devices);if(SUCCEEDED(hr))hr=devices->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&items,0);
 if(hr==S_OK){while(items->Next(1,&m,nullptr)==S_OK){IPropertyBag*bag=nullptr;m->BindToStorage(nullptr,nullptr,IID_IPropertyBag,(void**)&bag);VARIANT value;VariantInit(&value);if(bag&&SUCCEEDED(bag->Read(L"FriendlyName",&value,nullptr))&&value.vt==VT_BSTR&&wcscmp(value.bstrVal,L"BandHeartBeat Virtual Camera")==0){std::wcout<<L"DirectShow category: BandHeartBeat Virtual Camera found\n";hr=m->BindToObject(nullptr,nullptr,IID_IBaseFilter,(void**)&filter);VariantClear(&value);bag->Release();m->Release();m=nullptr;break;}VariantClear(&value);if(bag)bag->Release();m->Release();m=nullptr;}}
 int result=6;if(filter){std::wcout<<L"BindToObject HRESULT S_OK\n";IPin*pin=nullptr;hr=filter->FindPin(L"Output",&pin);auto sink=new TestSink();sink->verifyPixel=false;sink->producerPattern=cmd==L"--probe-producer";if(SUCCEEDED(hr))hr=pin->Connect(sink,nullptr);if(SUCCEEDED(hr))hr=filter->Run(0);bool delivered=SUCCEEDED(hr)&&sink->Wait(3)&&sink->valid;filter->Stop();if(pin){pin->Disconnect();pin->Release();}sink->Disconnect();sink->Release();filter->Release();result=delivered?0:7;std::cout<<(delivered?"PASS registered moniker -> BindToObject -> allocator/Receive 3 samples\n":"FAIL registered sample delivery\n");}else std::wcerr<<L"Virtual source not found/bind failed HRESULT 0x"<<std::hex<<hr<<L"\n";
 if(items)items->Release();if(devices)devices->Release();CoUninitialize();return result;
}
